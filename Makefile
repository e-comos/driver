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

# Linker flags and libraries
LDFLAGS = -nostdlib \
          -T linker.ld \
          -L/usr/local/lib
LDLIBS  = -leclib

TARGET  = drivers.bin

# Automatically find all .c files in any subdirectory
SRCS    = $(shell find . -name "*.c")
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

# Generic pattern rule to compile any .c file anywhere in the repo
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o drivers.elf $(OBJS) $(LDLIBS)
	@echo "Linking multi-driver bundle..."
	$(OBJCOPY) -O binary drivers.elf $(TARGET)
	@echo "Successfully built $(TARGET)"

clean:
	rm -f $(OBJS) drivers.elf $(TARGET)
