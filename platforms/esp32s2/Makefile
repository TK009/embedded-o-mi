
CHIP:=esp32
BOARD:=esp32s2
ESP_ROOT:=$(HOME)/esp/arduino-esp32

# use: make list_flash_defs
#FLASH_DEF

#UPLOAD_PORT?=
#UPLOAD_SPEED
#MONITOR_SPEED
#USE_CCACHE=1
#BUILD_THREADS=1
#NO_USER_OBJ_LIB=1

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
				--jerry-cmdline=OFF --jerry-port-default=OFF --lto=OFF --strip=OFF
PATH:=$(PATH):$(ESP_ROOT)/tools/xtensa-esp32s2-elf/bin
$(ESP32S2JS):
ifeq (,$(wildcard $(word 1,$(ESP32S2JS))))
	@echo MAKE JERRY SCRIPT $(ESP32S2JS)
	cd $(JERRY_DIR); $(PY3) tools/build.py --builddir=$(JERRY_BUILDDDIR) $(JERRY_ESP32_OPTIONS) --external-context=ON --profile=minimal
endif

# parallel build fix (the first depends on the second)
$(word 1,$(ESP32S2JS)): $(word 2,$(ESP32S2JS))

LIBS := $(ESP32S2JS) $(SRCS:%=$(EOMI_ROOT)/%) $(EOMI_ROOT)/$(OBJDIR)/OmiConstants.c \
	$(JERRY_DIR)/jerry-core/include/ $(JERRY_DIR)/jerry-ext/include/ $(JERRY_DIR)/targets/esp-idf/*.c
	#$(EOMI_ROOT)/$(OBJDIR)/OmiConstants.h

include ./makeEspArduino/makeEspArduino.mk 

