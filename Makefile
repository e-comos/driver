# Compiler and Toolchain Configuration
CC      = gcc
LD      = ld
OBJCOPY = objcopy

# Freestanding flags tailored for low-level OS development
CFLAGS  = -ffreestanding \
          -fno-pie \
          -fno-stack-protector \
          -mno-red-zone \
          -Wall \
          -Wextra \
          -O2 \
          -Iinclude

# Linker flags for flat binary/custom entry execution
LDFLAGS = -nostdlib \
          -T linker.ld

TARGET  = vga_driver.bin
SRCS    = src/vga_driver.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o vga_driver.elf $(OBJS)
	$(OBJCOPY) -O binary vga_driver.elf $(TARGET)
	@echo "Successfully built $(TARGET)"

clean:
	rm -f $(OBJS) vga_driver.elf $(TARGET)
