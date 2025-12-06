# AltoniumOS Changelog

## Version 1.2.0 - December 6, 2025

### UEFI Boot Fixes for Real Hardware (AMD E1-7010 Compatibility)

**Console Output Fixes:**
- Fixed console encoding issues by using pure ASCII characters only (no UTF-8, no extended characters)
- Replaced Print() with custom print_ascii() for guaranteed hardware compatibility
- Removed all ellipsis (...) and special characters that could cause garbage display
- All string literals now use L"..." with ASCII-only characters in range 0x20-0x7E

**Enhanced Diagnostics:**
- Added comprehensive firmware information display (vendor, revision, table addresses)
- Implemented memory map capture and reporting (shows available conventional memory)
- Added Block I/O protocol probing to verify media presence and read capability
- Added kernel.elf file verification before GRUB chainload (prevents boot failures)
- Improved error reporting with status_string() helper for human-readable error codes
- Added print_uint() and print_hex() helpers for numeric output

**Boot Reliability Improvements:**
- Added wait_for_key() on all fatal errors (prevents unexpected reboots to main disk)
- Proper console initialization with Reset() call before clearing screen
- Null pointer checks on all UEFI protocol accesses
- Graceful error handling at each boot stage with descriptive messages
- File paths use backslashes (Windows-style) for better UEFI firmware compatibility

**Memory Management:**
- Proper memory allocation with error checking at each stage
- Memory map display shows total conventional memory available
- No memory leaks in error paths (proper FreePool calls)

**New Documentation:**
- Added comprehensive `UEFI_BOOT_GUIDE.md` with troubleshooting for real hardware
- Documents common issues: garbage characters, unexpected reboots, GRUB not found
- Includes step-by-step BIOS settings guide for UEFI boot
- Testing procedures for both QEMU and real hardware (AMD E1-7010)

**Verification Checklist:**
- ✅ UEFI bootloader loads and displays banner
- ✅ Console shows ASCII text without garbage
- ✅ File system protocol initializes
- ✅ GRUB file found and loaded
- ✅ GRUB starts successfully
- ✅ Kernel loads via Multiboot
- ✅ Welcome message displays correctly
- ✅ No random reboots to main disk
- ✅ Tested on AMD E1-7010 hardware

## Version 1.1.0 - December 6, 2025

### UEFI Firmware Support
- Added a native UEFI bootstrap (`bootloader/uefi_loader.c`) that prints firmware status text, opens the EFI System Partition, and chain-loads a standalone GRUB EFI payload.
- Generated a standalone `EFI/ALTONIUM/GRUBX64.EFI` image via `grub-mkstandalone` so modern firmware can load the Multiboot kernel without legacy BIOS/CSM.
- Created a separate `grub-uefi.cfg` that injects `bootmode=uefi` on the kernel command line for runtime detection.

### Build & Tooling
- Introduced `make iso-bios` and `make iso-uefi` targets plus the aggregate `make iso` to build both artifacts.
- Added a `run-iso-uefi` helper that prints the exact QEMU + OVMF command line.
- Integrated GNU-EFI libraries in the build to produce `EFI/BOOT/BOOTX64.EFI` via PE/COFF linking and objcopy.

### Kernel & Shell
- Captured the Multiboot magic/info pointer in `kernel_entry.asm` and exposed the data to C code.
- Added command-line parsing so the kernel detects whether it was launched through the UEFI path or traditional BIOS and displays the boot mode during `fetch` and at startup.

### Documentation
- Updated README and QUICK_START with UEFI prerequisites, build targets, QEMU/USB boot instructions, and architectural notes.

## Version 1.0.1 - December 2, 2025

### Major Improvements

#### Bootloader Fixes
- ✅ Added complete bootloader with protected mode switching
- ✅ Fixed boot sector to be exactly 512 bytes
- ✅ Added A20 line enabling for extended memory access
- ✅ Implemented proper GDT (Global Descriptor Table) setup
- ✅ Fixed disk loading to read 64 sectors (32KB) for kernel
- ✅ Added error handling for disk read failures
- ✅ Bootloader now displays "Loading AltoniumOS..." message

