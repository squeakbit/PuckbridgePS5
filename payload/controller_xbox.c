/* controller_xbox.c — Xbox One S GIP controller for Ghost-Control
 * All button bit positions hardware-confirmed on PS5 via live GIP probe.
 * Reference: xboxSeriesSButtonBits.md
 */

#include "controller_xbox.h"
#include "xbox_adapter.h"
#include "usb_helpers.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __PROSPERO__
#include <ps5/klog.h>
#define LOG(...) klog_printf("[GC] " __VA_ARGS__)
#else
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#endif

/* Counts GIP INPUT packets — gates Guide button injection at startup */
static uint32_t g_input_count = 0;
static uint8_t wireless_init_sequence;
static int wireless_initialized;

#define GIP_OPT_ACK         0x10u
#define GIP_OPT_SYSTEM      0x20u
#define GIP_OPT_CHUNK_START 0x40u
#define GIP_OPT_CHUNK       0x80u
#define GIP_MAX_PACKET      65535u

struct wireless_gip_header {
    uint8_t command;
    uint8_t options;
    uint8_t sequence;
    size_t header_length;
    size_t packet_length;
    size_t chunk_offset;
};

struct wireless_gip_chunk {
    uint8_t *packet;
    size_t total;
    size_t received;
    uint8_t command;
    uint8_t options;
    uint8_t sequence;
    int active;
};

static struct wireless_gip_chunk wireless_chunk;

/* ── helpers ──────────────────────────────────────────────────────────── */

static uint8_t trig_scale(uint16_t v) {
    if (v > 1023u) v = 1023u;
    return (uint8_t)((v * 255u) / 1023u);
}

#define DEADZONE 7849

static uint8_t stick_x(int16_t v) {
    return (v > DEADZONE || v < -DEADZONE) ? (uint8_t)((v + 32768) >> 8) : 128u;
}

static uint8_t stick_y(int16_t v) {
    return (v > DEADZONE || v < -DEADZONE) ? (uint8_t)(255 - ((v + 32768) >> 8)) : 128u;
}

/* ── input parsing ────────────────────────────────────────────────────── */

/* Parse GIP INPUT (cmd=0x20, 18 bytes) into ScePadData.
 * Hardware-confirmed bit positions — see xboxSeriesSButtonBits.md */
static void parse_input_payload(const uint8_t *b, ScePadData *o) {
    uint8_t  b4   = b[0];
    uint8_t  b5   = b[1];
    uint16_t lt16 = (uint16_t)b[2]  | ((uint16_t)b[3]  << 8);
    uint16_t rt16 = (uint16_t)b[4]  | ((uint16_t)b[5]  << 8);
    int16_t  lx   = (int16_t)((uint16_t)b[6] | ((uint16_t)b[7] << 8));
    int16_t  ly   = (int16_t)((uint16_t)b[8] | ((uint16_t)b[9] << 8));
    int16_t  rx   = (int16_t)((uint16_t)b[10] | ((uint16_t)b[11] << 8));
    int16_t  ry   = (int16_t)((uint16_t)b[12] | ((uint16_t)b[13] << 8));

    uint8_t lt = trig_scale(lt16);
    uint8_t rt = trig_scale(rt16);

    o->leftStick.x      = stick_x(lx);
    o->leftStick.y      = stick_y(ly);
    o->rightStick.x     = stick_x(rx);
    o->rightStick.y     = stick_y(ry);
    o->analogButtons.l2 = lt;
    o->analogButtons.r2 = rt;

    uint32_t btn = 0;

    /* b[4]: system + face buttons */
    if (b4 & 0x04u) btn |= SCE_PAD_BUTTON_OPTIONS;  /* Menu (≡)  → Options  */
    if (b4 & 0x08u) btn |= SCE_PAD_BUTTON_SHARE;    /* View (⧉)  → Share    */
    if (b4 & 0x10u) btn |= SCE_PAD_BUTTON_CROSS;    /* A         → Cross    */
    if (b4 & 0x20u) btn |= SCE_PAD_BUTTON_CIRCLE;   /* B         → Circle   */
    if (b4 & 0x40u) btn |= SCE_PAD_BUTTON_SQUARE;   /* X         → Square   */
    if (b4 & 0x80u) btn |= SCE_PAD_BUTTON_TRIANGLE; /* Y         → Triangle */

    /* b[5]: dpad + bumpers + stick clicks */
    if (b5 & 0x01u) btn |= SCE_PAD_BUTTON_UP;
    if (b5 & 0x02u) btn |= SCE_PAD_BUTTON_DOWN;
    if (b5 & 0x04u) btn |= SCE_PAD_BUTTON_LEFT;
    if (b5 & 0x08u) btn |= SCE_PAD_BUTTON_RIGHT;
    if (b5 & 0x10u) btn |= SCE_PAD_BUTTON_L1;       /* LB → L1 */
    if (b5 & 0x20u) btn |= SCE_PAD_BUTTON_R1;       /* RB → R1 */
    if (b5 & 0x40u) btn |= SCE_PAD_BUTTON_L3;       /* LS → L3 */
    if (b5 & 0x80u) btn |= SCE_PAD_BUTTON_R3;       /* RS → R3 */

    /* Triggers: analog + digital threshold */
    if (lt > 16u) btn |= SCE_PAD_BUTTON_L2;
    if (rt > 16u) btn |= SCE_PAD_BUTTON_R2;

    o->buttons   = btn;
    o->connected = 1;
    o->quat.w    = 1.0f;
}

