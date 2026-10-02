DEBUG_FLAGS = -D__FINAL__=1
LOG_TYPE = -D__USE_PRINTF__
BUILD_TYPE = _final

ifeq ($(DEBUG),1)
    DEBUG_FLAGS = -D__FINAL__=0
    BUILD_TYPE = _debug
endif

TYPE := $(BUILD_TYPE)
BUILD_FOLDER := $(shell pwd)/../../bin/plugins
OUTPUT_PRX := bluehen
TARGET := $(BUILD_FOLDER)/prx$(TYPE)/$(OUTPUT_PRX)
TARGET_ELF := $(BUILD_FOLDER)/elf$(TYPE)/$(OUTPUT_PRX)
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
GH_SDK := $(GOLDHEN_SDK)
INTDIR := build
COMMON_DIR := ../../common

CC := clang
LD := ld.lld
FINAL := $(DEBUG_FLAGS)
CPPFLAGS := $(FINAL) $(LOG_TYPE) -I$(GH_SDK)/include -I$(COMMON_DIR)
CFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -Wall -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include $(CPPFLAGS)
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x -e _init --eh-frame-hdr -L$(TOOLCHAIN)/lib -L$(GH_SDK) -lSceLibcInternal -lGoldHEN_Hook -lkernel -lSceSysmodule -lScePad

$(TARGET): $(INTDIR)/bt_audio_plugin.o $(INTDIR)/plugin_common.o
	mkdir -p $(dir $(TARGET)) $(dir $(TARGET_ELF))
	$(LD) $(GH_SDK)/build/crtprx.o $(INTDIR)/*.o -o $(TARGET_ELF).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/linux/create-fself -in=$(TARGET_ELF).elf -out=$(TARGET_ELF).oelf --lib=$(TARGET).prx --paid 0x3800000000000011

$(INTDIR)/bt_audio_plugin.o: bt_audio_plugin.c
	mkdir -p $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/plugin_common.o: $(COMMON_DIR)/plugin_common.c
	mkdir -p $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean
.DEFAULT_GOAL := all

all: $(TARGET)

clean:
	rm -rf $(INTDIR) $(TARGET) $(TARGET_ELF).elf $(TARGET_ELF).oelf
