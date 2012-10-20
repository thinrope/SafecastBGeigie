RELEASE_DIR := $(CURDIR)/../releases
BUILD_DATE := $(shell date +%F)

DEFAULT_ARGS := BUILD_DATE=$(BUILD_DATE) RELEASE_DIR=$(RELEASE_DIR)


UPLOAD_CMD := avrdude -q -V -p atmega328p -c arduino -b 57600 -P /dev/ttyUSB0

SUBDIRS := bGeigieMini

.PHONY:	all $(SUBDIRS)

all: bGeigieMini

flash:
ifndef FIRMWARE
	@echo -e "Please run as:\n\tmake flash FIRMWARE=$(RELEASE_DIR)/bGeigieMini-1.3.9_001111.hex"
else
	$(UPLOAD_CMD) -U flash:w:${FIRMWARE}:i
endif


JP:
	$(MAKE) --directory=bGeigieMini $(DEFAULT_ARGS) ENABLE_DIAGNOSTIC=0 PLUSSHIELD=0 JAPAN_POST=1 TX_ENABLED=1 GPS_PROGRAMMING=1

$(SUBDIRS):
	$(MAKE) --directory=$@ $(DEFAULT_ARGS)

clean size release upload:
	for d in $(SUBDIRS); do $(MAKE) --directory=$$d $@ $(DEFAULT_ARGS); done

distclean: clean
	for d in $(SUBDIRS); do $(MAKE) --directory=$$d $@ $(DEFAULT_ARGS); done

