.PHONY: all clean build run iso img run-iso run-img

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -std=c99 -ffreestanding -fno-stack-protector -Wall -Wextra -nostdlib
LDFLAGS = -m elf_i386 -T linker.ld

BUILD_DIR = build
DIST_DIR = dist

all: build

build: $(DIST_DIR)/kernel.elf $(DIST_DIR)/kernel.bin

dirs:
	@mkdir -p $(BUILD_DIR) $(DIST_DIR)

$(BUILD_DIR)/kernel_entry.o: kernel_entry.asm dirs
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.o: kernel.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/disk.o: disk.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(DIST_DIR)/kernel.elf: $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/disk.o dirs
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/disk.o

$(DIST_DIR)/kernel.bin: $(DIST_DIR)/kernel.elf
	objcopy -O binary $< $@

$(DIST_DIR)/boot.bin: boot.asm dirs
	$(AS) -f bin -o $@ $<

bootable: $(DIST_DIR)/boot.bin $(DIST_DIR)/kernel.bin
	@echo "Creating bootable image..."
	@dd if=/dev/zero of=$(DIST_DIR)/os.img bs=1M count=10 2>/dev/null
	@dd if=$(DIST_DIR)/boot.bin of=$(DIST_DIR)/os.img bs=512 count=1 conv=notrunc 2>/dev/null
	@dd if=$(DIST_DIR)/kernel.bin of=$(DIST_DIR)/os.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "Bootable image created at $(DIST_DIR)/os.img"

iso: $(DIST_DIR)/kernel.elf
	@echo "Creating bootable ISO image..."
	@mkdir -p $(DIST_DIR)/iso/boot/grub
	@cp $(DIST_DIR)/kernel.elf $(DIST_DIR)/iso/boot/
	@cp grub.cfg $(DIST_DIR)/iso/boot/grub/
	@grub-mkrescue -o $(DIST_DIR)/os.iso $(DIST_DIR)/iso 2>/dev/null
	@echo "Bootable ISO image created at $(DIST_DIR)/os.iso"

img: $(DIST_DIR)/boot.bin $(DIST_DIR)/kernel.bin
	@echo "Creating raw disk image..."
	@dd if=/dev/zero of=$(DIST_DIR)/os.img bs=1M count=10 2>/dev/null
	@dd if=$(DIST_DIR)/boot.bin of=$(DIST_DIR)/os.img bs=512 count=1 conv=notrunc 2>/dev/null
	@dd if=$(DIST_DIR)/kernel.bin of=$(DIST_DIR)/os.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "Raw disk image created at $(DIST_DIR)/os.img"

run-iso: iso
	@echo "Run ISO with: qemu-system-i386 -cdrom $(DIST_DIR)/os.iso"

run-img: img
	@echo "Run IMG with: qemu-system-i386 -hda $(DIST_DIR)/os.img"

run: $(DIST_DIR)/kernel.elf
	@echo "Run with: qemu-system-i386 -kernel $(DIST_DIR)/kernel.elf"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

help:
	@echo "Targets:"
	@echo "  all       - Build the kernel (default)"
	@echo "  build     - Build the kernel"
	@echo "  iso       - Create a bootable ISO image"
	@echo "  img       - Create a raw disk image"
	@echo "  run-iso   - Show how to run ISO in QEMU"
	@echo "  run-img   - Show how to run IMG in QEMU"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Show how to run with QEMU direct kernel boot"
	@echo "  help      - Show this help message"