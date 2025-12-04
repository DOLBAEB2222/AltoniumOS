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

### Run with Disk I/O Support
```bash
qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide
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
Initializing disk driver... OK
Running disk self-test... OK
Type 'help' for available commands

>
```

## Available Commands

- `help` - Display available commands
- `clear` - Clear the screen
- `echo <text>` - Print text to console
- `fetch` - Display system information
- `disk` - Test disk I/O and show disk information
- `shutdown` - Shut down the system

### Testing Disk I/O

To test the disk functionality:

1. Boot with disk support (see command above)
2. Type `disk` at the prompt
3. You should see disk initialization, sector read test, and hex dump

## Project Status

✅ Fully functional bootloader  
✅ Multiboot compliant  
✅ Boots in QEMU  
✅ Compatible with real hardware  
✅ Zero compilation warnings  
✅ All tests passing  
✅ ATA PIO disk driver implemented  
✅ LBA addressing support  
✅ Sector read/write operations  

## Files

- `boot.asm` - Bootloader (512 bytes)
- `kernel_entry.asm` - Kernel entry with Multiboot header
- `kernel.c` - Main kernel code
- `disk.c` - ATA PIO disk driver
- `disk.h` - Disk driver header
- `linker.ld` - Linker script
- `test_boot.sh` - Automated tests

## Documentation

- `README.md` - Full documentation
- `CHANGELOG.md` - Change history
- `TICKET_COMPLETION_SUMMARY.md` - Implementation details
- `FINAL_VERIFICATION.md` - Test results

## Support

For issues or questions, see full documentation in README.md