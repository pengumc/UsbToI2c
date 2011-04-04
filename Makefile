# Name: Makefile
# Author: Michiel van der Coelen
# date: 2011-04-01
# tabsize: 2
 
MMCU = atmega88
AVRDUDE_MCU = m88
AVRDUDE_PROGRAMMERID = usbasp
#AVRDUDE_PORT = 
F_CPU = 20000000
NAME = UsbMaster

#for the love of god, keep the linking order!
OBJECTS = usbdrv.o usbdrvasm.o oddebug.o $(NAME).o
CFLAGS = -DF_CPU=$(F_CPU) -std=c99 -Wall -Os -mmcu=$(MMCU) -Ilib/usbdrv -I. -Isrc/
CC = avr-gcc
SIZE = avr-size
OBJCOPY = avr-objcopy

vpath %.c ./lib/usbdrv ./src/firmware
vpath %.h ./lib/usbdrv 
vpath %.S ./lib/usbdrv 

.PHONY: all clean test
all: bin\$(NAME).hex
	$(SIZE) bin\$(NAME).hex

#rebuild everything!
force: clean all

bin\$(NAME).hex: $(NAME).elf
	$(OBJCOPY) -O ihex $(NAME).elf bin\$(NAME).hex
	#rm -f $(OBJECTS) $(NAME).elf
	
$(NAME).elf: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(NAME).elf $(OBJECTS)

#compile src files
%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@


clean:
	rm -f $(OBJECTS) $(NAME).elf

program: bin\$(NAME).hex
	avrdude -c $(AVRDUDE_PROGRAMMERID) -p $(AVRDUDE_MCU) -U flash:w:bin\$(NAME).hex

test:
	avrdude -c $(AVRDUDE_PROGRAMMERID) -p $(AVRDUDE_MCU) -v