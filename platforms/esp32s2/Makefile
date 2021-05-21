
CHIP:=esp32
BOARD:=esp32s2
ESP_ROOT:=$(HOME)/esp/arduino-esp32

# use: make list_flash_defs
#FLASH_DEF
FLASH_MODE=dio
# Saola, WROOM

BUILD_EXTRA_FLAGS := \
	-DWIFI_PASS='$(WIFI_PASS)' -DWIFI_SSID='$(WIFI_SSID)' \
	-DBOARD_HAS_PSRAM \
  -DTIMEZONE_SECS=7200 -DTIMEZONE_DST_SECS=3600 \
	-DNTP_SERVER0="0.fi.pool.ntp.org" -DNTP_SERVER1="1.fi.pool.ntp.org" -DNTP_SERVER2="2.fi.pool.ntp.org" \
	-DServoPin=16


UPLOAD_PORT?=$(shell find /dev -name 'ttyUSB*' 2>&- | sort | tail -n 1)
#UPLOAD_SPEED
USE_CCACHE=1

# DEBUGGING
#BUILD_THREADS=1
#NO_USER_OBJ_LIB=1
CORE_DEBUG_LEVEL=4





EOMI_ROOT:=../..
OBJDIR?=obj
SRCS?=src

#jerryscript for esp32s2
JERRY_DIR := $(EOMI_ROOT)/jerryscript
JERRY_BUILDDDIR := build/esp32s2
ESP32S2JS := $(JERRY_DIR)/$(JERRY_BUILDDDIR)/lib/libjerry-core.a $(JERRY_DIR)/$(JERRY_BUILDDDIR)/lib/libjerry-ext.a
USER_SRC_PATTERN := |libjerry-core.a|libjerry-ext.a

JERRY_TOOLCHAIN := ../platforms/esp32s2/jerryscript-toolchain-esp32s2.cmake
#JERRY_TOOLCHAIN := cmake/toolchain-esp32.cmake
JERRY_ESP32_OPTIONS := --toolchain=$(JERRY_TOOLCHAIN) --cmake-param "-GUnix Makefiles" \
				--jerry-cmdline=OFF --jerry-port-default=OFF --lto=OFF --strip=OFF --external-context=ON \
				--profile=$(abspath jerryscript.profile)

PATH:=$(PATH):$(ESP_ROOT)/tools/xtensa-esp32s2-elf/bin

$(ESP32S2JS):
ifeq (,$(wildcard $(word 1,$(ESP32S2JS))))
	@echo MAKE JERRY SCRIPT $(ESP32S2JS)
	@cd $(JERRY_DIR); $(PY3) tools/build.py --builddir=$(JERRY_BUILDDDIR) $(JERRY_ESP32_OPTIONS)
endif

# parallel build fix (the first depends on the second)
$(word 1,$(ESP32S2JS)): $(word 2,$(ESP32S2JS))

LIBS := $(ESP32S2JS) \
	$(SRCS:%=$(EOMI_ROOT)/%) \
	$(EOMI_ROOT)/$(OBJDIR)/OmiConstants.c \
	$(JERRY_DIR)/jerry-core/include/ \
	$(JERRY_DIR)/jerry-ext/include/ \
	$(JERRY_DIR)/targets/esp-idf/*.c \
	AsyncTCP \
	ESPAsyncWebServer \
	Adafruit_NeoPixel/ \
	ESP32-ESP32S2-AnalogWrite/ \

	#DFRobot_SHT20 \


include ./makeEspArduino/makeEspArduino.mk 

