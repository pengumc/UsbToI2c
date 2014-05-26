#author: michiel van der Coelen
#2014
#compiling a commandline testing utility for UsbToI2C (UsbMaster)

NAME = sendcmd
EXT = .exe
CC = gcc
CFLAGS = -I. -Wall -Wconversion
LIBS = -L. -lhidapi
OBJECTS = $(NAME).o
#`pkg-config libusb-1.0 --cflags`
#`pkg-config libusb-1.0 --libs`
ifeq ($(shell uname), Linux)
	CFLAGS = -I. -Wall -Wconversion 
	LIBS = -L. -lhidapi-libusb -lusb-1.0
	EXT = 
endif

all:$(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(NAME)$(EXT)

%.o:%.c
	$(CC) $(CFLAGS) $< -c  -o $@

clean:
	rm $(OBJECTS)
