# MyOS - Multiboot-compliant 32-bit Kernel

A minimal operating system kernel with Multiboot-compliant bootstrap code written in NASM, compatible with GRUB, QEMU, and legacy BIOS hardware.

## Features

- **Multiboot-compliant bootstrap**: Implements the Multiboot specification for compatibility with GRUB and other Multiboot-compliant bootloaders
- **32-bit protected mode**: Full GDT setup with proper code and data segments
- **Stack initialization**: 16 KB stack for kernel operations
- **BSS section zeroing**: Proper initialization of uninitialized data
- **QEMU compatible**: Can be loaded directly with `qemu-system-i386 -kernel`
- **Legacy BIOS support**: Works on real hardware with GRUB

## Boot Process

The boot sequence follows these steps:

1. **Multiboot Header**: GRUB reads the Multiboot header from `boot/boot.asm`
2. **Protected Mode Entry**: Bootloader loads kernel at 1 MB and enters at `_start`
3. **GDT Setup**: A minimal Global Descriptor Table is loaded with flat code and data segments
4. **Segment Initialization**: All segment registers are updated to use the new GDT
5. **Stack Setup**: ESP is set to the top of a 16 KB stack
6. **BSS Zeroing**: The `.bss` section is cleared to zero
7. **Kernel Entry**: Control is transferred to `kernel_main` with Multiboot magic and info pointer

## Building

### Prerequisites

- NASM (Netwide Assembler)
- GCC with 32-bit support
- GNU ld (linker)
- GRUB tools (optional, for ISO creation)
- QEMU (optional, for testing)

### Build Commands

```bash
# Build kernel binary
make

# Create bootable ISO
make iso

# Run in QEMU
make run

# Clean build artifacts
make clean
```

## File Structure

```
boot/
  boot.asm          - Multiboot-compliant bootstrap code (NASM)
kernel/
  main.c            - Kernel entry point (C code)
linker.ld           - Linker script defining memory layout
Makefile            - Build system
```

## Boot Code Details

### Multiboot Header

The Multiboot header is placed in its own section (`.multiboot`) and contains:
- Magic number: `0x1BADB002`
- Flags: Page alignment + Memory info
- Checksum: Ensures header validity

### GDT (Global Descriptor Table)

The bootstrap sets up a minimal flat memory model:
- **Null descriptor** (required by x86 architecture)
- **Code segment**: Base 0, limit 4 GB, 32-bit, executable
- **Data segment**: Base 0, limit 4 GB, 32-bit, writable

### Stack

A 16 KB stack is allocated in the `.bss` section, providing sufficient space for kernel initialization and operation.

### BSS Zeroing

The bootstrap zeroes the `.bss` section (uninitialized data) to ensure proper C runtime environment. The linker script provides `__bss_start` and `__bss_end` symbols.

## Testing

### QEMU

```bash
make run
```

This will build the kernel and run it in QEMU. You should see "Kernel booted successfully!" displayed.

### Real Hardware

1. Create bootable ISO: `make iso`
2. Write to USB drive: `dd if=myos.iso of=/dev/sdX bs=4M`
3. Boot from USB on target hardware

### Debugging

```bash
# Run with QEMU debugger
qemu-system-i386 -kernel kernel.bin -s -S

# In another terminal
gdb kernel.bin
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Compatibility

- **GRUB**: Fully compatible with GRUB Legacy and GRUB 2
- **QEMU**: Can be loaded directly with `-kernel` option
- **Legacy BIOS**: Works on hardware with traditional BIOS
- **UEFI**: Not supported (requires different boot protocol)

## License

See LICENSE file for details.