void xbox_parse_input(const uint8_t *b, ScePadData *o) {
    parse_input_payload(b + 4, o);
}

/* ── GIP protocol ─────────────────────────────────────────────────────── */

static int read_one(int fd, struct usb_fs_endpoint *eps,
                    uint8_t *buf, uint32_t timeout_ms) {
    void *b[1] = {buf}; uint32_t l[1] = {64};
    eps[0].ppBuffer = b; eps[0].pLength = l; eps[0].nFrames   = 1;
    eps[0].timeout  = timeout_ms; eps[0].aFrames = 0; eps[0].status = 0;
    eps[0].flags    = USB_FS_FLAG_SINGLE_SHORT_OK | USB_FS_FLAG_MULTI_SHORT_OK;

    struct usb_fs_start st; memset(&st, 0, sizeof(st)); st.ep_index = 0;
    if (ioctl(fd, USB_FS_START, &st) != 0) return -errno;

    int polls = (int)((timeout_ms + 300) / 50) + 1;
    for (int w = 0; w < polls; w++) {
        struct usb_fs_complete co; memset(&co, 0, sizeof(co)); co.ep_index = 0;
        if (ioctl(fd, USB_FS_COMPLETE, &co) == 0) {
            if (eps[0].aFrames == 0 || l[0] == 0) return 0;
            return (int)l[0];
        }
        if (errno != EBUSY) {
            struct usb_fs_stop sp; memset(&sp, 0, sizeof(sp)); sp.ep_index = 0;
            ioctl(fd, USB_FS_STOP, &sp);
            return -errno;
        }
        usleep(50000);
    }
    struct usb_fs_stop sp; memset(&sp, 0, sizeof(sp)); sp.ep_index = 0;
    ioctl(fd, USB_FS_STOP, &sp);
    return 0;
}

static void send_ack(int fd, struct usb_fs_endpoint *eps, uint8_t orig_seq) {
    uint8_t ack[8] = {GIP_CMD_ACK, 0x00, 0x00, 0x04,
                      orig_seq, GIP_CMD_ANNOUNCE, 0x00, 0x00};
    usb_send_out(fd, &eps[1], ack, 8, "ack");
}

void xbox_gip_handshake(int fd, struct usb_fs_endpoint *eps) {
    static const uint8_t power[] = {0x05, 0x20, 0x00, 0x01, 0x00};
    uint8_t buf[64];
    int announced = 0;

    g_input_count = 0;

    /* Pass 0: wait for ANNOUNCE; pass 1: send hello to re-trigger */
    for (int pass = 0; pass < 2 && !announced; pass++) {
        if (pass == 1) {
            uint8_t hello[4] = {GIP_CMD_ACK, 0x00, 0x00, 0x00};
            usb_send_out(fd, &eps[1], hello, 4, "hello");
        }
        /* Pass 0: 3 reads — if ANNOUNCE was already sent during probe, we miss it fast.
         * Pass 1: send hello to re-trigger, wait longer. Handles both direct + hub. */
        int reads = (pass == 0) ? 3 : 60;
        for (int i = 0; i < reads && !announced; i++) {
            int n = read_one(fd, eps, buf, 150);
            if (n > 0 && buf[0] == GIP_CMD_ANNOUNCE) {
                send_ack(fd, eps, buf[1]);
                announced = 1;
            } else if (n > 0 && buf[0] == GIP_CMD_INPUT) {
                announced = 1;
            }
        }
    }

    usb_send_out(fd, &eps[1], power, 5, "power");
    LOG("Xbox handshake done (announced=%d)\n", announced);
}

