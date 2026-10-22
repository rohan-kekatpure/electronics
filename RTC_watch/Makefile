# Project Settings
TARGET = watch
PORT   = /dev/cu.SLAB_USBtoUART
BAUD   = 57600
MCU    = atmega328p
F_CPU  = 8000000UL

# Tools
CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# Compiler Flags
# -Os optimizes for size (critical for ATmega)
# -mmcu specifies the chip
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall

# 1. Generate HEX (The build chain)
hex: $(TARGET).hex

$(TARGET).o: $(TARGET).cc
	$(CC) $(CFLAGS) -c $(TARGET).cc -o $(TARGET).o

$(TARGET).elf: $(TARGET).o
	$(CC) $(CFLAGS) $(TARGET).o -o $(TARGET).elf

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $(TARGET).elf $(TARGET).hex

# 2. Flash over USB Serial
flash: $(TARGET).hex
	$(AVRDUDE) -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$(TARGET).hex:i

# Clean up build files
clean:
	rm -f *.o *.elf *.hex