/* Xbox Wireless Adapter support adapted from FGG-XSense (GPL-3.0-or-later).
 * Its radio transport incorporates xone code (GPL-2.0-or-later). */
#include "xbox_adapter.h"

#ifdef HAS_XBOX_WIRELESS_FIRMWARE
#include "xbox_adapter_firmware.h"
#endif

#include <sys/ioctl.h>
#include <sys/types.h>

#include <dev/usb/usb.h>
#include <dev/usb/usb_ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern void ghostpad_status_log(const char *fmt, ...);
#define log_line(...) ghostpad_status_log("[GC] " __VA_ARGS__)

#ifdef HAS_XBOX_WIRELESS_FIRMWARE
#define XBOX_ADAPTER_BULK_OUT  0x04
#define XBOX_ADAPTER_BULK_IN_WLAN 0x84
#define XBOX_ADAPTER_BULK_IN_CMD  0x85
#define XBOX_ADAPTER_EP_OUT_INDEX 0
#define XBOX_ADAPTER_EP_IN_CMD_INDEX 1
#define XBOX_ADAPTER_EP_IN_WLAN_INDEX 2
#define XBOX_ADAPTER_INTERFACE 0
#define XBOX_ADAPTER_IN_BUFFER_SIZE 512

#define MT_REQ_DEV_MODE  0x01
#define MT_REQ_MULTI_WRITE 0x06
#define MT_REQ_MULTI_READ  0x07
#define MT_REQ_WRITE_CFG 0x46
#define MT_REQ_READ_CFG  0x47

#define MT_USB_U3DMA_CFG             0x9018
#define MT_FCE_DMA_ADDR              0x0230
#define MT_FCE_DMA_LEN               0x0234
#define MT_FCE_PSE_CTRL              0x0800
#define MT_TX_CPU_FROM_FCE_BASE_PTR  0x09a0
#define MT_TX_CPU_FROM_FCE_MAX_COUNT 0x09a4
#define MT_TX_CPU_FROM_FCE_DESC_IDX  0x09a8
#define MT_FCE_PDMA_GLOBAL_CONF      0x09c4
#define MT_FCE_SKIP_FS               0x0a6c
#define MT_RF_PATCH                  0x0130
#define MT_EFUSE_CTRL                0x0024
#define MT_EFUSE_DATA_BASE           0x0028
#define MT_CMB_CTRL                  0x0020
#define MT_XO_CTRL5                  0x0114
#define MT_XO_CTRL6                  0x0118
#define MT_USB_DMA_CFG               0x0238
#define MT_WPDMA_GLO_CFG             0x0208
#define MT_WMM_AIFSN                 0x0214
#define MT_WMM_CWMIN                 0x0218
#define MT_WMM_CWMAX                 0x021c
#define MT_TSO_CTRL                  0x0250
#define MT_PBF_SYS_CTRL              0x0400
#define MT_PBF_TX_MAX_PCNT           0x0408
#define MT_MAC_SYS_CTRL              0x1004
#define MT_MAC_ADDR_DW0              0x1008
#define MT_MAC_BSSID_DW0             0x1010
#define MT_MAX_LEN_CFG               0x1018
#define MT_AMPDU_MAX_LEN_20M1S       0x1030
#define MT_AMPDU_MAX_LEN_20M2S       0x1034
#define MT_AUTO_RSP_CFG              0x1404
#define MT_BKOFF_SLOT_CFG            0x1104
#define MT_PWR_PIN_CFG               0x1204
#define MT_EDCA_CFG_BASE             0x1300
#define MT_TX_PIN_CFG                0x1328
#define MT_TX_SW_CFG0                0x1330
#define MT_TX_SW_CFG1                0x1334
#define MT_TXOP_CTRL_CFG             0x1340
#define MT_TX_TIMEOUT_CFG            0x1348
#define MT_TX_RETRY_CFG              0x134c
#define MT_CCK_PROT_CFG              0x1364
#define MT_OFDM_PROT_CFG             0x1368
#define MT_MM20_PROT_CFG             0x136c
#define MT_GF20_PROT_CFG             0x1374
#define MT_GF40_PROT_CFG             0x1378
#define MT_EXP_ACK_TIME              0x1380
#define MT_TX_ALC_CFG_2              0x13a8
#define MT_TX_ALC_CFG_3              0x13ac
#define MT_TX_ALC_CFG_0              0x13b0
#define MT_TX_ALC_CFG_4              0x13c0
#define MT_PIFS_TX_CFG               0x13ec
#define MT_RX_FILTR_CFG              0x1400
#define MT_LEGACY_BASIC_RATE         0x1408
#define MT_HT_BASIC_RATE             0x140c
#define MT_PN_PAD_MODE               0x150c
#define MT_TXOP_HLDR_ET              0x1608
#define MT_TX_PROT_CFG6              0x13e0
#define MT_TX_PROT_CFG7              0x13e4
#define MT_TX_PROT_CFG8              0x13e8
#define MT_DACCLK_EN_DLY_CFG         0x1264
#define MT_RF_PA_MODE_ADJ0           0x1228
#define MT_RF_PA_MODE_ADJ1           0x122c
#define MT_TX0_RF_GAIN_CORR           0x13a0
#define MT_TX1_RF_GAIN_CORR           0x13a4
#define MT_PBF_CFG                   0x0404
#define MT_PAUSE_ENABLE_CONTROL1     0x0a38
#define MT_RF_BYPASS_0               0x0504
#define MT_RF_SETTING_0              0x050c
#define MT_XIFS_TIME_CFG             0x1100
#define MT_FCE_L2_STUFF              0x080c
#define MT_TX_RTS_CFG                0x1344
#define MT_BEACON_TIME_CFG           0x1114
#define MT_EXT_CCA_CFG               0x141c
#define MT_CH_TIME_CFG               0x110c
#define MT_BBP_AGC_BASE              0x2300
#define MT_EE_MAC_ADDR               0x0004
#define MT_EE_XTAL_TRIM_1            0x003a
#define MT_EE_XTAL_TRIM_2            0x009e
#define MT_EE_TX_POWER_0_START_2G    0x0056
#define MT_EE_TX_POWER_0_START_5G    0x0062
#define MT_TX_POWER_GROUP_SIZE_5G    5
#define MT_MCU_MEMMAP_WLAN           0x410000u
#define MT_BEACON_BASE               0xc000u

#define MT_FW_LOAD_IVB  0x12
#define MT_MCU_ILM_OFFSET 0x00080000u
#define MT_MCU_DLM_OFFSET 0x00110800u
#define MT_FW_CHUNK_SIZE 0x3800u
#define MT_DMA_COMPLETE 0xc0000000u

#define MT_MCU_MSG_TYPE_CMD 0x40000000u
#define MT_MCU_MSG_PORT_CPU_TX 0x10000000u
#define MT_MCU_CMD_FUN_SET_OP 1u
#define MT_MCU_CMD_LOAD_CR 2u
#define MT_MCU_CMD_LED_MODE_OP 16u
#define MT_MCU_CMD_POWER_SAVING_OP 20u
#define MT_MCU_FUNCTION_Q_SELECT 1u
#define MT_MCU_CR_RF_BBP 2u
#define MT_MCU_RADIO_ON 0x31u
#define MT_MCU_RADIO_OFF 0x30u
#define MT_MCU_CMD_INIT_GAIN_OP 3u
#define MT_MCU_CMD_BURST_WRITE 8u
#define MT_MCU_CMD_SWITCH_CHANNEL_OP 30u
#define MT_MCU_CMD_CALIBRATION_OP 31u
#define MT_MCU_EVT_BUTTON 4u
#define MT_MCU_EVT_PACKET_RX 12u
#define MT_MCU_EVT_CLIENT_LOST 14u
#define MT_MCU_CAL_TEMP_SENSOR 2u
#define MT_MCU_CAL_RXDCOC 3u
#define MT_MCU_CAL_RC 4u
#define MT_TXD_INFO_80211         0x00080000u
#define MT_TXD_INFO_WIV           0x01000000u
#define MT_TXD_INFO_QSEL_EDCA     0x04000000u
#define MT_TXD_INFO_DPORT_WLAN    0x00000000u /* WLAN is DMA port 0. */
#define MT_MCU_TXD_INFO_WLAN (MT_TXD_INFO_DPORT_WLAN | \
                              MT_TXD_INFO_QSEL_EDCA | \
                              MT_TXD_INFO_WIV | \
                              MT_TXD_INFO_80211)
#define MT_MCU_TXWI_ACK_REQUEST 0x01u
#define MT_MCU_RXWI_SIZE 32u
#define MT_MCU_TXWI_SIZE 20u
#define MT_WIFI_HEADER_SIZE 24u
#define MT_WIFI_QOS_CONTROL_SIZE 2u
#define MT_WIFI_L2_PADDING_SIZE 2u
#define MT_MCU_RXINFO_L2PAD (1u << 14)
#define MT_WIFI_RESERVED_SUBTYPE 0x0070u
#define MT_WIFI_CLIENT_PAIR_REQ 0x01u
#define MT_WIFI_CLIENT_PAIR_RESP 0x02u
#define MT_WIFI_CLIENT_ENABLE_ENCRYPTION 0x10u
#define MT_WIFI_MAX_CLIENTS 16u
#define MT_WIFI_WCID_ADDR_BASE 0x1800u
#define MT_WIFI_WCID_KEY_BASE 0x8000u
#define MT_WIFI_WCID_IV_BASE 0xa000u
#define MT_WIFI_WCID_ATTR_BASE 0xa800u
#define MT_WIFI_CLIENT_ADD 1u
#define MT_WIFI_INPUT_QUEUE_SIZE 8
#define MT_MS_SET_MAC_ADDRESS 0u
#define MT_MS_SET_IDLE_TIME 5u
#define MT_MS_SET_CHAN_CANDIDATES 7u
#define MT_GIP_CMD_ACK 0x01u
#define MT_GIP_CMD_ANNOUNCE 0x02u
#define MT_GIP_CMD_IDENTIFY 0x04u
#define MT_GIP_OPT_SYSTEM 0x20u
#define MT_GIP_OPT_ACK 0x10u
#define MT_PHY_BW_20 0u
#define MT_PHY_BW_40 1u
#define MT_PHY_BW_80 2u

/* Radio and dongle protocol handling adapted from xone, GPL-2.0-or-later.
 * Copyright (C) 2021 Severin von Wnuck-Lipinski. */

#define XFER_TIMEOUT_MS 2000
#define POLL_TIMEOUT_MS 1000
#define POLL_INTERVAL_US 1000
#define USB_FS_INDEX_COUNT 3

typedef struct {
    int fd;
    char operation[128];
    struct usb_fs_endpoint endpoints[USB_FS_INDEX_COUNT];
    void *buffer_pointer[USB_FS_INDEX_COUNT][1];
    uint32_t buffer_length[USB_FS_INDEX_COUNT][1];
    unsigned char buffer[4 + MT_FW_CHUNK_SIZE + 4];
    unsigned char input_buffers[2][XBOX_ADAPTER_IN_BUFFER_SIZE];
    uint16_t endpoint_max_packet_length;
    uint32_t endpoint_max_bufsize;
    int fs_initialized;
    int interface_claimed;
    int input_receives_started;
} adapter_transport;