int xbox_handle_packet(int fd, struct usb_fs_endpoint *eps,
                       const uint8_t *buf, uint32_t len,
                       ScePadData *out_pad) {
    (void)fd; (void)eps;
    uint8_t cmd = buf[0];

    /* Player input */
    if (cmd == GIP_CMD_INPUT && len >= 18) {
        g_input_count++;
        xbox_parse_input(buf, out_pad);
        return 1;
    }

    /* Guide button (Xbox logo): cmd=0x07, b[4]=0x01 pressed, 0x00 released.
     * Controller auto-sends this at connect — gate behind 10 INPUT packets
     * so the startup auto-send does not inject a PS button press. */
    if (cmd == 0x07) {
        out_pad->connected    = 1;
        out_pad->quat.w       = 1.0f;
        out_pad->leftStick.x  = 128;
        out_pad->leftStick.y  = 128;
        out_pad->rightStick.x = 128;
        out_pad->rightStick.y = 128;
        if (g_input_count > 10 && len >= 5 && (buf[4] & 0x01u))
            out_pad->buttons = SCE_PAD_BUTTON_PS;
        return 1;
    }

    /* Re-announce during streaming */
    if (cmd == GIP_CMD_ANNOUNCE && len >= 4) {
        static const uint8_t power[] = {0x05, 0x20, 0x00, 0x01, 0x00};
        send_ack(fd, eps, buf[1]);
        usb_send_out(fd, &eps[1], power, 5, "repower");
        return 0;
    }

    return 0;
}

static void clear_wireless_chunk(void) {
    free(wireless_chunk.packet);
    memset(&wireless_chunk, 0, sizeof wireless_chunk);
}

void xbox_wireless_reset(void) {
    clear_wireless_chunk();
    wireless_init_sequence = 0;
    wireless_initialized = 0;
    g_input_count = 0;
}

static int decode_varint(const uint8_t *buf, size_t len, size_t *cursor,
                         size_t *value) {
    uint32_t result = 0;
    unsigned shift = 0;

    for (unsigned count = 0; count < 5; count++) {
        uint8_t byte;

        if (*cursor >= len) return 0;
        byte = buf[(*cursor)++];
        if (count == 4 && (byte & 0xf0u) != 0) return 0;
        result |= (uint32_t)(byte & 0x7fu) << shift;
        if ((byte & 0x80u) == 0) {
            *value = result;
            return 1;
        }
        shift += 7;
    }
    return 0;
}

static int parse_wireless_header(const uint8_t *buf, size_t len,
                                 struct wireless_gip_header *header) {
    size_t cursor = 3;

    if (buf == NULL || header == NULL || len < 4) return 0;
    header->command = buf[0];
    header->options = buf[1];
    header->sequence = buf[2];
    if (!decode_varint(buf, len, &cursor, &header->packet_length)) return 0;
    header->chunk_offset = 0;
    if ((header->options & GIP_OPT_CHUNK) != 0 &&
        !decode_varint(buf, len, &cursor, &header->chunk_offset))
        return 0;
    if (header->packet_length > len - cursor) return 0;
    header->header_length = cursor;
    return 1;
}

static size_t encode_wireless_header(uint8_t *buf, uint8_t command,
                                     uint8_t options, uint8_t sequence,
                                     size_t packet_length) {
    size_t cursor = 3;
    size_t remaining = packet_length;

    buf[0] = command;
    buf[1] = options;
    buf[2] = sequence;
    do {
        uint8_t byte = (uint8_t)(remaining & 0x7fu);
        remaining >>= 7;
        if (remaining != 0) byte |= 0x80u;
        buf[cursor++] = byte;
    } while (remaining != 0);

    if ((cursor & 1u) != 0) {
        buf[cursor - 1] |= 0x80u;
        buf[cursor++] = 0;
    }
    return cursor;
}

