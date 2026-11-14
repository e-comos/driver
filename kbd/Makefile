# Keyboard driver: standalone module, loaded at 0x800000 by init-service.
CC      = x86_64-elf-gcc
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
PREFIX  ?= $(HOME)/.local/eclib

CFLAGS  = -ffreestanding -fno-pie -fno-stack-protector -mno-red-zone \
          -Wall -Wextra -O2 -I$(PREFIX)/include
LDFLAGS = -nostdlib -T linker.ld -L$(PREFIX)/lib
LDLIBS  = -leclib

TARGET = kbd.bin

all: $(TARGET)

kbd.o: kbd.c
	$(CC) $(CFLAGS) -c $< -o $@

kbd.elf: kbd.o linker.ld
	$(LD) $(LDFLAGS) -o $@ kbd.o $(LDLIBS)

$(TARGET): kbd.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f kbd.o kbd.elf $(TARGET)