typedef struct {
    unsigned char address[6];
    int present;
    int encryption_enabled;
    int gip_announce_seen;
    unsigned char gip_id;
    int gip_id_valid;
} adapter_client;

static adapter_transport adapter;
static unsigned char adapter_address[6];
static int adapter_pairing_enabled;
static unsigned char gip_message[512];
static size_t gip_message_length;
static uint8_t gip_message_wcid;
static adapter_client adapter_clients[MT_WIFI_MAX_CLIENTS];
static unsigned char adapter_input_queue[MT_WIFI_INPUT_QUEUE_SIZE]
                                        [XBOX_ADAPTER_IN_BUFFER_SIZE];
static uint16_t adapter_input_lengths[MT_WIFI_INPUT_QUEUE_SIZE];
static unsigned adapter_input_head, adapter_input_tail, adapter_input_count;
static unsigned adapter_input_debug_count[2];
static unsigned adapter_wifi_frame_debug_count;
static unsigned adapter_wifi_data_debug_count;
static unsigned adapter_gip_debug_count;
static unsigned adapter_gip_rx_debug_count;
static unsigned adapter_gip_tx_debug_count;
static uint8_t adapter_gip_sequence = 1;

static void set_operation(adapter_transport *dev, const char *operation)
{
    snprintf(dev->operation, sizeof dev->operation, "%s", operation);
}

static uint32_t read_le32(const unsigned char *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void write_le32(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static int get_device_stats(adapter_transport *dev,
                            struct usb_device_stats *stats)
{
    memset(stats, 0, sizeof *stats);
    return ioctl(dev->fd, USB_DEVICESTATS, stats) == 0;
}

static int log_ioctl_failure(adapter_transport *dev, const char *ioctl_name)
{
    int saved_errno = errno;

    log_line("  adapter %s (%s) failed: %s (%d)", ioctl_name,
             dev->operation, strerror(saved_errno), saved_errno);
    errno = saved_errno;
    return 0;
}

static int queue_adapter_input(const unsigned char *data, size_t length,
                               int endpoint_index)
{
    unsigned index;

    if ((endpoint_index == XBOX_ADAPTER_EP_IN_CMD_INDEX ||
         endpoint_index == XBOX_ADAPTER_EP_IN_WLAN_INDEX) &&
        adapter_input_debug_count[endpoint_index -
                                  XBOX_ADAPTER_EP_IN_CMD_INDEX] < 16) {
        uint32_t info = length >= 4 ? read_le32(data) : 0;
        size_t dump_length = length < 32 ? length : 32;
        char hex[32 * 2 + 1];
        size_t i;

        for (i = 0; i < dump_length; i++)
            snprintf(hex + i * 2, sizeof hex - i * 2, "%02x", data[i]);
        hex[dump_length * 2] = '\0';
        log_line("  adapter RX diagnostic EP 0x%02x: %u bytes, "
                 "FCE 0x%08x port %u event %u sequence %u, "
                 "first %u bytes %s",
                 endpoint_index == XBOX_ADAPTER_EP_IN_CMD_INDEX ?
                     XBOX_ADAPTER_BULK_IN_CMD : XBOX_ADAPTER_BULK_IN_WLAN,
                 (unsigned)length, info, (info >> 27) & 7u,
                 (info >> 20) & 0x0fu, (info >> 16) & 0x0fu,
                 (unsigned)dump_length, hex);
        adapter_input_debug_count[endpoint_index -
                                  XBOX_ADAPTER_EP_IN_CMD_INDEX]++;
    }

    if (adapter_input_count == MT_WIFI_INPUT_QUEUE_SIZE) {
        log_line("  adapter wireless input queue full; dropping packet");
        return 0;
    }

    index = adapter_input_tail;
    if (length > sizeof adapter_input_queue[index])
        length = sizeof adapter_input_queue[index];
    memcpy(adapter_input_queue[index], data, length);
    adapter_input_lengths[index] = (uint16_t)length;
    adapter_input_tail = (adapter_input_tail + 1) % MT_WIFI_INPUT_QUEUE_SIZE;
    adapter_input_count++;
    return 1;
}

static const char *usb_status_name(int status)
{
    switch (status) {
    case 0: return "success";
    case 20: return "timeout";
    case 22: return "stalled";
    default:
        return "unknown";
    }
}

static void probe_device_descriptor(adapter_transport *dev)
{
    struct usb_device_descriptor descriptor;

    memset(&descriptor, 0, sizeof descriptor);
    if (ioctl(dev->fd, USB_GET_DEVICE_DESC, &descriptor) == 0) {
        log_line("  adapter standard USB descriptor probe succeeded: "
                 "VID %04x, PID %04x, USB %04x",
                 UGETW(descriptor.idVendor), UGETW(descriptor.idProduct),
                 UGETW(descriptor.bcdUSB));
    } else {
        int saved_errno = errno;

        log_line("  adapter standard USB descriptor probe failed: %s (%d)",
                 strerror(saved_errno), saved_errno);
    }
}

static int control_transfer(adapter_transport *dev, uint8_t request,
                            uint16_t value, uint16_t index,
                            void *data, uint16_t length, int input)
{
    struct usb_ctl_request control;

    snprintf(dev->operation, sizeof dev->operation,
             "USB request 0x%02x (value 0x%04x, index 0x%04x, %u bytes)",
             request, value, index, length);
    memset(&control, 0, sizeof control);
    control.ucr_data = data;
    control.ucr_request.bmRequestType =
        (uint8_t)(UT_VENDOR | UT_DEVICE | (input ? UT_READ : UT_WRITE));
    control.ucr_request.bRequest = request;
    USETW(control.ucr_request.wValue, value);
    USETW(control.ucr_request.wIndex, index);
    USETW(control.ucr_request.wLength, length);

    if (ioctl(dev->fd, USB_DO_REQUEST, &control) != 0) {
        int saved_errno = errno;

        log_line("  adapter %s failed: %s (%d), bmRequestType 0x%02x, "
                 "actual length %u, request flags 0x%04x",
                 dev->operation, strerror(saved_errno), saved_errno,
                 control.ucr_request.bmRequestType, control.ucr_actlen,
                 control.ucr_flags);
        if (request == MT_REQ_READ_CFG && index == MT_FCE_DMA_ADDR)
            probe_device_descriptor(dev);
        errno = saved_errno;
        return 0;
    }
    if (control.ucr_actlen != length) {
        log_line("  adapter %s returned %u bytes, expected %u",
                 dev->operation, control.ucr_actlen, length);
        errno = EIO;
        return 0;
    }
    return 1;
}

static int control_read_request(adapter_transport *dev, uint8_t request,
                                uint16_t address, uint32_t *value)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };

    if (!control_transfer(dev, request, 0, address,
                         bytes, sizeof bytes, 1)) return 0;
    *value = read_le32(bytes);
    return 1;
}

static int control_read(adapter_transport *dev, uint16_t address,
                        uint32_t *value)
{
    return control_read_request(dev, MT_REQ_MULTI_READ, address, value);
}

static int control_read_cfg(adapter_transport *dev, uint16_t address,
                            uint32_t *value)
{
    return control_read_request(dev, MT_REQ_READ_CFG, address, value);
}

static int control_write_request(adapter_transport *dev, uint8_t request,
                                 uint16_t address, uint32_t value)
{
    unsigned char bytes[4];

    write_le32(bytes, value);
    return control_transfer(dev, request, 0, address,
                            bytes, sizeof bytes, 0);
}

static int control_write(adapter_transport *dev, uint16_t address,
                         uint32_t value)
{
    return control_write_request(dev, MT_REQ_MULTI_WRITE, address, value);
}

static int control_write_cfg(adapter_transport *dev, uint16_t address,
                             uint32_t value)
{
    return control_write_request(dev, MT_REQ_WRITE_CFG, address, value);
}

static int device_mode(adapter_transport *dev, uint16_t address)
{
    return control_transfer(dev, MT_REQ_DEV_MODE, address, 0, NULL, 0, 0);
}

static int wait_register(adapter_transport *dev, uint16_t address,
                         uint32_t mask, uint32_t expected)
{
    int waited;
    uint32_t value = 0;

    for (waited = 0; waited < POLL_TIMEOUT_MS; waited++) {
        if (!control_read_cfg(dev, address, &value)) return 0;
        if ((value & mask) == expected) return 1;
        usleep(POLL_INTERVAL_US);
    }
    snprintf(dev->operation, sizeof dev->operation,
             "register 0x%04x expected masked value 0x%08x "
             "(mask 0x%08x), last read 0x%08x",
             address, expected, mask, value);
    errno = ETIMEDOUT;
    return 0;
}