static int acknowledge_wireless_chunk(const struct wireless_gip_header *header,
                                      size_t offset, size_t total) {
    uint8_t ack[13] = {
        GIP_CMD_ACK, GIP_OPT_SYSTEM, header->sequence, 9,
        0, header->command, GIP_OPT_SYSTEM, 0, 0, 0, 0, 0, 0
    };
    size_t acknowledged = offset + header->packet_length;

    if (acknowledged > total || acknowledged > 0xffffu) return 0;
    ack[7] = (uint8_t)(acknowledged & 0xffu);
    ack[8] = (uint8_t)((acknowledged >> 8) & 0xffu);
    ack[11] = (uint8_t)((total - acknowledged) & 0xffu);
    ack[12] = (uint8_t)(((total - acknowledged) >> 8) & 0xffu);
    if (!xbox_adapter_send_gip(ack, sizeof ack)) {
        LOG("Xbox adapter GIP chunk ACK failed errno=%d\n", errno);
        return 0;
    }
    return 1;
}

static int process_wireless_chunk(const uint8_t *buf, size_t len,
                                  const uint8_t **complete,
                                  size_t *complete_length) {
    struct wireless_gip_header header;
    size_t total, offset;

    *complete = NULL;
    *complete_length = 0;
    if (!parse_wireless_header(buf, len, &header) ||
        (header.options & GIP_OPT_CHUNK) == 0) {
        LOG("Malformed Xbox wireless GIP chunk header\n");
        clear_wireless_chunk();
        return 0;
    }

    if ((header.options & GIP_OPT_CHUNK_START) != 0) {
        total = header.chunk_offset;
        if (total == 0 || total > GIP_MAX_PACKET ||
            header.packet_length > total) {
            LOG("Invalid Xbox wireless GIP chunk size %u\n", (unsigned)total);
            clear_wireless_chunk();
            return 0;
        }
        if (wireless_chunk.active && wireless_chunk.packet != NULL &&
            wireless_chunk.command == header.command &&
            wireless_chunk.sequence == header.sequence &&
            wireless_chunk.total == total &&
            wireless_chunk.received >= header.packet_length &&
            memcmp(wireless_chunk.packet + 6,
                   buf + header.header_length, header.packet_length) == 0) {
            if ((header.options & GIP_OPT_ACK) != 0)
                return acknowledge_wireless_chunk(&header, 0, total);
            return 0;
        }
        clear_wireless_chunk();
        wireless_chunk.packet = malloc(total + 6);
        if (wireless_chunk.packet == NULL) {
            LOG("Could not allocate %u bytes for Xbox wireless GIP chunks\n",
                (unsigned)(total + 6));
            return 0;
        }
        wireless_chunk.active = 1;
        wireless_chunk.total = total;
        wireless_chunk.command = header.command;
        wireless_chunk.options = header.options;
        wireless_chunk.sequence = header.sequence;
        offset = 0;
    } else {
        if (!wireless_chunk.active || wireless_chunk.packet == NULL ||
            wireless_chunk.command != header.command ||
            wireless_chunk.sequence != header.sequence) {
            LOG("Xbox wireless GIP chunk arrived without a matching start\n");
            return 0;
        }
        total = wireless_chunk.total;
        offset = header.chunk_offset;
    }

    if (offset > total || header.packet_length > total - offset) {
        LOG("Xbox wireless GIP chunk exceeds message bounds\n");
        return 0;
    }
    if (offset < wireless_chunk.received) {
        if (header.packet_length <= wireless_chunk.received - offset &&
            memcmp(wireless_chunk.packet + 6 + offset,
                   buf + header.header_length, header.packet_length) == 0) {
            if ((header.options & GIP_OPT_ACK) != 0)
                return acknowledge_wireless_chunk(&header, offset, total);
            return 0;
        }
        LOG("Conflicting duplicate Xbox wireless GIP chunk at %u\n",
            (unsigned)offset);
        return 0;
    }
    if (offset != wireless_chunk.received) {
        LOG("Out-of-order Xbox wireless GIP chunk at %u (expected %u)\n",
            (unsigned)offset, (unsigned)wireless_chunk.received);
        return 0;
    }

    if ((header.options & GIP_OPT_ACK) != 0 &&
        !acknowledge_wireless_chunk(&header, offset, total)) {
        clear_wireless_chunk();
        return 0;
    }
    if (header.packet_length != 0) {
        memcpy(wireless_chunk.packet + 6 + offset,
               buf + header.header_length, header.packet_length);
        wireless_chunk.received += header.packet_length;
        return 0;
    }
    if (wireless_chunk.received != total) {
        LOG("Incomplete Xbox wireless GIP message: received %u of %u bytes\n",
            (unsigned)wireless_chunk.received, (unsigned)total);
        clear_wireless_chunk();
        return 0;
    }

    {
        uint8_t encoded_header[6];
        size_t header_length;
        uint8_t options = wireless_chunk.options &
            (uint8_t)~(GIP_OPT_ACK | GIP_OPT_CHUNK_START | GIP_OPT_CHUNK);

        header_length = encode_wireless_header(
            encoded_header, wireless_chunk.command, options,
            wireless_chunk.sequence, total);
        if (header_length > sizeof encoded_header) {
            LOG("Xbox wireless GIP reassembled header exceeded its buffer\n");
            clear_wireless_chunk();
            return 0;
        }
        memmove(wireless_chunk.packet + header_length,
                wireless_chunk.packet + 6, total);
        memcpy(wireless_chunk.packet, encoded_header, header_length);
        *complete = wireless_chunk.packet;
        *complete_length = header_length + total;
        wireless_chunk.packet = NULL;
        memset(&wireless_chunk, 0, sizeof wireless_chunk);
    }
    return 1;
}

