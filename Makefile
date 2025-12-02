CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
LDFLAGS = -m32 -nostdlib -Wl,--build-id=none -T linker.ld

# Directories
KERNEL_DIR = kernel
BUILD_DIR = build

# Source files
KERNEL_SOURCES = $(KERNEL_DIR)/console.c
SOURCES = kernel.c $(KERNEL_SOURCES)

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target
TARGET = kernel.bin

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -rf $(BUILD_DIR)

# Create a simple linker script if it doesn't exist
linker.ld:
	@echo "Creating default linker script"
	@echo 'ENTRY(kernel_main)' > linker.ld
	@echo 'SECTIONS {' >> linker.ld
	@echo '    . = 1M;' >> linker.ld
	@echo '    .text : { *(.text) }' >> linker.ld
	@echo '    .data : { *(.data) }' >> linker.ld
	@echo '    .bss : { *(.bss) }' >> linker.ld
	@echo '}' >> linker.ld