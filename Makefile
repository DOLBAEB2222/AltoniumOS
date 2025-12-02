# GNU Make-based build system for the OS kernel

# Default target
.PHONY: all
all: build/kernel.iso

# Toolchain configuration
NASM ?= nasm
CC ?= gcc
LD ?= ld
GRUB_MKRESCUE ?= grub-mkrescue
XORRISO ?= xorriso
QEMU ?= qemu-system-i386

# Compiler flags for 32-bit kernel
CFLAGS = -m32 -Wall -Wextra -Werror -ffreestanding -fno-builtin -nostdlib
NASMFLAGS = -f elf32 -F dwarf -g
LDFLAGS = -m elf_i386 -T linker.ld --oformat=elf32-i386

# Source files
BOOT_ASM = boot/entry.asm
KERNEL_C = kernel/kernel.c

# Object files
BOOT_OBJ = build/entry.o
KERNEL_OBJ = build/kernel.o

# Final kernel binary
KERNEL_ELF = build/kernel.elf
ISO_FILE = build/kernel.iso

# Create build directory
$(shell mkdir -p build)

# Assembly compilation
$(BOOT_OBJ): $(BOOT_ASM)
	@echo "[NASM] $<"
	@$(NASM) $(NASMFLAGS) -o $@ $<

# C compilation
$(KERNEL_OBJ): $(KERNEL_C)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Linking
$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJ)
	@echo "[LD] Linking kernel"
	@$(LD) $(LDFLAGS) -o $@ $^

# ISO image creation
$(ISO_FILE): $(KERNEL_ELF)
	@echo "[ISO] Creating bootable ISO"
	@mkdir -p build/isodir/boot/grub
	@cp $(KERNEL_ELF) build/isodir/boot/
	@cp boot/grub.cfg build/isodir/boot/grub/
	@$(GRUB_MKRESCUE) -o $@ build/isodir

# Clean build artifacts
.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts"
	@rm -rf build/

# Full rebuild
.PHONY: rebuild
rebuild: clean all

# ISO target
.PHONY: iso
iso: $(ISO_FILE)
	@echo "[OK] ISO ready at $(ISO_FILE)"

# Run with QEMU
.PHONY: run
run: $(ISO_FILE)
	@echo "[QEMU] Running kernel"
	@$(QEMU) -cdrom $(ISO_FILE) -m 512M

# Run with debugging
.PHONY: run-debug
run-debug: $(ISO_FILE)
	@echo "[QEMU] Running kernel with debugging"
	@$(QEMU) -cdrom $(ISO_FILE) -m 512M -S -gdb tcp::1234

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make all          - Build the kernel ELF and ISO (default)"
	@echo "  make iso          - Build bootable ISO image"
	@echo "  make run          - Run kernel in QEMU"
	@echo "  make run-debug    - Run kernel in QEMU with GDB debugging"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and rebuild everything"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Toolchain:"
	@echo "  NASM=$(NASM)"
	@echo "  CC=$(CC)"
	@echo "  LD=$(LD)"
	@echo "  GRUB_MKRESCUE=$(GRUB_MKRESCUE)"
	@echo "  QEMU=$(QEMU)"