static int bulk_write(adapter_transport *dev, const unsigned char *data,
                      uint32_t length)
{
    struct usb_fs_start start;
    struct usb_fs_complete complete;
    struct usb_device_stats stats_before, stats_after;
    int have_stats_before, have_stats_after;
    int waited, polls = 0, completions = 0, input_completions = 0;

    if (!dev->input_receives_started) {
        uint8_t indexes[] = {
            XBOX_ADAPTER_EP_IN_CMD_INDEX,
            XBOX_ADAPTER_EP_IN_WLAN_INDEX
        };
        size_t i;

        for (i = 0; i < sizeof indexes / sizeof indexes[0]; i++) {
            memset(&start, 0, sizeof start);
            start.ep_index = indexes[i];
            set_operation(dev, i == 0 ?
                          "starting USB bulk IN endpoint 0x85" :
                          "starting USB bulk IN endpoint 0x84");
            if (ioctl(dev->fd, USB_FS_START, &start) != 0)
                return log_ioctl_failure(dev, "USB_FS_START (bulk IN)");
        }
        dev->input_receives_started = 1;
        log_line("  adapter bulk IN receives queued on endpoints 0x85 and "
                 "0x84");
    }

    have_stats_before = get_device_stats(dev, &stats_before);
    dev->buffer_pointer[XBOX_ADAPTER_EP_OUT_INDEX][0] = (void *)data;
    dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0] = length;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].nFrames = 1;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].aFrames = 0;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status = 0;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].timeout = XFER_TIMEOUT_MS;

    snprintf(dev->operation, sizeof dev->operation,
             "bulk OUT transfer of %u bytes", length);
    memset(&start, 0, sizeof start);
    start.ep_index = XBOX_ADAPTER_EP_OUT_INDEX;
    if (ioctl(dev->fd, USB_FS_START, &start) != 0)
        return log_ioctl_failure(dev, "USB_FS_START");

    for (waited = 0; waited < XFER_TIMEOUT_MS; waited++) {
        polls++;
        memset(&complete, 0, sizeof complete);
        if (ioctl(dev->fd, USB_FS_COMPLETE, &complete) == 0) {
            completions++;
            if (complete.ep_index != XBOX_ADAPTER_EP_OUT_INDEX) {
                int input_index;
                int queued = 1;

                input_completions++;
                if (complete.ep_index == XBOX_ADAPTER_EP_IN_CMD_INDEX)
                    input_index = 0;
                else if (complete.ep_index == XBOX_ADAPTER_EP_IN_WLAN_INDEX)
                    input_index = 1;
                else
                    continue;

                if (dev->endpoints[complete.ep_index].status == 0 &&
                    dev->endpoints[complete.ep_index].aFrames != 0) {
                    queued = queue_adapter_input(
                        dev->input_buffers[input_index],
                        dev->buffer_length[complete.ep_index][0],
                        complete.ep_index);
                    log_line("  adapter USB bulk IN EP 0x%02x received "
                             "%u byte(s)",
                             input_index == 0 ? XBOX_ADAPTER_BULK_IN_CMD :
                                                XBOX_ADAPTER_BULK_IN_WLAN,
                             dev->buffer_length[complete.ep_index][0]);
                } else if (dev->endpoints[complete.ep_index].status != 0) {
                    log_line("  adapter USB bulk IN EP 0x%02x completed "
                             "with status %d (%s)",
                             input_index == 0 ? XBOX_ADAPTER_BULK_IN_CMD :
                                                XBOX_ADAPTER_BULK_IN_WLAN,
                             dev->endpoints[complete.ep_index].status,
                             usb_status_name(
                                 dev->endpoints[complete.ep_index].status));
                }

                dev->buffer_pointer[complete.ep_index][0] =
                    dev->input_buffers[input_index];
                dev->buffer_length[complete.ep_index][0] =
                    sizeof dev->input_buffers[input_index];
                dev->endpoints[complete.ep_index].nFrames = 1;
                dev->endpoints[complete.ep_index].aFrames = 0;
                dev->endpoints[complete.ep_index].status = 0;
                memset(&start, 0, sizeof start);
                start.ep_index = complete.ep_index;
                if (ioctl(dev->fd, USB_FS_START, &start) != 0)
                    return log_ioctl_failure(dev, "USB_FS_START (bulk IN)");
                if (!queued) {
                    errno = ENOBUFS;
                    set_operation(dev, "wireless input queue full");
                    return 0;
                }
                usleep(POLL_INTERVAL_US);
                continue;
            }
            have_stats_after = get_device_stats(dev, &stats_after);
            if (dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status == 0 &&
                dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].aFrames == 1 &&
                dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0] == length) {
                log_line("  adapter USB bulk OUT completed: %u bytes, "
                         "%d ms, %d poll(s), %d completion(s)",
                         dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0],
                         waited + 1, polls,
                         completions);
                if (have_stats_before && have_stats_after) {
                    log_line("  adapter USB bulk endpoint stats (IN+OUT): "
                             "+ok %u, +fail %u",
                             stats_after.uds_requests_ok[UE_BULK] -
                                 stats_before.uds_requests_ok[UE_BULK],
                             stats_after.uds_requests_fail[UE_BULK] -
                                 stats_before.uds_requests_fail[UE_BULK]);
                }
                return 1;
            }
            log_line("  adapter USB bulk OUT completion: requested %u bytes, "
                     "reported frame length %u, status %d (%s), "
                     "completed frames %u, "
                     "polls %d, completions %d, IN completions %d, "
                     "max packet %u, "
                     "max buffer %u",
                     length,
                     dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0],
                     dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status,
                     usb_status_name(
                         dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status),
                     dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].aFrames,
                     polls, completions,
                     input_completions,
                     dev->endpoint_max_packet_length,
                     dev->endpoint_max_bufsize);
            if (have_stats_before && have_stats_after) {
                log_line("  adapter USB bulk endpoint stats (IN+OUT): "
                         "+ok %u, +fail %u",
                         stats_after.uds_requests_ok[UE_BULK] -
                             stats_before.uds_requests_ok[UE_BULK],
                         stats_after.uds_requests_fail[UE_BULK] -
                             stats_before.uds_requests_fail[UE_BULK]);
            } else {
                log_line("  adapter USB_DEVICESTATS unavailable %s transfer",
                         !have_stats_before ? "before" : "after");
            }
            snprintf(dev->operation, sizeof dev->operation,
                     "bulk OUT transfer failed (USB status %d)",
                     dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status);
            errno = EIO;
            return 0;
        }
        if (errno != EBUSY && errno != EWOULDBLOCK &&
            errno != EAGAIN && errno != EINTR)
            return log_ioctl_failure(dev, "USB_FS_COMPLETE");
        usleep(POLL_INTERVAL_US);
    }

    log_line("  adapter USB_FS_COMPLETE timed out after %d ms; "
             "%d poll(s), %d completion(s), %d IN completion(s), "
             "frame status %d (%s), completed frames %u, "
             "reported frame length %u",
             XFER_TIMEOUT_MS, polls, completions, input_completions,
             dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status,
             usb_status_name(
                 dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].status),
             dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].aFrames,
             dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0]);
    errno = ETIMEDOUT;
    return 0;
}

static void close_transport(adapter_transport *dev)
{
    if (dev->fd < 0) return;

    if (dev->fs_initialized) {
        struct usb_fs_uninit uninit;

        memset(&uninit, 0, sizeof uninit);
        if (ioctl(dev->fd, USB_FS_UNINIT, &uninit) != 0)
            log_ioctl_failure(dev, "USB_FS_UNINIT");
    }
    if (dev->interface_claimed) {
        int iface = XBOX_ADAPTER_INTERFACE;

        if (ioctl(dev->fd, USB_RELEASE_INTERFACE, &iface) != 0)
            log_ioctl_failure(dev, "USB_RELEASE_INTERFACE");
    }
    close(dev->fd);
    dev->fd = -1;
}

static int open_transport(adapter_transport *dev, const char *node)
{
    struct usb_fs_init init;
    struct usb_fs_open endpoint;
    int config;

    memset(dev, 0, sizeof *dev);
    dev->fd = -1;
    set_operation(dev, "opening USB device");
    dev->fd = open(node, O_RDWR);
    if (dev->fd < 0) return 0;

    set_operation(dev, "reading active USB configuration");
    if (ioctl(dev->fd, USB_GET_CONFIG, &config) == 0) {
        log_line("  adapter USB active configuration: %d", config);
    } else {
        int saved_errno = errno;

        log_line("  adapter USB_GET_CONFIG unavailable: %s (%d)",
                 strerror(saved_errno), saved_errno);
    }

    {
        int iface = XBOX_ADAPTER_INTERFACE;

        set_operation(dev, "claiming USB interface");
        if (ioctl(dev->fd, USB_CLAIM_INTERFACE, &iface) != 0)
            return log_ioctl_failure(dev, "USB_CLAIM_INTERFACE");
        dev->interface_claimed = 1;
        log_line("  adapter USB interface 0 claimed");
    }

    dev->buffer_pointer[XBOX_ADAPTER_EP_OUT_INDEX][0] = dev->buffer;
    dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX][0] = sizeof dev->buffer;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].ppBuffer =
        dev->buffer_pointer[XBOX_ADAPTER_EP_OUT_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].pLength =
        dev->buffer_length[XBOX_ADAPTER_EP_OUT_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].nFrames = 1;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].flags =
        USB_FS_FLAG_SINGLE_SHORT_OK;
    dev->endpoints[XBOX_ADAPTER_EP_OUT_INDEX].timeout = XFER_TIMEOUT_MS;

    dev->buffer_pointer[XBOX_ADAPTER_EP_IN_CMD_INDEX][0] =
        dev->input_buffers[0];
    dev->buffer_length[XBOX_ADAPTER_EP_IN_CMD_INDEX][0] =
        sizeof dev->input_buffers[0];
    dev->endpoints[XBOX_ADAPTER_EP_IN_CMD_INDEX].ppBuffer =
        dev->buffer_pointer[XBOX_ADAPTER_EP_IN_CMD_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_IN_CMD_INDEX].pLength =
        dev->buffer_length[XBOX_ADAPTER_EP_IN_CMD_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_IN_CMD_INDEX].nFrames = 1;
    dev->endpoints[XBOX_ADAPTER_EP_IN_CMD_INDEX].flags =
        USB_FS_FLAG_SINGLE_SHORT_OK;
    dev->endpoints[XBOX_ADAPTER_EP_IN_CMD_INDEX].timeout =
        USB_FS_TIMEOUT_NONE;

    dev->buffer_pointer[XBOX_ADAPTER_EP_IN_WLAN_INDEX][0] =
        dev->input_buffers[1];
    dev->buffer_length[XBOX_ADAPTER_EP_IN_WLAN_INDEX][0] =
        sizeof dev->input_buffers[1];
    dev->endpoints[XBOX_ADAPTER_EP_IN_WLAN_INDEX].ppBuffer =
        dev->buffer_pointer[XBOX_ADAPTER_EP_IN_WLAN_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_IN_WLAN_INDEX].pLength =
        dev->buffer_length[XBOX_ADAPTER_EP_IN_WLAN_INDEX];
    dev->endpoints[XBOX_ADAPTER_EP_IN_WLAN_INDEX].nFrames = 1;
    dev->endpoints[XBOX_ADAPTER_EP_IN_WLAN_INDEX].flags =
        USB_FS_FLAG_SINGLE_SHORT_OK;
    dev->endpoints[XBOX_ADAPTER_EP_IN_WLAN_INDEX].timeout =
        USB_FS_TIMEOUT_NONE;

    memset(&init, 0, sizeof init);
    init.pEndpoints = dev->endpoints;
    init.ep_index_max = USB_FS_INDEX_COUNT;
    set_operation(dev, "initializing USB transfer");
    if (ioctl(dev->fd, USB_FS_INIT, &init) != 0)
        return log_ioctl_failure(dev, "USB_FS_INIT");
    dev->fs_initialized = 1;

    memset(&endpoint, 0, sizeof endpoint);
    endpoint.max_bufsize = sizeof dev->buffer;
    endpoint.max_frames = 1;
    endpoint.ep_index = XBOX_ADAPTER_EP_OUT_INDEX;
    endpoint.ep_no = XBOX_ADAPTER_BULK_OUT;
    set_operation(dev, "opening USB bulk endpoint 0x04");
    if (ioctl(dev->fd, USB_FS_OPEN, &endpoint) != 0)
        return log_ioctl_failure(dev, "USB_FS_OPEN");
    dev->endpoint_max_packet_length = endpoint.max_packet_length;
    dev->endpoint_max_bufsize = endpoint.max_bufsize;
    log_line("  adapter USB bulk EP 0x04: max packet %u, max buffer %u",
             dev->endpoint_max_packet_length,
             dev->endpoint_max_bufsize);

    memset(&endpoint, 0, sizeof endpoint);
    endpoint.max_bufsize = sizeof dev->input_buffers[0];
    endpoint.max_frames = 1;
    endpoint.ep_index = XBOX_ADAPTER_EP_IN_CMD_INDEX;
    endpoint.ep_no = XBOX_ADAPTER_BULK_IN_CMD;
    set_operation(dev, "opening USB bulk IN endpoint 0x85");
    if (ioctl(dev->fd, USB_FS_OPEN, &endpoint) != 0)
        return log_ioctl_failure(dev, "USB_FS_OPEN");
    log_line("  adapter USB bulk IN EP 0x85: max packet %u, max buffer %u",
             endpoint.max_packet_length, endpoint.max_bufsize);

    memset(&endpoint, 0, sizeof endpoint);
    endpoint.max_bufsize = sizeof dev->input_buffers[1];
    endpoint.max_frames = 1;
    endpoint.ep_index = XBOX_ADAPTER_EP_IN_WLAN_INDEX;
    endpoint.ep_no = XBOX_ADAPTER_BULK_IN_WLAN;
    set_operation(dev, "opening USB bulk IN endpoint 0x84");
    if (ioctl(dev->fd, USB_FS_OPEN, &endpoint) != 0)
        return log_ioctl_failure(dev, "USB_FS_OPEN");
    log_line("  adapter USB bulk IN EP 0x84: max packet %u, max buffer %u",
             endpoint.max_packet_length, endpoint.max_bufsize);

    return 1;
}

