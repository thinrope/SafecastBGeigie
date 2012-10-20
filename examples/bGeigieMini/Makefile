# for the arduinoChibi we need this
USER_LIB_PATH = ../../libraries

# hardware settings
BOARD_TAG = pro328
ARDUINO_PORT = /dev/ttyUSB0

# firmware settings
MONITOR_BAUDRATE = 57600
ARDUINO_LIBS = bGeigie chibi SD SD/utility EEPROM

# default setting for kalin@Gentoo
MONITOR_CMD = screen
ARDUINO_DIR = /usr/share/arduino
ARDMK_DIR = $(HOME)/sketchbook/NetRad-THS/tools/Arduino-Makefile
AVR_TOOLS_DIR = /usr
CFLAGS= -g
include ${ARDMK_DIR}/arduino-mk/Arduino.mk
