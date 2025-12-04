# AltoniumOS Quick Start Guide

## Building & Running

### Quick Build & Test
```bash
make build          # Build kernel
make img            # Create the FAT12 disk image (make bootable is an alias)
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
Initializing FAT12 filesystem... OK
Mounted volume at /
Type 'help' for available commands

>
```

## Available Commands

- `help` - Display available commands
- `clear` - Clear the screen
- `echo <text>` - Print text to console
- `fetch` - Display system information
- `disk` - Test disk I/O and show disk information
- `ls [PATH]` - List files in the current directory or a specified path
- `dir [PATH]` - Alias for ls
- `pwd` - Show the current working directory
- `cd PATH` - Change directories (supports `/`, `.`, and `..`)
- `cat FILE` - Dump a file stored on the FAT12 volume
- `touch FILE` - Create a zero-length file
- `write FILE TEXT` - Create/overwrite an 8.3 text file using the rest of the line
- `mkdir NAME` - Create a directory
- `rm FILE` - Delete a regular file
- `shutdown` - Shut down the system

### Testing Disk I/O

To test the disk functionality:

1. Boot with disk support (see command above)
2. Type `disk` at the prompt
3. You should see disk initialization, sector read test, and hex dump

### Working with Files

After the `disk` command succeeds, try the FAT12 shell commands:

```text
dir
[DIR] DOCS
      README.TXT (92 bytes)
      SYSTEM.CFG (42 bytes)
```

```
cd docs
pwd              # Shows /DOCS
cat INFO.TXT     # Prints the seeded note
touch EMPTY.TXT  # Creates a zero-length file
ls               # Shows all files including EMPTY.TXT
write TODO.TXT Remember the demo
```

Filenames must follow the 8.3 DOS convention and shell writes are limited to ~16 KB per file creation. Both `ls` and `dir` can be used interchangeably.

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
✅ FAT12 filesystem with shell commands  

## Files

- `boot.asm` - Bootloader + FAT12 BPB (512 bytes)
- `kernel_entry.asm` - Kernel entry with Multiboot header
- `kernel.c` - Main kernel code
- `disk.c` / `disk.h` - ATA PIO disk driver
- `fat12.c` / `fat12.h` - FAT12 filesystem implementation
- `scripts/build_fat12_image.py` - Generates the FAT12 disk layout used by `make img`
- `linker.ld` - Linker script
- `test_boot.sh` - Automated tests

## Documentation

- `README.md` - Full documentation
- `CHANGELOG.md` - Change history
- `TICKET_COMPLETION_SUMMARY.md` - Implementation details
- `FINAL_VERIFICATION.md` - Test results

## Support

For issues or questions, see full documentation in README.md