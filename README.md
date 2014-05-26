UsbToI2C
========
avr usb device to control the I2C servocontroller.

building
--------
- Makefile in root is for the firmware. 
- to build the commandline utility "sencmd":
 1 cd src/cmdline
 2 make
 
leds:
-----
red (PB0): on when data waiting to be read.
green: on when I2C bus is being used.


