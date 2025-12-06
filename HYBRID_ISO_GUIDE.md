# Hybrid GRUB ISO Guide

## Overview

AltoniumOS now includes a unified hybrid ISO that boots on both BIOS and UEFI systems without any manual configuration. This single ISO image (`dist/os-hybrid.iso`) provides:

- **Dual firmware support:** Works with both legacy BIOS and modern UEFI firmware
- **Multiple architectures:** Includes both x86 (32-bit) and x64 (64-bit) kernel binaries
- **Interactive menu:** GRUB menu allows selection of boot mode and architecture
- **Hardware diagnostics:** Built-in system information and diagnostics tools
- **Single image deployment:** One ISO works on all hardware types

## Key Features

### 1. Unified Boot Image

The hybrid ISO contains both:
- **BIOS boot path:** El Torito bootable CD with GRUB core.img
- **UEFI boot path:** EFI System Partition with BOOTX64.EFI and GRUBX64.EFI

### 2. Multi-Architecture Support

- **x86 32-bit kernel** (`/boot/x86/kernel.elf`) - Current production kernel
- **x64 64-bit kernel** (`/boot/x64/kernel64.elf`) - Placeholder for future 64-bit support

### 3. Boot Parameters

The GRUB menu passes kernel parameters to identify the boot mode:
- `bootmode=bios` or `bootmode=uefi` - Indicates firmware type
- `arch=x86` or `arch=x64` - Indicates target architecture

### 4. Interactive GRUB Menu

When you boot from the hybrid ISO, you see:

```
AltoniumOS - BIOS 32-bit Kernel
AltoniumOS - UEFI 32-bit Kernel  
AltoniumOS - UEFI 64-bit Kernel (Placeholder)
Hardware Diagnostics
```

The menu auto-boots the first entry after 5 seconds.

## Building the Hybrid ISO

### Prerequisites

```bash
sudo apt-get install nasm gcc binutils make python3
sudo apt-get install grub-pc-bin grub-efi-amd64-bin xorriso
sudo apt-get install gnu-efi ovmf  # For UEFI support
```

### Build Commands

```bash
# Build the hybrid ISO (recommended method)
make iso-hybrid

# Or use the default 'iso' target (now creates hybrid)
make iso

# Clean and rebuild
make clean && make iso-hybrid
```

### Build Output

The build process creates:

```
dist/
├── os-hybrid.iso          # Main hybrid ISO image (17MB)
├── kernel.elf             # x86 32-bit kernel binary
├── kernel64.elf           # x64 64-bit kernel placeholder
├── EFI/
│   ├── BOOT/
│   │   └── BOOTX64.EFI    # UEFI first-stage bootloader
│   └── ALTONIUM/
│       └── GRUBX64.EFI    # GRUB EFI chainloader
└── hybrid_iso/
    ├── boot/
    │   ├── grub/
    │   │   └── grub.cfg   # Unified GRUB configuration
    │   ├── x86/
    │   │   └── kernel.elf # x86 kernel in ISO
    │   └── x64/
    │       └── kernel64.elf # x64 kernel in ISO
    └── EFI/
        ├── BOOT/
        │   └── BOOTX64.EFI
        └── ALTONIUM/
            └── GRUBX64.EFI
```

## Testing

### Test in QEMU (BIOS Mode)

```bash
qemu-system-i386 -cdrom dist/os-hybrid.iso

# Or use the make target
make run-iso-hybrid
```

### Test in QEMU (UEFI Mode)

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom dist/os-hybrid.iso
```

### Automated Verification

```bash
./scripts/verify_hybrid_iso.sh
```

This script:
1. Tests BIOS boot with `qemu-system-i386`
2. Tests UEFI boot with `qemu-system-x86_64` + OVMF
3. Verifies ISO structure and contents
4. Reports pass/fail for each test

## Deployment to Real Hardware

### Writing to USB Drive

**Linux:**
```bash
# Identify your USB device
lsblk

# Write the hybrid ISO (REPLACE /dev/sdX with your device!)
sudo dd if=dist/os-hybrid.iso of=/dev/sdX bs=4M status=progress conv=fsync

# Ensure data is flushed
sync
```

**Windows:**
- Use Rufus, Etcher, or Win32 Disk Imager
- Select the hybrid ISO
- Write in DD mode
- No special partition scheme required

### Booting from USB

1. **Insert USB drive** into target machine
2. **Enter BIOS/UEFI setup** (usually F2, F10, or Del during boot)
3. **Configure boot settings:**
   - Disable Secure Boot (if booting in UEFI mode)
   - Enable USB boot
   - Set USB as first boot device
4. **Save and reboot**
5. **Select boot mode:**
   - UEFI systems: Choose "UEFI: [USB Drive Name]"
   - BIOS systems: Choose "USB: [Drive Name]" or "Removable Device"

### What You'll See

**GRUB Menu (Both BIOS and UEFI):**
```
                     GNU GRUB version 2.12
┌─────────────────────────────────────────────────┐
│*AltoniumOS - BIOS 32-bit Kernel                 │
│ AltoniumOS - UEFI 32-bit Kernel                 │
│ AltoniumOS - UEFI 64-bit Kernel (Placeholder)   │
│ Hardware Diagnostics                            │
└─────────────────────────────────────────────────┘
```

**After Kernel Boots:**
```
Welcome to AltoniumOS 1.0.0
Boot mode: BIOS (or UEFI)
Initializing disk driver... OK
Running disk self-test... OK
Initializing FAT12 filesystem... OK
Mounted volume at /
Type 'help' for available commands