static int send_firmware_chunk(adapter_transport *dev, uint32_t address,
                               const unsigned char *data, uint16_t length)
{
    uint32_t transfer_length = ((uint32_t)length + 3u) & ~3u;
    uint32_t packet_length = 4u + transfer_length + 4u;
    uint32_t dma_address, dma_length;

    dev->buffer[0] = (unsigned char)transfer_length;
    dev->buffer[1] = (unsigned char)(transfer_length >> 8);
    dev->buffer[2] = 0;
    dev->buffer[3] = 0x50;
    memcpy(dev->buffer + 4, data, length);
    memset(dev->buffer + 4 + length, 0,
           packet_length - 4u - length);

    log_line("  adapter firmware chunk: address 0x%08x, firmware bytes %u, "
             "DMA bytes %u, USB packet %u",
             address, length, transfer_length, packet_length);
    if (!control_write_cfg(dev, MT_FCE_DMA_ADDR, address) ||
        !control_write_cfg(dev, MT_FCE_DMA_LEN, transfer_length << 16))
        return 0;

    if (!control_read_cfg(dev, MT_FCE_DMA_ADDR, &dma_address) ||
        !control_read_cfg(dev, MT_FCE_DMA_LEN, &dma_length))
        return 0;
    log_line("  adapter DMA registers before bulk OUT: address 0x%08x, "
             "length 0x%08x", dma_address, dma_length);

    if (!bulk_write(dev, dev->buffer, packet_length)) {
        int transfer_errno = errno;
        char transfer_operation[sizeof dev->operation];
        int have_dma_address, have_dma_length;

        memcpy(transfer_operation, dev->operation, sizeof transfer_operation);
        have_dma_address = control_read_cfg(dev, MT_FCE_DMA_ADDR,
                                             &dma_address);
        have_dma_length = control_read_cfg(dev, MT_FCE_DMA_LEN, &dma_length);
        log_line("  adapter DMA registers after failed bulk OUT: "
                 "address %s, length %s",
                 have_dma_address ? "read" : "unavailable",
                 have_dma_length ? "read" : "unavailable");
        if (have_dma_address)
            log_line("    DMA address: 0x%08x", dma_address);
        if (have_dma_length)
            log_line("    DMA length: 0x%08x", dma_length);
        snprintf(dev->operation, sizeof dev->operation, "%s",
                 transfer_operation);
        errno = transfer_errno;
        return 0;
    }

    if (!wait_register(dev, MT_FCE_DMA_LEN, 0xffffffffu,
                       (transfer_length << 16) | MT_DMA_COMPLETE)) {
        log_line("  adapter USB bulk OUT completed but DMA did not: %s",
                 dev->operation);
        return 0;
    }
    log_line("  adapter firmware chunk DMA completed at 0x%08x", address);
    return 1;
}

static int load_firmware(adapter_transport *dev)
{
    const unsigned char *firmware = xbox_wireless_firmware;
    const size_t header_size = 32;
    uint32_t ilm_length, dlm_length, address;
    size_t offset, end;
    uint32_t value;

    if (xbox_wireless_firmware_size < header_size) {
        errno = EINVAL;
        return 0;
    }
    ilm_length = read_le32(firmware);
    dlm_length = read_le32(firmware + 4);
    if ((size_t)ilm_length > xbox_wireless_firmware_size - header_size ||
        (size_t)dlm_length >
            xbox_wireless_firmware_size - header_size - ilm_length) {
        errno = EINVAL;
        return 0;
    }
    log_line("  adapter firmware image: %u bytes, ILM %u bytes, DLM %u bytes, "
             "chunk limit %u",
             (unsigned)xbox_wireless_firmware_size, ilm_length, dlm_length,
             MT_FW_CHUNK_SIZE);

    if (!control_read_cfg(dev, MT_FCE_DMA_ADDR, &value)) {
        int saved_errno = errno;
        char failed_operation[sizeof dev->operation];
        uint32_t probe_value;

        snprintf(failed_operation, sizeof failed_operation, "%s",
                 dev->operation);
        log_line("  adapter probing adjacent read-only register 0x%04x",
                 MT_FCE_DMA_LEN);
        if (control_read_cfg(dev, MT_FCE_DMA_LEN, &probe_value)) {
            log_line("  adapter adjacent register 0x%04x read succeeded: "
                     "0x%08x", MT_FCE_DMA_LEN, probe_value);
        } else {
            log_line("  adapter adjacent register 0x%04x read failed: %s",
                     MT_FCE_DMA_LEN, strerror(errno));
        }
        snprintf(dev->operation, sizeof dev->operation, "%s",
                 failed_operation);
        errno = saved_errno;
        return 0;
    }
    if (value != 0) {
        if (!control_read(dev, MT_RF_PATCH, &value) ||
            !control_write(dev, MT_RF_PATCH, value & ~(1u << 19)) ||
            !device_mode(dev, MT_FW_LOAD_IVB) ||
            !wait_register(dev, MT_FCE_DMA_ADDR, 0x80000000u,
                           0x80000000u))
            return 0;
    }

    if (!control_write_cfg(dev, MT_USB_U3DMA_CFG, 0x00c00000u) ||
        !control_write(dev, MT_FCE_PSE_CTRL, 0x01) ||
        !control_write(dev, MT_TX_CPU_FROM_FCE_BASE_PTR, 0x400230) ||
        !control_write(dev, MT_TX_CPU_FROM_FCE_MAX_COUNT, 0x01) ||
        !control_write(dev, MT_TX_CPU_FROM_FCE_DESC_IDX, 0x01) ||
        !control_write(dev, MT_FCE_PDMA_GLOBAL_CONF, 0x44) ||
        !control_write(dev, MT_FCE_SKIP_FS, 0x03)) return 0;

    offset = header_size;
    end = offset + ilm_length;
    address = MT_MCU_ILM_OFFSET;
    while (offset < end) {
        uint32_t remaining = (uint32_t)(end - offset);
        uint16_t chunk = (uint16_t)(remaining > MT_FW_CHUNK_SIZE ?
                                    MT_FW_CHUNK_SIZE : remaining);

        if (!send_firmware_chunk(dev, address, firmware + offset, chunk))
            return 0;
        offset += chunk;
        address += chunk;
    }

    end = offset + dlm_length;
    address = MT_MCU_DLM_OFFSET;
    while (offset < end) {
        uint32_t remaining = (uint32_t)(end - offset);
        uint16_t chunk = (uint16_t)(remaining > MT_FW_CHUNK_SIZE ?
                                    MT_FW_CHUNK_SIZE : remaining);

        if (!send_firmware_chunk(dev, address, firmware + offset, chunk))
            return 0;
        offset += chunk;
        address += chunk;
    }

    if (!control_write_cfg(dev, MT_FCE_DMA_ADDR, 0) ||
        !device_mode(dev, MT_FW_LOAD_IVB) ||
        !wait_register(dev, MT_FCE_DMA_ADDR, 0x01, 0x01)) return 0;

    return 1;
}

static int send_mcu_payload(adapter_transport *dev, uint8_t command,
                            const unsigned char *payload, size_t payload_length)
{
    unsigned char packet[XBOX_ADAPTER_IN_BUFFER_SIZE] = { 0 };
    uint32_t padded_length, packet_length;

    if (payload_length > sizeof packet - 8u ||
        payload_length > UINT32_MAX) {
        errno = EINVAL;
        return 0;
    }

    padded_length = ((uint32_t)payload_length + 3u) & ~3u;
    packet_length = 4u + padded_length + 4u;
    if (packet_length > sizeof packet) {
        errno = EINVAL;
        return 0;
    }

    write_le32(packet, MT_MCU_MSG_TYPE_CMD | MT_MCU_MSG_PORT_CPU_TX |
                       ((uint32_t)command << 20) | padded_length);
    if (payload_length != 0) memcpy(packet + 4, payload, payload_length);

    snprintf(dev->operation, sizeof dev->operation,
             "MT76 MCU command 0x%02x", command);
    return bulk_write(dev, packet, packet_length);
}

static int send_mcu_command(adapter_transport *dev, uint8_t command,
                            const uint32_t *words, size_t word_count)
{
    unsigned char payload[16];
    size_t i;

    if (word_count > sizeof payload / sizeof(uint32_t)) {
        errno = EINVAL;
        return 0;
    }
    for (i = 0; i < word_count; i++)
        write_le32(payload + i * sizeof(uint32_t), words[i]);
    return send_mcu_payload(dev, command, payload,
                            word_count * sizeof(uint32_t));
}

static int send_ms_command(adapter_transport *dev, uint32_t command,
                           const void *data, size_t length)
{
    unsigned char payload[64] = { 0 };

    if (length > sizeof payload - sizeof(uint32_t)) {
        errno = EINVAL;
        return 0;
    }
    write_le32(payload, command);
    if (length != 0)
        memcpy(payload + sizeof(uint32_t), data, length);
    return send_mcu_payload(dev, MT_MCU_CMD_INIT_GAIN_OP, payload,
                            sizeof(uint32_t) + length);
}

static int send_burst_write(adapter_transport *dev, uint32_t offset,
                            const void *data, size_t length)
{
    unsigned char payload[128] = { 0 };

    if (length > sizeof payload - sizeof(uint32_t)) {
        errno = EINVAL;
        return 0;
    }
    write_le32(payload, MT_MCU_MEMMAP_WLAN + offset);
    if (length != 0)
        memcpy(payload + sizeof(uint32_t), data, length);
    return send_mcu_payload(dev, MT_MCU_CMD_BURST_WRITE, payload,
                            sizeof(uint32_t) + length);
}

static int read_efuse(adapter_transport *dev, uint16_t address,
                      unsigned char *data, size_t length)
{
    uint32_t control, value;
    size_t i, remaining;
    int retry;

    if (!control_read(dev, MT_EFUSE_CTRL, &control)) return 0;
    control &= ~0x03ff00c0u;
    control |= 0x40000000u | ((uint32_t)(address & ~0x0fu) << 16);
    if (!control_write(dev, MT_EFUSE_CTRL, control)) return 0;

    for (retry = 0; retry < 50; retry++) {
        if (!control_read(dev, MT_EFUSE_CTRL, &control)) return 0;
        if ((control & 0x40000000u) == 0) break;
        usleep(10000);
    }
    if (retry == 50) {
        errno = ETIMEDOUT;
        snprintf(dev->operation, sizeof dev->operation,
                 "EFUSE read timeout at 0x%04x", address);
        return 0;
    }

    for (i = 0; i < length; i += sizeof(uint32_t)) {
        uint16_t register_offset =
            (uint16_t)(MT_EFUSE_DATA_BASE + (address & 0x0cu) + i);
        unsigned char bytes[4];

        if (!control_read(dev, register_offset, &value)) return 0;
        write_le32(bytes, value);
        remaining = length - i;
        if (remaining > sizeof bytes) remaining = sizeof bytes;
        memcpy(data + i, bytes, remaining);
    }
    return 1;
}

