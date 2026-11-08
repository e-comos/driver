# Compiler and Toolchain Configuration
CC      = gcc
LD      = ld
OBJCOPY = objcopy

TARGET  = drivers.bin
OBJS    = vga/eightvga.o

# Freestanding flags tailored for low-level OS development
CFLAGS  = -ffreestanding \
        -fno-pie \
        -fno-stack-protector \
        -mno-red-zone \
        -Wall \
        -Wextra \
        -O2 \
        -Iinclude \
        -I$(HOME)/.local/include \
        -g

# Linker flags and libraries
LDFLAGS = -nostdlib \
          -T linker.ld \
          -L$(HOME)/.local/lib
LDLIBS  = -leclib

.PHONY: all clean

all: $(TARGET)

vga/eightvga.o: vga/eightvga.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): drivers.elf
	$(OBJCOPY) -O binary drivers.elf $(TARGET)
	@echo "Successfully built $(TARGET)"

drivers.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo "Linking multi-driver bundle..."

clean:
	rm -f $(OBJS) drivers.elf $(TARGET)
