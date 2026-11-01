PS5_HOST ?= 192.168.69.44
PORT     ?= 9021
TARGET   := ghost-control-ps5.elf
.DEFAULT_GOAL := all

ifndef PS5_PAYLOAD_SDK
$(error PS5_PAYLOAD_SDK is not set)
endif

include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

CFLAGS  += -D__PROSPERO__ -Wall -Wextra -g -O2 -fPIC -fno-stack-protector -I.
LDFLAGS += -lScePad -lSceUserService -lSceSystemService -lpthread -ldl

SRC := gc_main.c shellui_pad.c usb_helpers.c controller_nintendo.c controller_xbox.c xbox_adapter.c controller_steam.c controller_sc2.c sc2_profile.c game_list.c webui.c sc2_haptics.c sc2_menu.c sc2_binding.c sc2_gyro.c sc2_flick.c sc2_trigfx.c ds_handoff.c bridge_probe.c bridge/wireless_ds4.c game_hooks.c
XBOX_ADAPTER_FIRMWARE := ../FW_ACC_00U.bin
XBOX_ADAPTER_FIRMWARE_C := .build/xbox_adapter_firmware.c

ifneq ($(wildcard $(XBOX_ADAPTER_FIRMWARE)),)
CFLAGS += -DHAS_XBOX_WIRELESS_FIRMWARE
SRC += $(XBOX_ADAPTER_FIRMWARE_C)

$(XBOX_ADAPTER_FIRMWARE_C): $(XBOX_ADAPTER_FIRMWARE) embed_xbox_firmware.py
	python3 embed_xbox_firmware.py $(XBOX_ADAPTER_FIRMWARE) $@
endif

.PHONY: all clean deploy

all: $(TARGET)

webui_html.h: web/index.html gen_html.py
	python3 gen_html.py

$(TARGET): $(SRC) webui_html.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -rf .build

deploy: $(TARGET)
	nc -w 8 $(PS5_HOST) $(PORT) < $(TARGET)