typedef struct {
    uint16_t address;
    uint32_t value;
} adapter_register_value;

static int initialize_radio_registers(adapter_transport *dev)
{
    static const adapter_register_value values[] = {
        { MT_MAC_SYS_CTRL, 0x03 },
        { MT_USB_DMA_CFG, 0 },
        { MT_MAC_SYS_CTRL, 0 },
        { MT_PWR_PIN_CFG, 0 },
        { 0x0070, 0x6b006464 },
        { MT_WPDMA_GLO_CFG, 0x70 },
        { MT_WMM_AIFSN, 0x2273 },
        { MT_WMM_CWMIN, 0x2344 },
        { MT_WMM_CWMAX, 0x34aa },
        { MT_FCE_DMA_ADDR, 0x041200 },
        { MT_TSO_CTRL, 0 },
        { MT_PBF_SYS_CTRL, 0x080c00 },
        { MT_PBF_TX_MAX_PCNT, 0x1fbf1f1f },
        { MT_FCE_PSE_CTRL, 0x01 },
        { MT_MAC_SYS_CTRL, 0x0c },
        { MT_AUTO_RSP_CFG, 0x13 },
        { MT_MAX_LEN_CFG, 0x3e3fff },
        { MT_AMPDU_MAX_LEN_20M1S, 0xfffc9855 },
        { MT_AMPDU_MAX_LEN_20M2S, 0xff },
        { MT_BKOFF_SLOT_CFG, 0x0109 },
        { MT_PWR_PIN_CFG, 0 },
        { MT_EDCA_CFG_BASE, 0x064320 },
        { MT_EDCA_CFG_BASE + 4, 0x0a4700 },
        { MT_EDCA_CFG_BASE + 8, 0x043238 },
        { MT_EDCA_CFG_BASE + 12, 0x03212f },
        { MT_TX_PIN_CFG, 0x150f0f },
        { MT_TX_SW_CFG0, 0x101001 },
        { MT_TX_SW_CFG1, 0x010000 },
        { MT_TXOP_CTRL_CFG, 0x10583f },
        { MT_TX_TIMEOUT_CFG, 0x0a0f90 },
        { MT_TX_RETRY_CFG, 0x47d01f0f },
        { MT_CCK_PROT_CFG, 0x03f40003 },
        { MT_OFDM_PROT_CFG, 0x03f40003 },
        { MT_MM20_PROT_CFG, 0x01742004 },
        { MT_GF20_PROT_CFG, 0x01742004 },
        { MT_GF40_PROT_CFG, 0x03f42084 },
        { MT_EXP_ACK_TIME, 0x2c00dc },
        { MT_TX_ALC_CFG_2, 0x22160a00 },
        { MT_TX_ALC_CFG_3, 0x22160a76 },
        { MT_TX_ALC_CFG_0, 0x3f3f1818 },
        { MT_TX_ALC_CFG_4, 0x0606 },
        { MT_PIFS_TX_CFG, 0x060fff },
        { MT_RX_FILTR_CFG, 0x017f17 },
        { MT_LEGACY_BASIC_RATE, 0x017f },
        { MT_HT_BASIC_RATE, 0x8003 },
        { MT_PN_PAD_MODE, 0x02 },
        { MT_TXOP_HLDR_ET, 0x02 },
        { MT_TX_PROT_CFG6, 0xe3f42004 },
        { MT_TX_PROT_CFG7, 0xe3f42084 },
        { MT_TX_PROT_CFG8, 0xe3f42104 },
        { MT_DACCLK_EN_DLY_CFG, 0 },
        { MT_RF_PA_MODE_ADJ0, 0xee000000 },
        { MT_RF_PA_MODE_ADJ1, 0xee000000 },
        { MT_TX0_RF_GAIN_CORR, 0x0f3c3c3c },
        { MT_TX1_RF_GAIN_CORR, 0x0f3c3c3c },
        { MT_PBF_CFG, 0x1efebcf5 },
        { MT_PAUSE_ENABLE_CONTROL1, 0x0a },
        { MT_RF_BYPASS_0, 0x7f000000 },
        { MT_RF_SETTING_0, 0x1a800000 },
        { MT_XIFS_TIME_CFG, 0x33a40e0a },
        { MT_FCE_L2_STUFF, 0x03ff0223 },
        { MT_TX_RTS_CFG, 0 },
        { MT_BEACON_TIME_CFG, 0x0640 },
        { MT_EXT_CCA_CFG, 0xf0e4 },
        { MT_CH_TIME_CFG, 0x015f }
    };
    size_t i;

    for (i = 0; i < sizeof values / sizeof values[0]; i++) {
        if (!control_write(dev, values[i].address, values[i].value))
            return 0;
    }
    return 1;
}

static int calibrate_crystal(adapter_transport *dev)
{
    unsigned char trim[4];
    uint16_t trim_value;
    int offset;
    uint32_t control;

    if (!read_efuse(dev, MT_EE_XTAL_TRIM_2, trim, sizeof trim)) return 0;
    trim_value = (uint16_t)(((uint16_t)trim[3] << 8) | trim[2]);
    offset = trim_value & 0x7f;
    if ((trim_value & 0xff) == 0xff)
        offset = 0;
    else if ((trim_value & 0x80) != 0)
        offset = -offset;

    trim_value >>= 8;
    if (trim_value == 0 || trim_value == 0xff) {
        if (!read_efuse(dev, MT_EE_XTAL_TRIM_1, trim, sizeof trim))
            return 0;
        trim_value = (uint16_t)(((uint16_t)trim[3] << 8) | trim[2]);
        trim_value &= 0xff;
        if (trim_value == 0 || trim_value == 0xff) trim_value = 0x14;
    }
    trim_value = (uint16_t)((trim_value & 0x7f) + offset);

    if (!control_read_cfg(dev, MT_XO_CTRL5, &control) ||
        !control_write_cfg(dev, MT_XO_CTRL5,
                           (control & ~0x00007f00u) |
                               ((uint32_t)trim_value << 8)) ||
        !control_write_cfg(dev, MT_XO_CTRL6, 0x00007f00u) ||
        !control_write(dev, MT_CMB_CTRL, 0x0091a7ffu))
        return 0;
    return 1;
}

static int send_calibration(adapter_transport *dev, uint32_t calibration,
                            uint32_t value)
{
    const uint32_t args[] = { calibration, value };
    return send_mcu_command(dev, MT_MCU_CMD_CALIBRATION_OP, args,
                            sizeof args / sizeof args[0]);
}

typedef struct {
    uint8_t index, band, bandwidth, group, scan, power;
} adapter_channel;

static adapter_channel adapter_channels[] = {
    { 0x01, 1, MT_PHY_BW_20, 0, 1, 0 },
    { 0x06, 2, MT_PHY_BW_20, 0, 1, 0 },
    { 0x0b, 3, MT_PHY_BW_20, 0, 1, 0 },
    { 0x24, 1, MT_PHY_BW_40, 1, 1, 0 },
    { 0x28, 1, MT_PHY_BW_40, 1, 0, 0 },
    { 0x2c, 2, MT_PHY_BW_40, 1, 1, 0 },
    { 0x30, 2, MT_PHY_BW_40, 1, 0, 0 },
    { 0x95, 1, MT_PHY_BW_80, 5, 1, 0 },
    { 0x99, 1, MT_PHY_BW_80, 5, 0, 0 },
    { 0x9d, 2, MT_PHY_BW_80, 5, 1, 0 },
    { 0xa1, 2, MT_PHY_BW_80, 5, 0, 0 },
    { 0xa5, 2, MT_PHY_BW_80, 5, 0, 0 }
};

static int channel_power(adapter_transport *dev, adapter_channel *channel)
{
    unsigned char entry[8];
    uint16_t address;
    unsigned target_index, offset;
    int power;

    if (channel->bandwidth == MT_PHY_BW_20) {
        address = MT_EE_TX_POWER_0_START_2G;
        target_index = 4;
    } else {
        address = (uint16_t)(MT_EE_TX_POWER_0_START_5G +
                             channel->group * MT_TX_POWER_GROUP_SIZE_5G);
        target_index = 5;
    }
    if (!read_efuse(dev, address, entry, sizeof entry)) return 0;

    power = entry[target_index];
    offset = entry[target_index + channel->band];
    if ((offset & 0x80u) != 0) {
        int delta = (int)(offset & 0x3fu);
        power += (offset & 0x40u) ? delta : -delta;
    }
    channel->power = (uint8_t)power;
    return 1;
}

static int switch_channel(adapter_transport *dev,
                          const adapter_channel *channel)
{
    unsigned char payload[20] = { 0 };

    payload[0] = channel->index;
    payload[4] = 0x01;
    payload[5] = 0x01;
    payload[16] = channel->bandwidth;
    payload[17] = channel->power;
    payload[18] = channel->scan;
    return send_mcu_payload(dev, MT_MCU_CMD_SWITCH_CHANNEL_OP,
                            payload, sizeof payload);
}

static int set_channel_candidates(adapter_transport *dev, uint8_t best)
{
    unsigned char candidates[12 + sizeof adapter_channels /
                                   sizeof adapter_channels[0] * 4];
    size_t i, count = 0;

    write_le32(candidates, 1);
    write_le32(candidates + 4, best);
    write_le32(candidates + 8, (uint32_t)(sizeof adapter_channels /
                                          sizeof adapter_channels[0] - 1));
    for (i = 0; i < sizeof adapter_channels / sizeof adapter_channels[0]; i++) {
        if (adapter_channels[i].index == best) continue;
        write_le32(candidates + 12 + count * 4, adapter_channels[i].index);
        count++;
    }
    return send_ms_command(dev, MT_MS_SET_CHAN_CANDIDATES, candidates,
                           12 + count * 4);
}

static int initialize_channels(adapter_transport *dev, uint8_t *selected)
{
    size_t i;
    adapter_channel *channel;
    uint32_t args[1];

    if (!control_write(dev, MT_RX_FILTR_CFG, 0x014f13)) return 0;
    for (i = 0; i < sizeof adapter_channels / sizeof adapter_channels[0]; i++) {
        channel = &adapter_channels[i];
        if (!channel_power(dev, channel) || !switch_channel(dev, channel))
            return 0;
    }
    if (!control_write(dev, MT_RX_FILTR_CFG, 0x017f17)) return 0;

    channel = &adapter_channels[sizeof adapter_channels /
                                sizeof adapter_channels[0] - 1];
    channel->scan = 1;
    if (!switch_channel(dev, channel)) return 0;

    args[0] = MT_MCU_RADIO_OFF;
    if (!send_mcu_command(dev, MT_MCU_CMD_POWER_SAVING_OP, args, 1))
        return 0;
    usleep(50000);
    args[0] = MT_MCU_RADIO_ON;
    if (!send_mcu_command(dev, MT_MCU_CMD_POWER_SAVING_OP, args, 1))
        return 0;

    channel->scan = 0;
    if (!switch_channel(dev, channel) ||
        !set_channel_candidates(dev, channel->index))
        return 0;
    *selected = channel->index;
    return 1;
}

