# compile time ifdefs
VERSION := $(shell cat VERSION)
BUILD_DATE ?= $(shell date +%F)
ENABLE_DIAGNOSTIC ?= 0
PLUSSHIELD ?= 0
JAPAN_POST ?= 0
TX_ENABLED ?= 1
GPS_PROGRAMMING ?= 1
# FIXME: UGLY CODE GPS_MTK=1 GPS_CANMORE=2
GPS_TYPE ?= 1
VERSION_VERBOSE := $(VERSION) [ _BUILD_DATE=$(BUILD_DATE) ENABLE_DIAGNOSTIC=$(ENABLE_DIAGNOSTIC) PLUSSHIELD=$(PLUSSHIELD) JAPAN_POST=$(JAPAN_POST) TX_ENABLED=$(TX_ENABLED) GPS_PROGRAMMING=$(GPS_PROGRAMMING) GPS_TYPE=$(GPS_TYPE) ]
RELEASE_NAME := bGeigieMini-$(VERSION)_$(ENABLE_DIAGNOSTIC)$(PLUSSHIELD)$(JAPAN_POST)$(TX_ENABLED)$(GPS_PROGRAMMING)$(GPS_TYPE)

CFLAGS := $(CFLAGS) \
	-D'__VERSION="$(VERSION)"' \
	-D'__BUILD_DATE="$(BUILD_DATE)"' \
	-D'ENABLE_DIAGNOSTIC=$(ENABLE_DIAGNOSTIC)' \
	-D'PLUSSHIELD=$(PLUSSHIELD)' \
	-D'JAPAN_POST=$(JAPAN_POST)' \
	-D'TX_ENABLED=$(TX_ENABLED)' \
	-D'GPS_PROGRAMMING=$(GPS_PROGRAMMING)' \
	-D'GPS_TYPE=$(GPS_TYPE)' \
	-D'__RELEASE_NAME="$(RELEASE_NAME)"' \
	-D'__VERSION_VERBOSE="$(VERSION_VERBOSE)"'

CXXFLAGS := $(CFLAGS)

# for the arduinoChibi we need this
USER_LIB_PATH = ../../libraries

# hardware settings
BOARD_TAG = pro328
ARDUINO_PORT = /dev/ttyUSB0

# firmware settings
MONITOR_BAUDRATE = 9600
ARDUINO_LIBS = bGeigie chibi SD SD/utility EEPROM

# default setting for kalin@Gentoo
MONITOR_CMD = screen
ARDUINO_DIR = /usr/share/arduino
ARDMK_DIR = ../../tools/Arduino-Makefile
AVR_TOOLS_DIR = /usr
include ${ARDMK_DIR}/arduino-mk/Arduino.mk

release: $(OBJDIR)/bGeigieMini.hex
ifdef	RELEASE_DIR
	cp $(OBJDIR)/bGeigieMini.hex $(RELEASE_DIR)/$(RELEASE_NAME).hex
else
	$(error RELEASE_DIR is not defined, may be you need to use make from the parent directory?)
endif