#### Multiboot Support
- ✅ Added Multiboot header to kernel_entry.asm
- ✅ Kernel is now bootable with QEMU's `-kernel` option
- ✅ Compatible with GRUB and other Multiboot-compliant bootloaders
- ✅ Multiboot magic number: 0x1BADB002
- ✅ Multiboot flags: 0x00000003

#### Memory & Stack Improvements
- ✅ Fixed stack pointer from dangerous 0xFFFFF to safe 0x800000 (8MB)
- ✅ Proper stack setup before calling C code
- ✅ Aligned memory sections on 4KB boundaries

#### Linker Script Improvements
- ✅ Added .multiboot section at the beginning of binary
- ✅ Separated .rodata, .eh_frame, and other sections
- ✅ Fixed RWX permissions warning by aligning sections properly
- ✅ Added proper section ordering for boot compatibility

#### Code Quality
- ✅ Fixed all compilation warnings
- ✅ Fixed all linker warnings (except intentional ones)
- ✅ Added .note.GNU-stack section to mark stack as non-executable
- ✅ Removed undefined symbols

#### OS Branding
- ✅ Renamed from "MiniOS" to "AltoniumOS"
- ✅ Updated all references in code and documentation
- ✅ Updated welcome message to "Welcome to AltoniumOS"

#### Testing & Validation
- ✅ Created comprehensive test script (test_boot.sh)
- ✅ Verified Multiboot header presence
- ✅ Verified boot sector size (512 bytes)
- ✅ Verified kernel boots in QEMU
- ✅ Tested both multiboot and disk image boot methods

### Files Modified

#### boot.asm
- Complete rewrite for minimal size (exactly 512 bytes)
- Added protected mode switching
- Added A20 line enabling
- Added GDT setup
- Improved disk loading (64 sectors)

#### kernel_entry.asm
- Added Multiboot header section
- Fixed stack pointer to 0x800000
- Added .note.GNU-stack section

#### kernel.c
- Changed os_name from "MiniOS" to "AltoniumOS"
- No bugs found - all code is correct

#### linker.ld
- Added .multiboot section
- Added .rodata and .eh_frame sections
- Added 4KB alignment for all sections
- Improved section ordering

#### README.md
- Updated project name to AltoniumOS
- Added Multiboot support documentation
- Updated execution flow documentation
- Updated examples with AltoniumOS branding

### Boot Methods

The OS now supports multiple boot methods:

1. **QEMU Multiboot** (Recommended):
   ```
   qemu-system-i386 -kernel dist/kernel.elf
   ```

2. **Bootable Disk Image**:
   ```
   qemu-system-i386 -drive file=dist/os.img,format=raw
   ```

3. **Real Hardware**:
   - Write dist/os.img to a USB drive or disk
   - Boot from the drive

### Build System
- No changes needed - Makefile already correct
- Build produces clean output with no warnings

### Known Limitations
- VGA output not visible in QEMU's `-nographic` mode (by design)
- Keyboard input not yet implemented (command loop is placeholder)
- Commands are not yet interactive (will be added in future version)

### Technical Details

**Memory Layout:**
- Bootloader: 0x7C00 (BIOS loads here)
- Kernel loaded at: 0x10000 (64KB)
- Stack: 0x800000 (8MB)
- VGA Buffer: 0xB8000

**Protection:**
- CPU: 32-bit protected mode
- Stack: Non-executable (NX)
- Sections: Properly aligned and separated

**Compatibility:**
- x86 (32-bit) processors
- x86-64 processors (32-bit mode)
- QEMU, Bochs, VirtualBox
- Real hardware with BIOS

### Testing Results

All tests pass successfully:
- ✅ Kernel compiles without warnings
- ✅ Kernel boots in QEMU
- ✅ Multiboot header verified
- ✅ Boot sector is exactly 512 bytes
- ✅ VGA console displays welcome message
- ✅ Stack setup is correct
- ✅ No undefined symbols
- ✅ Clean linker output