static int initialize_address(adapter_transport *dev,
                              unsigned char address[6])
{
    if (!read_efuse(dev, MT_EE_MAC_ADDR, address, 6)) return 0;
    if (address[0] != 0x62) {
        address[0] = 0x62;
        address[1] = 0x45;
        address[2] = 0xbd;
    }
    return send_burst_write(dev, MT_MAC_ADDR_DW0, address, 6) &&
           send_burst_write(dev, MT_MAC_BSSID_DW0, address, 6) &&
           send_ms_command(dev, MT_MS_SET_MAC_ADDRESS, address, 6);
}

static int initialize_beacon(adapter_transport *dev,
                             const unsigned char address[6], int pairing)
{
    unsigned char beacon[MT_MCU_TXWI_SIZE + 36 + 20] = { 0 };
    unsigned char *frame = beacon + MT_MCU_TXWI_SIZE;
    static const unsigned char information_element[20] = {
        0x00, 0x00, 0xdd, 0x10, 0x00, 0x50, 0xf2, 0x11,
        0x01, 0x10, 0x00, 0xa5, 0x30, 0x99, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    /* TXWI followed by the 802.11 beacon frame and vendor IE. */
    beacon[0] = 0x08;             /* timestamp generation */
    beacon[2] = 0x00;
    beacon[3] = 0x20;             /* OFDM */
    beacon[4] = 0x02;             /* no-sequence ACK control */
    beacon[6] = 0x38;             /* 36-byte header + 20-byte IE */
    frame[0] = 0x80;              /* beacon frame control */
    memset(frame + 4, 0xff, 6);
    memcpy(frame + 10, address, 6);
    memcpy(frame + 16, address, 6);
    frame[32] = 100;              /* beacon interval */
    frame[34] = 0x31;
    frame[35] = 0xc6;             /* capability information */
    memcpy(frame + 36, information_element,
           sizeof information_element);
    frame[46] = pairing ? 1 : 0;

    if (!send_burst_write(dev, MT_BEACON_BASE, beacon, sizeof beacon))
        return 0;
    log_line("  adapter %s beacon written (%u bytes)",
             pairing ? "pairing" : "normal", (unsigned)sizeof beacon);
    return control_write(dev, MT_BEACON_TIME_CFG, 0x001f0640u);
}

static int set_led_mode(adapter_transport *dev, uint32_t mode)
{
    return send_mcu_command(dev, MT_MCU_CMD_LED_MODE_OP, &mode, 1);
}

static int set_pairing_mode(adapter_transport *dev, int enabled)
{
    if (adapter_pairing_enabled == enabled) return 1;
    if (!initialize_beacon(dev, adapter_address, enabled) ||
        !set_led_mode(dev, enabled ? 0 : 2))
        return 0;
    adapter_pairing_enabled = enabled;
    return 1;
}

static int send_gip_to_client(adapter_transport *dev, size_t client_index,
                              const unsigned char *message, size_t length);

static int send_gip_command(adapter_transport *dev, uint8_t command,
                            const unsigned char *payload,
                            size_t payload_length, uint8_t wcid)
{
    unsigned char message[7];

    if (payload_length > sizeof message - 4u) {
        errno = EMSGSIZE;
        return 0;
    }
    if (wcid == 0 || wcid > MT_WIFI_MAX_CLIENTS) {
        errno = ENOTCONN;
        return 0;
    }
    message[0] = command;
    message[1] = MT_GIP_OPT_SYSTEM;
    message[2] = adapter_gip_sequence++;
    if (adapter_gip_sequence == 0) adapter_gip_sequence = 1;
    message[3] = (unsigned char)payload_length;
    if (payload_length != 0)
        memcpy(message + 4, payload, payload_length);
    return send_gip_to_client(dev, (size_t)(wcid - 1), message,
                              4u + payload_length);
}

static int send_gip_to_client(adapter_transport *dev, size_t client_index,
                              const unsigned char *message, size_t length)
{
    unsigned char payload[8 + MT_MCU_TXWI_SIZE +
                          MT_WIFI_HEADER_SIZE +
                          MT_WIFI_QOS_CONTROL_SIZE +
                          MT_WIFI_L2_PADDING_SIZE + 512] = { 0 };
    unsigned char client_message[512];
    unsigned char *txwi = payload + 8;
    unsigned char *frame = txwi + MT_MCU_TXWI_SIZE;
    size_t frame_length;
    size_t txwi_frame_length;

    if (message == NULL || length < 4 || length > sizeof client_message ||
        client_index >= MT_WIFI_MAX_CLIENTS) {
        errno = EINVAL;
        return 0;
    }
    if (!adapter_clients[client_index].present ||
        !adapter_clients[client_index].gip_id_valid) {
        errno = ENOTCONN;
        return 0;
    }

    frame_length = MT_WIFI_HEADER_SIZE + MT_WIFI_QOS_CONTROL_SIZE +
                   MT_WIFI_L2_PADDING_SIZE + length;
    txwi_frame_length = MT_WIFI_HEADER_SIZE + MT_WIFI_QOS_CONTROL_SIZE +
                        length;
    payload[2] = 0;
    payload[3] = (uint8_t)client_index;
    txwi[0] = 0x80;
    txwi[2] = 0x00;
    txwi[3] = 0x20;
    txwi[4] = MT_MCU_TXWI_ACK_REQUEST;
    txwi[5] = (uint8_t)client_index;
    txwi[6] = (unsigned char)txwi_frame_length;
    txwi[7] = (unsigned char)(txwi_frame_length >> 8);
    frame[0] = 0x88;
    frame[1] = (unsigned char)(0x02 |
        (adapter_clients[client_index].encryption_enabled ? 0x40 : 0));
    frame[2] = 0x90;
    memcpy(frame + 4, adapter_clients[client_index].address, 6);
    memcpy(frame + 10, adapter_address, 6);
    memcpy(frame + 16, adapter_address, 6);
    memcpy(client_message, message, length);
    client_message[1] = (unsigned char)((client_message[1] & 0xf0u) |
                                        adapter_clients[client_index].gip_id);
    if (client_message[0] == MT_GIP_CMD_ACK && length >= 13 &&
        client_message[3] >= 9) {
        client_message[6] = (unsigned char)(MT_GIP_OPT_SYSTEM |
            adapter_clients[client_index].gip_id);
    }
    if (adapter_gip_tx_debug_count < 16) {
        log_line("  wireless GIP TX: command 0x%02x, options 0x%02x, "
                 "sequence %u, length %u, GIP ID %u, WCID %u",
                 client_message[0], client_message[1], client_message[2],
                 client_message[3], adapter_clients[client_index].gip_id,
                 (unsigned)(client_index + 1));
        adapter_gip_tx_debug_count++;
    }
    memcpy(frame + MT_WIFI_HEADER_SIZE + MT_WIFI_QOS_CONTROL_SIZE +
           MT_WIFI_L2_PADDING_SIZE, client_message, length);
    return send_mcu_payload(dev, 0, payload,
                            8 + MT_MCU_TXWI_SIZE + frame_length);
}

static int send_gip_acknowledgement(adapter_transport *dev, uint8_t wcid,
                                    const unsigned char *message)
{
    unsigned char acknowledgement[4 + 9] = { 0 };
    unsigned char *payload = acknowledgement + 4;

    if (wcid == 0 || wcid > MT_WIFI_MAX_CLIENTS ||
        !adapter_clients[wcid - 1].present) {
        errno = ENOTCONN;
        return 0;
    }

    acknowledgement[0] = MT_GIP_CMD_ACK;
    acknowledgement[1] = MT_GIP_OPT_SYSTEM;
    acknowledgement[2] = message[2];
    acknowledgement[3] = 9;
    payload[1] = message[0];
    payload[2] = MT_GIP_OPT_SYSTEM;
    payload[3] = message[3];

    return send_gip_to_client(dev, (size_t)(wcid - 1), acknowledgement,
                              sizeof acknowledgement);
}

static int initialize_radio(adapter_transport *dev)
{
    const uint32_t select_q[] = { MT_MCU_FUNCTION_Q_SELECT, 1 };
    const uint32_t power_on[] = { MT_MCU_RADIO_ON };
    const uint32_t load_cr[] = { MT_MCU_CR_RF_BBP };
    const uint32_t idle_time[] = { 64 };
    unsigned char address[6];
    uint8_t channel;

    if (!send_mcu_command(dev, MT_MCU_CMD_FUN_SET_OP,
                          select_q, sizeof select_q / sizeof select_q[0]) ||
        !send_mcu_command(dev, MT_MCU_CMD_POWER_SAVING_OP,
                          power_on, sizeof power_on / sizeof power_on[0]) ||
        !send_mcu_command(dev, MT_MCU_CMD_LOAD_CR,
                          load_cr, sizeof load_cr / sizeof load_cr[0]) ||
        !initialize_radio_registers(dev) ||
        !calibrate_crystal(dev) ||
        !initialize_address(dev, address) ||
        !send_ms_command(dev, MT_MS_SET_IDLE_TIME, idle_time,
                         sizeof idle_time) ||
        !control_write(dev, MT_BBP_AGC_BASE + 8 * 4, 0x18365efa) ||
        !control_write(dev, MT_BBP_AGC_BASE + 9 * 4, 0x18365efa) ||
        !control_write(dev, MT_MAC_SYS_CTRL, 0) ||
        !control_write(dev, MT_RF_BYPASS_0, 0) ||
        !control_write(dev, MT_RF_SETTING_0, 0) ||
        !send_calibration(dev, MT_MCU_CAL_TEMP_SENSOR, 0) ||
        !send_calibration(dev, MT_MCU_CAL_RXDCOC, 1) ||
        !send_calibration(dev, MT_MCU_CAL_RC, 0) ||
        !control_write(dev, MT_MAC_SYS_CTRL, 0x0c) ||
        !initialize_channels(dev, &channel))
        return 0;

    memcpy(adapter_address, address, sizeof adapter_address);
    usleep(1000000);
    log_line("  adapter radio setup sent for channel 0x%02x", channel);
    if (!initialize_beacon(dev, address, 0)) return 0;
    return set_led_mode(dev, 2);
}

static int send_wlan_frame(adapter_transport *dev,
                           const unsigned char *address,
                           const unsigned char *frame, size_t frame_length)
{
    unsigned char packet[1024] = { 0 };
    uint32_t payload_length, padded_length;
    size_t packet_length;

    if (frame_length + MT_MCU_TXWI_SIZE > sizeof packet - 8u) {
        errno = EMSGSIZE;
        return 0;
    }
    payload_length = (uint32_t)(MT_MCU_TXWI_SIZE + frame_length);
    padded_length = (payload_length + 3u) & ~3u;
    packet_length = 4u + padded_length + 4u;

    write_le32(packet, MT_MCU_TXD_INFO_WLAN | padded_length);
    packet[4] = 0x80; /* MPDU density */
    packet[6] = 0x00;
    packet[7] = 0x20; /* OFDM */
    packet[8] = MT_MCU_TXWI_ACK_REQUEST;
    packet[9] = 0xff;
    packet[10] = (unsigned char)frame_length;
    packet[11] = (unsigned char)(frame_length >> 8);
    if (address != NULL && frame_length >= MT_WIFI_HEADER_SIZE &&
        memcmp(frame + 4, address, 6) != 0) {
        errno = EINVAL;
        return 0;
    }
    memcpy(packet + 4u + MT_MCU_TXWI_SIZE, frame, frame_length);
    memset(packet + 4 + payload_length, 0,
           packet_length - 4u - payload_length);
    return bulk_write(dev, packet, (uint32_t)packet_length);
}

static int send_client_command(adapter_transport *dev, uint8_t wcid,
                               const unsigned char *address, uint8_t command,
                               const unsigned char *data, size_t data_length)
{
    unsigned char payload[128] = { 0 };
    unsigned char *txwi = payload + 8;
    unsigned char *frame = txwi + MT_MCU_TXWI_SIZE;
    size_t frame_length = MT_WIFI_HEADER_SIZE + 2u + data_length;

    if (wcid == 0 || wcid > MT_WIFI_MAX_CLIENTS ||
        frame_length + 8u + MT_MCU_TXWI_SIZE > sizeof payload) {
        errno = EINVAL;
        return 0;
    }

    payload[3] = (uint8_t)(wcid - 1);
    txwi[0] = 0x80;
    txwi[2] = 0x00;
    txwi[3] = 0x20;
    txwi[4] = MT_MCU_TXWI_ACK_REQUEST;
    txwi[5] = (uint8_t)(wcid - 1);
    txwi[6] = (unsigned char)frame_length;
    txwi[7] = (unsigned char)(frame_length >> 8);

    frame[0] = (unsigned char)MT_WIFI_RESERVED_SUBTYPE;
    memcpy(frame + 4, address, 6);
    memcpy(frame + 10, adapter_address, 6);
    memcpy(frame + 16, adapter_address, 6);
    frame[MT_WIFI_HEADER_SIZE] = (unsigned char)MT_WIFI_RESERVED_SUBTYPE;
    frame[MT_WIFI_HEADER_SIZE + 1] = command;
    if (data_length != 0)
        memcpy(frame + MT_WIFI_HEADER_SIZE + 2, data, data_length);

    return send_mcu_payload(dev, 0, payload,
                            8u + MT_MCU_TXWI_SIZE + frame_length);
}

static int send_pair_response(adapter_transport *dev,
                              const unsigned char *address)
{
    static const unsigned char data[] = {
        0x00, 0x45, 0x55, 0x01, 0x0f, 0x8f, 0xff, 0x87, 0x1f
    };
    unsigned char frame[MT_WIFI_HEADER_SIZE + 2 + sizeof data] = { 0 };

    frame[0] = (unsigned char)MT_WIFI_RESERVED_SUBTYPE;
    memcpy(frame + 4, address, 6);
    memcpy(frame + 10, adapter_address, 6);
    memcpy(frame + 16, adapter_address, 6);
    frame[MT_WIFI_HEADER_SIZE] = (unsigned char)MT_WIFI_RESERVED_SUBTYPE;
    frame[MT_WIFI_HEADER_SIZE + 1] = MT_WIFI_CLIENT_PAIR_RESP;
    memcpy(frame + MT_WIFI_HEADER_SIZE + 2, data, sizeof data);
    return send_wlan_frame(dev, address, frame, sizeof frame);
}

static int send_association_response(adapter_transport *dev, uint8_t wcid,
                                     const unsigned char *address)
{
    unsigned char add_client[] = { 0, 0, 0, 0, 0x40, 0x1f, 0, 0 };
    unsigned char frame[MT_WIFI_HEADER_SIZE + 14] = { 0 };
    unsigned char wcid_address[8] = { 0 };

    if (wcid == 0 || wcid > MT_WIFI_MAX_CLIENTS) return 0;
    frame[0] = 0x10; /* association response */
    memcpy(frame + 4, address, 6);
    memcpy(frame + 10, adapter_address, 6);
    memcpy(frame + 16, adapter_address, 6);
    frame[26] = 0x10;
    frame[27] = 0x01; /* status code 0x0110 */
    frame[28] = 0x00;
    frame[29] = 0x0f; /* association ID */

    memcpy(wcid_address, address, 6);
    add_client[0] = (uint8_t)(wcid - 1);
    if (!send_burst_write(dev, MT_WIFI_WCID_ADDR_BASE + wcid * 8u,
                          wcid_address, 8) ||
        !send_ms_command(dev, MT_WIFI_CLIENT_ADD, add_client,
                         sizeof add_client) ||
        !send_wlan_frame(dev, address, frame, sizeof frame))
        return 0;
    memset(&adapter_clients[wcid - 1], 0,
           sizeof adapter_clients[wcid - 1]);
    memcpy(adapter_clients[wcid - 1].address, address, 6);
    adapter_clients[wcid - 1].present = 1;
    log_line("  wireless controller association response sent (WCID %u)",
             wcid);
    return 1;
}

static int handle_wifi_frame(adapter_transport *dev,
                             const unsigned char *frame, size_t length,
                             uint8_t wcid, uint32_t rxinfo)
{
    uint16_t control;
    size_t header_length = MT_WIFI_HEADER_SIZE;
    const unsigned char *body;
    size_t body_length;

    if (length < MT_WIFI_HEADER_SIZE) return 1;
    control = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);
    if ((control & 0x0300u) == 0x0300u) header_length += 6;
    if ((control & 0x008cu) == 0x0088u) header_length += 2;
    if (header_length > length) return 1;

    body = frame + header_length;
    body_length = length - header_length;
    if ((rxinfo & MT_MCU_RXINFO_L2PAD) != 0) {
        if (body_length < 2) return 1;
        body += 2;
        body_length -= 2;
    }

    if (adapter_wifi_frame_debug_count < 64 &&
        (((control >> 2) & 3u) != 0 || ((control >> 4) & 0x0fu) != 8)) {
        size_t dump_length = body_length < 12 ? body_length : 12;
        char hex[25];
        size_t i;

        for (i = 0; i < dump_length; i++)
            snprintf(hex + i * 2, sizeof hex - i * 2, "%02x", body[i]);
        hex[dump_length * 2] = '\0';
        log_line("  adapter 802.11 RX: FC 0x%04x, type %u subtype %u, "
                 "DA %02x:%02x:%02x:%02x:%02x:%02x "
                 "SA %02x:%02x:%02x:%02x:%02x:%02x, "
                 "body %u bytes %s",
                 control, (control >> 2) & 3u, (control >> 4) & 0x0fu,
                 frame[4], frame[5], frame[6], frame[7], frame[8], frame[9],
                 frame[10], frame[11], frame[12], frame[13], frame[14],
                 frame[15], (unsigned)body_length, hex);
        adapter_wifi_frame_debug_count++;
    }

    if (((control >> 2) & 3u) == 2 &&
        adapter_wifi_data_debug_count < 32) {
        size_t dump_length = body_length < 16 ? body_length : 16;
        char hex[33];
        size_t i;

        for (i = 0; i < dump_length; i++)
            snprintf(hex + i * 2, sizeof hex - i * 2, "%02x", body[i]);
        hex[dump_length * 2] = '\0';
        log_line("  adapter WLAN data: FC 0x%04x protected=%u "
                 "WCID %u body %u byte(s) %s",
                 control, (control & 0x4000u) != 0, wcid,
                 (unsigned)body_length, hex);
        adapter_wifi_data_debug_count++;
    }

    if ((control & 0x00fcu) == 0x0070u || (control & 0x00fcu) == 0) {
        log_line("  adapter WLAN frame: control 0x%04x, WCID %u, "
                 "%u byte(s)", control, wcid, (unsigned)length);
    }

    if ((control & 0x000cu) == 0x0008u &&
        (control & 0x0080u) != 0) {
        if (wcid != 0 && wcid <= MT_WIFI_MAX_CLIENTS &&
            adapter_clients[wcid - 1].present && body_length >= 4 &&
            body[0] == MT_GIP_CMD_ANNOUNCE &&
            (body[1] & MT_GIP_OPT_SYSTEM) != 0) {
            int first_announce =
                !adapter_clients[wcid - 1].gip_announce_seen;

            if (adapter_gip_debug_count < 32) {
                log_line("  adapter GIP RX: announce options 0x%02x "
                         "sequence %u payload %u",
                         body[1], body[2], body[3]);
                adapter_gip_debug_count++;
            }
            adapter_clients[wcid - 1].gip_id = body[1] & 0x0fu;
            adapter_clients[wcid - 1].gip_id_valid = 1;
            if ((body[1] & MT_GIP_OPT_ACK) != 0 &&
                !send_gip_acknowledgement(dev, wcid, body)) {
                log_line("  wireless GIP announce ACK failed: %s",
                         strerror(errno ? errno : EIO));
                return 0;
            }
            if (!send_gip_command(dev, MT_GIP_CMD_IDENTIFY, NULL, 0, wcid)) {
                log_line("  wireless GIP identify request failed: %s",
                         strerror(errno ? errno : EIO));
                return 0;
            }
            if (adapter_pairing_enabled) {
                if (!initialize_beacon(dev, adapter_address, 0) ||
                    !set_led_mode(dev, 1))
                    return 0;
                adapter_pairing_enabled = 0;
            }
            adapter_clients[wcid - 1].gip_announce_seen = 1;
            if (first_announce) {
                log_line("  wireless GIP announce received; GIP ID %u, "
                         "WCID %u; identify request sent",
                         adapter_clients[wcid - 1].gip_id, wcid);
            }
        } else if (wcid != 0 && wcid <= MT_WIFI_MAX_CLIENTS &&
                   adapter_clients[wcid - 1].present &&
                   body_length >= 4 &&
                   body_length <= sizeof gip_message) {
            if (adapter_gip_rx_debug_count < 32) {
                log_line("  adapter GIP RX: command 0x%02x options "
                         "0x%02x sequence %u, frame %u byte(s)",
                         body[0], body[1], body[2], (unsigned)body_length);
                adapter_gip_rx_debug_count++;
            }
            if (body_length <= sizeof gip_message) {
                memcpy(gip_message, body, body_length);
                gip_message_length = body_length;
                gip_message_wcid = wcid;
            }
        }
        return 1;
    }

    if ((control & 0x000cu) != 0) return 1;
    if ((control & 0x00fcu) == 0x0070u &&
        body_length >= 2 &&
        body[0] == (unsigned char)MT_WIFI_RESERVED_SUBTYPE) {
        if (body[1] == MT_WIFI_CLIENT_PAIR_REQ) {
            if (!send_pair_response(dev, frame + 10)) return 0;
            if (!initialize_beacon(dev, adapter_address, 0) ||
                !set_led_mode(dev, 2))
                return 0;
            adapter_pairing_enabled = 0;
            log_line("  wireless controller pairing request handled");
        } else if (body[1] == MT_WIFI_CLIENT_ENABLE_ENCRYPTION &&
                   wcid != 0 && wcid <= MT_WIFI_MAX_CLIENTS &&
                   adapter_clients[wcid - 1].present) {
            static const unsigned char enable[] = { 0, 0 };

            if (!send_client_command(dev, wcid, frame + 10,
                                     MT_WIFI_CLIENT_ENABLE_ENCRYPTION,
                                     enable, sizeof enable))
                return 0;
            adapter_clients[wcid - 1].encryption_enabled = 1;
            log_line("  wireless controller encryption enabled (WCID %u)",
                     wcid);
        }
        return 1;
    }

    if ((control & 0x00fcu) == 0x0000u) {
        size_t i;
        uint8_t client_wcid = 0;

        for (i = 0; i < MT_WIFI_MAX_CLIENTS; i++) {
            if (adapter_clients[i].present &&
                memcmp(adapter_clients[i].address, frame + 10, 6) == 0) {
                client_wcid = (uint8_t)(i + 1);
                break;
            }
            if (!adapter_clients[i].present && client_wcid == 0)
                client_wcid = (uint8_t)(i + 1);
        }
        if (client_wcid == 0) return 1;
        log_line("  wireless association request from %02x:%02x:%02x:%02x:"
                 "%02x:%02x",
                 frame[10], frame[11], frame[12], frame[13], frame[14],
                 frame[15]);
        return send_association_response(dev, client_wcid, frame + 10);
    }

    return 1;
}

