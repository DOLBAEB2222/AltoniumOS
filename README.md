# OS Kernel Project

A minimal 32-bit bootable OS kernel with a GNU Make-based build system, NASM assembly bootloader, and GRUB multiboot support.

## Project Structure

```
.
├── boot/              # Bootloader code (NASM assembly)
│   ├── entry.asm     # Multiboot entry point
│   └── grub.cfg      # GRUB boot configuration
├── kernel/           # Kernel code (C/C++)
│   └── kernel.c      # Kernel main entry point
├── include/          # Header files for the kernel
├── build/            # Build artifacts (generated)
├── Makefile          # Build system
├── linker.ld         # Linker script
└── LICENSE           # MIT License
```

## Required Tools

### Linux (Ubuntu/Debian)

Install the required dependencies:

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    nasm \
    grub2-common \
    grub-pc-bin \
    xorriso \
    qemu-system-x86 \
    gcc-multilib \
    g++-multilib
```

### macOS

Using Homebrew:

```bash
brew install nasm grub xorriso qemu
```

### Fedora/RHEL

```bash
sudo dnf install -y \
    gcc gcc-c++ make \
    nasm \
    grub2-tools \
    xorriso \
    qemu-system-x86
```

## Building

### Quick Start

Build the entire project with:

```bash
make all
```

This creates:
- `build/kernel.elf` - The kernel ELF binary
- `build/kernel.iso` - A bootable ISO image

### Build Targets

- `make all` - Build kernel ELF and bootable ISO (default)
- `make iso` - Build only the bootable ISO
- `make clean` - Remove all build artifacts
- `make rebuild` - Clean and build everything
- `make help` - Display available targets

## Running

### Using QEMU

Run the kernel in QEMU:

```bash
make run
```

This will start the kernel in a QEMU virtual machine.

### Debugging with GDB

To run with GDB debugging support:

```bash
make run-debug
```

Then in another terminal, connect with GDB:

```bash
gdb build/kernel.elf -ex "target remote :1234" -ex "break kernel_main" -ex "continue"
```

### Physical Hardware

To copy the ISO to real hardware:

#### USB Thumb Drive (Linux)

1. Identify the USB device:
```bash
lsblk
# Look for a device like /dev/sdc (NOT /dev/sdc1)
```

2. Write the ISO to the USB drive:
```bash
sudo dd if=build/kernel.iso of=/dev/sdX bs=4M conv=fsync
# Replace /dev/sdX with your actual device
# WARNING: This will overwrite the entire device!
```

3. Boot from USB in your computer's BIOS/UEFI settings

#### Creating a Bootable CD/DVD

```bash
# Linux
sudo cdrecord -v -dev=/dev/sr0 build/kernel.iso

# macOS
hdiutil burn build/kernel.iso
```

#### Using GNOME Disks (GUI - Linux)

1. Open GNOME Disks
2. Select the USB drive
3. Click the menu (three dots)
4. Select "Restore Disk Image"
5. Choose `build/kernel.iso`

#### Using Etcher (GUI - Linux/macOS/Windows)

1. Download [Balena Etcher](https://www.balena.io/etcher/)
2. Open Etcher
3. Select `build/kernel.iso`
4. Select the target USB drive
5. Click Flash

## Customizing the Toolchain

You can override the toolchain by setting environment variables:

```bash
# Use Clang instead of GCC
make CC=clang

# Use custom NASM
make NASM=/usr/local/bin/nasm

# Use custom QEMU path
make QEMU=/custom/path/qemu-system-i386 run
```

## Multiboot Specification

This kernel adheres to the Multiboot specification (version 0.6.96). The bootloader (GRUB) loads the kernel at physical address 0x100000 (1MB) and provides information about the system in the multiboot structure.

## File Descriptions

- **boot/entry.asm** - Multiboot-compliant entry point in 32-bit protected mode
- **kernel/kernel.c** - Kernel main function
- **linker.ld** - Linker script for proper memory layout
- **boot/grub.cfg** - GRUB boot menu configuration
- **Makefile** - Build system with multiple targets

## Development Notes

- The kernel runs in 32-bit protected mode
- Default boot address is 1MB (0x100000)
- Stack is allocated at boot time with 16KB size
- NASM is used for assembly code
- GCC/Clang is used for C/C++ code
- Linking is performed with the GNU linker (ld)

## License

This project is licensed under the MIT License - see the LICENSE file for details.
