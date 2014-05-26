UsbToI2C
========
avr usb device to control the I2C servocontroller.

building
--------
- Makefile in root is for the firmware. 
- to build the commandline utility "sendcmd":
 1. `cd src/cmdline`
 2. `make`

 syntax: `sendcmd r|w [cmd] [byte0  byte1 ... byte11]`
 
 cmd refers to one of the custom_rq in src/requests.h
 
leds:
-----
- red (PB0): on when data waiting to be read.
- green (PD7): on when I2C bus is being used.