static void initialize_wireless_controller(void) {
    const uint8_t led_on[] = { 0x00, 0x01, 0x14 };
    uint8_t power[5];
    uint8_t led[7];

    if (wireless_initialized) return;
    power[0] = 0x05;
    power[1] = GIP_OPT_SYSTEM;
    power[2] = wireless_init_sequence++;
    power[3] = 1;
    power[4] = 0;
    led[0] = 0x0a;
    led[1] = GIP_OPT_SYSTEM;
    led[2] = wireless_init_sequence++;
    led[3] = 3;
    memcpy(led + 4, led_on, sizeof led_on);
    if (!xbox_adapter_send_gip(power, sizeof power)) {
        LOG("Xbox adapter GIP power-on failed errno=%d\n", errno);
        return;
    }
    if (!xbox_adapter_send_gip(led, sizeof led)) {
        LOG("Xbox adapter GIP LED initialization failed errno=%d\n", errno);
        return;
    }
    wireless_initialized = 1;
    LOG("Xbox wireless GIP power-on and LED initialization sent\n");
}

static int handle_wireless_message(const uint8_t *buf, size_t len,
                                   ScePadData *out_pad) {
    struct wireless_gip_header header;
    const uint8_t *payload;

    if (!parse_wireless_header(buf, len, &header)) return 0;
    payload = buf + header.header_length;
    if ((header.options & GIP_OPT_ACK) != 0 &&
        (header.options & GIP_OPT_CHUNK) == 0) {
        uint8_t ack[13] = {
            GIP_CMD_ACK, GIP_OPT_SYSTEM, header.sequence, 9,
            0, header.command, GIP_OPT_SYSTEM, buf[3],
            0, 0, 0, 0, 0
        };
        if (!xbox_adapter_send_gip(ack, sizeof ack))
            LOG("Xbox adapter GIP ACK failed errno=%d\n", errno);
    }

    if (header.command == 0x04u && !wireless_initialized)
        initialize_wireless_controller();

    if (header.command == GIP_CMD_INPUT && header.packet_length >= 14) {
        g_input_count++;
        parse_input_payload(payload, out_pad);
        return 1;
    }

    if (header.command == 0x07u) {
        out_pad->connected = 1;
        out_pad->quat.w = 1.0f;
        out_pad->leftStick.x = out_pad->leftStick.y = 128;
        out_pad->rightStick.x = out_pad->rightStick.y = 128;
        if (g_input_count > 10 && header.packet_length >= 1 &&
            (payload[0] & 0x01u))
            out_pad->buttons = SCE_PAD_BUTTON_PS;
        return 1;
    }

    return 0;
}

int xbox_handle_wireless_packet(const uint8_t *buf, uint32_t len,
                                ScePadData *out_pad) {
    struct wireless_gip_header header;
    const uint8_t *message = buf;
    size_t message_length = len;

    if (!parse_wireless_header(buf, len, &header)) {
        LOG("Malformed Xbox wireless GIP message (%u bytes)\n", len);
        return 0;
    }
    if ((header.options & GIP_OPT_CHUNK) != 0) {
        if (!process_wireless_chunk(buf, len, &message, &message_length) ||
            message == NULL)
            return 0;
    }
    return handle_wireless_message(message, message_length, out_pad);
}
