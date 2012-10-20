
UPLOAD_CMD := avrdude -q -V -p atmega328p -c arduino -b 57600 -P /dev/ttyUSB0


.PHONY:	all


all:
ifndef FIRMWARE
	@echo -e "Please run as:\n\tmake FIRMWARE=bGeigieMini-1.3.9_001111.hex"
else
	$(UPLOAD_CMD) -U flash:w:${FIRMWARE}:i
endif

list:
	@ls -las bGeigieMini*.hex