static int process_adapter_message(adapter_transport *dev,
                                   const unsigned char *packet, size_t length)
{
    uint32_t info, port, event, sequence, rxinfo, control;
    const unsigned char *payload;
    size_t payload_length;
    uint8_t wcid;
    uint16_t mpdu_length;
    size_t frame_length;

    if (length < 8) return 1;
    info = read_le32(packet);
    port = (info >> 27) & 7u;
    sequence = (info >> 16) & 0x0fu;
    payload = packet + 4;
    payload_length = length - 8;
    event = (info >> 20) & 0x0fu;

    /* Sequence 1 marks MCU command responses, not received WLAN frames. */
    if (sequence == 1) return 1;

    if (port == 1) {
        if (event == MT_MCU_EVT_BUTTON) {
            if (!set_pairing_mode(dev, 1)) return 0;
            log_line("  adapter pairing button event handled");
            return 1;
        }
        if (event == MT_MCU_EVT_CLIENT_LOST) {
            if (payload_length >= 1 && payload[0] > 0 &&
                payload[0] <= MT_WIFI_MAX_CLIENTS) {
                size_t i;
                int any_client = 0;

                memset(&adapter_clients[payload[0] - 1], 0,
                       sizeof adapter_clients[0]);
                log_line("  wireless controller lost (WCID %u)",
                         payload[0]);
                for (i = 0; i < MT_WIFI_MAX_CLIENTS; i++) {
                    if (adapter_clients[i].present) {
                        any_client = 1;
                        break;
                    }
                }
                if (!any_client) {
                    if (!set_pairing_mode(dev, 1)) return 0;
                    log_line("  adapter returned to pairing mode after "
                             "controller loss");
                }
            }
            return 1;
        }
        if (event != MT_MCU_EVT_PACKET_RX) return 1;
    } else if (port != 0) {
        return 1;
    }

    if (payload_length < MT_MCU_RXWI_SIZE) return 1;

    rxinfo = read_le32(payload);
    control = read_le32(payload + 4);
    wcid = (uint8_t)(control & 0xffu);
    mpdu_length = (uint16_t)((control >> 16) & 0x3fffu);
    payload += MT_MCU_RXWI_SIZE;
    payload_length -= MT_MCU_RXWI_SIZE;
    frame_length = (size_t)mpdu_length +
                   ((rxinfo & MT_MCU_RXINFO_L2PAD) != 0 ? 2u : 0u);
    if (frame_length < payload_length) payload_length = frame_length;
    log_line("  adapter WLAN RX packet: port %u, event %u, sequence %u, "
             "WCID %u, frame %u byte(s)",
             port, event, sequence, wcid, (unsigned)payload_length);
    return handle_wifi_frame(dev, payload, payload_length, wcid, rxinfo);
}

