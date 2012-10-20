### Safecast bGeigie Library

This library implements the basic blocks used in the bGeigie system from Safecast.

Following classes are included (see libraries/bGeigie):

* GPS: Simple class reading geo-location data received from a GPS module connected through the Hardware Serial port.
* HardwareCounter: Uses the Timer1 of the 328p as a hardware counter to record the number of pulses in a given time interval.
* InterruptCounter: A small library using the hardware interrupt as an event counter.

### Examples

The sketches given in examples are the actuall firmware of the different Safecast bGeigie devices:

* bGeigieMini
* bGeigieClassic
* bGeigieNinja
* bGeigieConfigBurner
* SlidingWindowCounter

### Usage

NOTE: For now only bGeigieMini can be built

mkdir Safecast ; cd Safecast
git clone --recursive https://github.com/thinrope/SafecastBGeigie.git
cd SafecastBGeigie
make size upload monitor


To upload a binary, use `make flash` and follow the instructions.