>
```

## GRUB Menu Options Explained

### 1. AltoniumOS - BIOS 32-bit Kernel

- **Purpose:** Boot the x86 kernel in BIOS mode
- **Target:** `/boot/x86/kernel.elf`
- **Parameters:** `bootmode=bios arch=x86`
- **Best for:** Legacy systems, maximum compatibility

### 2. AltoniumOS - UEFI 32-bit Kernel

- **Purpose:** Boot the x86 kernel in UEFI mode
- **Target:** `/boot/x86/kernel.elf`
- **Parameters:** `bootmode=uefi arch=x86`
- **Best for:** Modern systems with UEFI firmware

### 3. AltoniumOS - UEFI 64-bit Kernel (Placeholder)

- **Purpose:** Demonstrate x64 kernel support
- **Target:** `/boot/x64/kernel64.elf`
- **Parameters:** `bootmode=uefi arch=x64`
- **Current status:** Shows placeholder message
- **Future:** Will boot full 64-bit kernel

### 4. Hardware Diagnostics

- **Purpose:** Display system information
- **Shows:**
  - Boot mode (BIOS/UEFI)
  - Architecture details
  - Memory map (`lsmmap`)
  - Available devices (`ls -l`)
- **Use:** Troubleshooting boot issues

## Technical Details

### ISO Structure

The hybrid ISO uses:
- **El Torito boot specification** for BIOS compatibility
- **EFI System Partition** for UEFI compatibility
- **ISO 9660 + Joliet** file system extensions
- **Rock Ridge extensions** for long filenames

### UEFI Boot Chain

1. **Firmware** loads `EFI/BOOT/BOOTX64.EFI`
2. **BOOTX64.EFI** (custom bootstrapper):
   - Displays firmware diagnostics
   - Verifies kernel.elf presence
   - Loads `EFI/ALTONIUM/GRUBX64.EFI`
3. **GRUBX64.EFI** (GRUB EFI chainloader):
   - Reads `/boot/grub/grub.cfg`
   - Displays boot menu
   - Uses multiboot protocol
4. **Kernel** receives control with boot parameters

### BIOS Boot Chain

1. **BIOS** loads El Torito boot sector
2. **GRUB** core.img loads from ISO
3. **GRUB** reads `/boot/grub/grub.cfg`
4. **GRUB** displays boot menu
5. **Kernel** loaded via multiboot protocol

### Advantages Over Separate ISOs

**Before (separate ISOs):**
- Required two different ISO files
- Had to choose correct ISO for target hardware
- Different build and test procedures
- Two images to maintain and document

**Now (hybrid ISO):**
- Single ISO works everywhere
- Firmware auto-detects boot mode
- Unified build and test process
- One image to maintain
- Easier deployment and distribution

## Troubleshooting

### Issue: ISO Only Boots in BIOS Mode

**Solution:** 
- Verify Secure Boot is disabled in UEFI setup
- Ensure you selected the UEFI boot entry (not Legacy)
- Check BIOS boot priority settings

### Issue: ISO Only Boots in UEFI Mode

**Solution:**
- Enable Legacy boot or CSM mode in BIOS
- Verify USB boot is enabled
- Try a different USB port

### Issue: GRUB Menu Doesn't Appear

**Symptoms:** System boots but no menu shown

**Solution:**
- The default entry auto-boots after 5 seconds
- Press a key during boot to stop countdown
- Check the GRUB config timeout setting

### Issue: "No bootable device" Error

**Solution:**
1. Verify ISO was written correctly: `md5sum dist/os-hybrid.iso`
2. Rewrite USB: `sudo dd if=dist/os-hybrid.iso of=/dev/sdX bs=4M conv=fsync`
3. Check BIOS boot order settings
4. Try a different USB drive

### Issue: System Hangs at GRUB Menu

**Solution:**
- Press Enter to boot the highlighted entry
- Press 'e' to edit boot parameters
- Press 'c' for GRUB command line
- Rebuild ISO: `make clean && make iso-hybrid`

## Future Enhancements

The hybrid ISO infrastructure supports future improvements:

1. **64-bit Kernel Implementation**
   - Replace placeholder with real x64 kernel
   - Full 64-bit memory support
   - 64-bit driver development

2. **Additional Boot Options**
   - Safe mode boot
   - Debug mode with logging
   - Recovery mode
   - Network boot integration

3. **BOOTIA32.EFI Support**
   - Add 32-bit UEFI bootloader
   - Support for 32-bit UEFI tablets
   - Complete architecture coverage

4. **Customizable Boot Parameters**
   - Video mode selection
   - Memory limits
   - Driver options
   - Debug flags

## See Also

- `README.md` - Main project documentation
- `QUICK_START.md` - Quick start guide
- `UEFI_BOOT_GUIDE.md` - Detailed UEFI information
- `Makefile` - Build system documentation
- `grub/hybrid.cfg` - GRUB configuration file
- `scripts/verify_hybrid_iso.sh` - Verification script
- `bootloader/uefi_loader.c` - UEFI bootstrap source

## Summary

The hybrid ISO represents a significant improvement in AltoniumOS deployment:
- **One image for all hardware** - Works on BIOS and UEFI systems
- **Multiple architectures** - x86 and x64 support in one package
- **User choice** - Interactive menu for boot options
- **Better testing** - Single build artifact to verify
- **Easier deployment** - One ISO to distribute and document

Build with `make iso-hybrid` and enjoy universal compatibility!