static int adapter_requeue_input(adapter_transport *dev, int index)
{
    struct usb_fs_start start;
    int input_index = index == XBOX_ADAPTER_EP_IN_CMD_INDEX ? 0 : 1;

    dev->buffer_pointer[index][0] = dev->input_buffers[input_index];
    dev->buffer_length[index][0] = sizeof dev->input_buffers[input_index];
    dev->endpoints[index].nFrames = 1;
    dev->endpoints[index].aFrames = 0;
    dev->endpoints[index].status = 0;
    memset(&start, 0, sizeof start);
    start.ep_index = (uint8_t)index;
    if (ioctl(dev->fd, USB_FS_START, &start) != 0)
        return log_ioctl_failure(dev, "USB_FS_START (bulk IN)");
    return 1;
}
#endif

int xbox_adapter_open(const char *node, char *why, size_t whylen)
{
#ifdef HAS_XBOX_WIRELESS_FIRMWARE
    int saved_errno;

    memset(adapter_clients, 0, sizeof adapter_clients);
    adapter_pairing_enabled = 0;
    adapter_input_head = adapter_input_tail = adapter_input_count = 0;
    memset(adapter_input_debug_count, 0,
           sizeof adapter_input_debug_count);
    adapter_wifi_frame_debug_count = 0;
    adapter_wifi_data_debug_count = 0;
    adapter_gip_debug_count = 0;
    adapter_gip_sequence = 1;
    gip_message_length = 0;
    gip_message_wcid = 0;
    if (!open_transport(&adapter, node)) {
        saved_errno = errno;
        if (saved_errno == 0) saved_errno = EIO;
        snprintf(why, whylen, "could not configure adapter USB transport at %s: %s",
                 adapter.operation, strerror(saved_errno));
        close_transport(&adapter);
        adapter_input_head = adapter_input_tail = adapter_input_count = 0;
        memset(adapter_input_debug_count, 0,
               sizeof adapter_input_debug_count);
        adapter_wifi_frame_debug_count = 0;
        adapter_gip_debug_count = 0;
        adapter_gip_rx_debug_count = 0;
        adapter_gip_tx_debug_count = 0;
        adapter_gip_sequence = 1;
        return 0;
    }

    if (load_firmware(&adapter) && initialize_radio(&adapter)) {
        snprintf(why, whylen,
                 "adapter firmware and radio started; waiting for wireless "
                 "controller pairing");
        return 1;
    } else {
        saved_errno = errno;
        if (saved_errno == 0) saved_errno = EIO;
        snprintf(why, whylen, "adapter initialization failed at %s: %s",
                 adapter.operation, strerror(saved_errno));
    }

    close_transport(&adapter);
    return 0;
#else
    (void)node;
    snprintf(why, whylen, "adapter firmware is not available in this build");
    return 0;
#endif
}

void xbox_adapter_close(void)
{
#ifdef HAS_XBOX_WIRELESS_FIRMWARE
    close_transport(&adapter);
    memset(adapter_clients, 0, sizeof adapter_clients);
    gip_message_length = 0;
    gip_message_wcid = 0;
    adapter_input_head = adapter_input_tail = adapter_input_count = 0;
    memset(adapter_input_debug_count, 0,
           sizeof adapter_input_debug_count);
    adapter_wifi_frame_debug_count = 0;
    adapter_wifi_data_debug_count = 0;
    adapter_gip_debug_count = 0;
    adapter_gip_rx_debug_count = 0;
    adapter_gip_tx_debug_count = 0;
    adapter_gip_sequence = 1;
#endif
}

int xbox_adapter_read(const unsigned char **message)
{
#ifdef HAS_XBOX_WIRELESS_FIRMWARE
    struct usb_fs_complete complete;
    int index, result = 0;
    unsigned queue_index;

    gip_message_length = 0;
    gip_message_wcid = 0;
    if (adapter_input_count != 0) {
        queue_index = adapter_input_head;
        adapter_input_head = (adapter_input_head + 1) %
                             MT_WIFI_INPUT_QUEUE_SIZE;
        adapter_input_count--;
        if (!process_adapter_message(
                &adapter, adapter_input_queue[queue_index],
                adapter_input_lengths[queue_index])) {
            int saved_errno = errno;
            log_line("  adapter wireless request processing failed: %s",
                     strerror(saved_errno ? saved_errno : EIO));
            errno = saved_errno;
            return -1;
        }
        if (gip_message_length != 0) {
            *message = gip_message;
            return (int)gip_message_length;
        }
        return 0;
    }

    memset(&complete, 0, sizeof complete);
    if (ioctl(adapter.fd, USB_FS_COMPLETE, &complete) != 0) {
        if (errno == EBUSY || errno == EWOULDBLOCK || errno == EAGAIN ||
            errno == EINTR)
            return 0;
        log_ioctl_failure(&adapter, "USB_FS_COMPLETE");
        return -1;
    }
    index = (int)complete.ep_index;
    if (index != XBOX_ADAPTER_EP_IN_CMD_INDEX &&
        index != XBOX_ADAPTER_EP_IN_WLAN_INDEX)
        return 0;

    if (adapter.endpoints[index].status == 0 &&
        adapter.endpoints[index].aFrames != 0) {
        log_line("  adapter USB bulk IN EP 0x%02x received %u byte(s)",
                 index == XBOX_ADAPTER_EP_IN_CMD_INDEX ?
                     XBOX_ADAPTER_BULK_IN_CMD : XBOX_ADAPTER_BULK_IN_WLAN,
                 adapter.buffer_length[index][0]);
        result = queue_adapter_input(
            adapter.input_buffers[
                index == XBOX_ADAPTER_EP_IN_CMD_INDEX ? 0 : 1],
            adapter.buffer_length[index][0], index);
    } else if (adapter.endpoints[index].status != 0) {
        log_line("  adapter bulk IN endpoint 0x%02x status %d (%s)",
                 index == XBOX_ADAPTER_EP_IN_CMD_INDEX ?
                     XBOX_ADAPTER_BULK_IN_CMD : XBOX_ADAPTER_BULK_IN_WLAN,
                 adapter.endpoints[index].status,
                 usb_status_name(adapter.endpoints[index].status));
    }

    if (!adapter_requeue_input(&adapter, index)) return -1;
    return result ? 0 : -1;
#else
    (void)message;
    errno = ENOTSUP;
    return -1;
#endif
}

int xbox_adapter_controller_connected(void)
{
#ifdef HAS_XBOX_WIRELESS_FIRMWARE
    size_t i;

    for (i = 0; i < MT_WIFI_MAX_CLIENTS; i++) {
        if (adapter_clients[i].present &&
            adapter_clients[i].gip_announce_seen)
            return 1;
    }
#endif
    return 0;
}

int xbox_adapter_send_gip(const unsigned char *message, size_t length)
{
#ifdef HAS_XBOX_WIRELESS_FIRMWARE
    size_t i;

    if (message == NULL || length < 4 || length > 512) {
        errno = EINVAL;
        return 0;
    }
    if (gip_message_wcid != 0 &&
        gip_message_wcid <= MT_WIFI_MAX_CLIENTS &&
        adapter_clients[gip_message_wcid - 1].present &&
        adapter_clients[gip_message_wcid - 1].gip_id_valid)
        return send_gip_to_client(&adapter, gip_message_wcid - 1,
                                  message, length);
    for (i = 0; i < MT_WIFI_MAX_CLIENTS; i++) {
        if (adapter_clients[i].present && adapter_clients[i].gip_id_valid)
            return send_gip_to_client(&adapter, i, message, length);
    }
    errno = ENOTCONN;
    return 0;
#else
    (void)message;
    (void)length;
    errno = ENOTSUP;
    return 0;
#endif
}
