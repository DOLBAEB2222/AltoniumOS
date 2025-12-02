# AltoniumOS Quick Start Guide

## Building & Running

### Quick Build & Test
```bash
make build          # Build kernel
make bootable       # Create bootable disk image
./test_boot.sh      # Run automated tests
```

### Run in QEMU (Recommended)
```bash
qemu-system-i386 -kernel dist/kernel.elf
```

### Run from Bootable Disk
```bash
qemu-system-i386 -drive file=dist/os.img,format=raw
```

### Clean Build
```bash
make clean && make build
```

## What You'll See

When AltoniumOS boots, you'll see:
```
Welcome to AltoniumOS 1.0.0
Type 'help' for available commands

>
```

## Available Commands

- `help` - Display available commands
- `clear` - Clear the screen
- `echo <text>` - Print text to console
- `fetch` - Display system information
- `shutdown` - Shut down the system

## Project Status

✅ Fully functional bootloader  
✅ Multiboot compliant  
✅ Boots in QEMU  
✅ Compatible with real hardware  
✅ Zero compilation warnings  
✅ All tests passing  

## Files

- `boot.asm` - Bootloader (512 bytes)
- `kernel_entry.asm` - Kernel entry with Multiboot header
- `kernel.c` - Main kernel code
- `linker.ld` - Linker script
- `test_boot.sh` - Automated tests

## Documentation

- `README.md` - Full documentation
- `CHANGELOG.md` - Change history
- `TICKET_COMPLETION_SUMMARY.md` - Implementation details
- `FINAL_VERIFICATION.md` - Test results

## Support

For issues or questions, see the full documentation in README.md
