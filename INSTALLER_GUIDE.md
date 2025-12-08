# AltoniumOS Installer Guide

This guide explains how to use the AltoniumOS installer and disk partitioner tools.

## Overview

AltoniumOS includes two installer utilities:

1. **`install`** - Full OS installer wizard that guides you through disk partitioning, formatting, and OS installation
2. **`diskpart`** - Standalone disk partition manager for creating, deleting, and managing partitions

Both utilities use a lightweight Text User Interface (TUI) for easy navigation.

## System Requirements

- x86 or x86_64 compatible system
- At least 16 MB of RAM
- Target disk with at least 100 MB of free space
- Keyboard for navigation

## Running the Installer

### Full Installation Wizard

To launch the complete installation wizard:

```
> install
```

The installer will guide you through the following steps:

#### Step 1: Select Target Disk

- Lists all detected storage devices (IDE, SATA AHCI, NVMe)
- Shows disk capacity and driver information
- Use **UP/DOWN** arrow keys to select a disk
- Press **ENTER** to continue or **ESC** to cancel

**Warning:** The selected disk will be completely reformatted. All existing data will be lost!

#### Step 2: Select Partition Table Type

Choose between:
- **MBR (Master Boot Record)** - Legacy BIOS compatibility (recommended)
- **GPT (GUID Partition Table)** - UEFI systems (stub implementation)

Most systems should use MBR for maximum compatibility.

#### Step 3: Select Filesystem Type

Choose the filesystem for your installation:

- **FAT12** - Legacy filesystem, uses existing AltoniumOS formatter
- **FAT32** - Windows compatible, recommended for most users
- **ext2** - Linux filesystem (basic implementation)

**FAT32** is recommended for general use as it provides the best compatibility and supports larger volumes.

#### Step 4: Confirm Formatting

- Review your selections
- Confirm that you want to format the target disk
- Press **Y** to proceed or **N** to go back

The installer will:
1. Create a new partition table
2. Create a primary partition
3. Format the partition with your chosen filesystem
4. Display progress messages

#### Step 5: Copy System Files

Displays a summary of files to be copied:
- Kernel image (kernel.bin)
- Boot configuration
- System files

**Note:** In the current version, file copying is stubbed. On real hardware with a proper source medium (USB/CD), this step would copy the OS files to the target disk.

#### Step 6: Install Bootloader

Shows bootloader installation status:
- Target disk information
- Boot sector type (MBR)
- Installation readiness

**Note:** Bootloader installation is stubbed in this version. On real hardware, this would write the boot sector to make the disk bootable.

#### Step 7: Installation Complete

- Displays a summary of completed actions
- Press any key to return to the shell

## Disk Partition Manager

The `diskpart` utility provides direct access to partition management without running the full installer.

### Launching Diskpart

```
> diskpart
```

### Diskpart Features

#### Current Partition Layout

Displays all existing partitions with:
- Partition number
- Filesystem type (FAT12, FAT32, Linux, etc.)
- Start sector (LBA)
- Size in megabytes

#### Available Actions

1. **Create new partition**
   - Automatically finds free space on the disk
   - Creates a new partition in the next available slot (max 4 for MBR)
   - Uses Linux filesystem type by default
   - Requires confirmation

2. **Delete partition**
   - Deletes the last partition from the partition table
   - Requires explicit confirmation to prevent accidental data loss
   - Does not affect other partitions

3. **Refresh partition list**
   - Reloads the partition table from disk
   - Useful after making changes or if the display is corrupted

4. **Exit to shell**
   - Returns to the command prompt

### Navigation

- **UP/DOWN arrows** - Select menu items
- **ENTER** - Activate selected action
- **ESC** - Exit to shell
- **Y/N** - Confirm or cancel destructive operations

### Partition Table Limits

- **MBR:** Maximum 4 primary partitions
- **GPT:** Up to 128 partitions (stub implementation)

## Safety Features

### Confirmation Dialogs

All destructive operations require explicit confirmation:
- Formatting disks
- Deleting partitions
- Creating new partition tables

You must press **Y** to proceed or **N** to cancel.

### Non-Destructive Testing

When testing in QEMU, you can:
1. Create a separate test disk image
2. Run the installer on the test disk
3. Keep your primary `os.img` intact

Example:
```bash
# Create a test disk
dd if=/dev/zero of=test_disk.img bs=1M count=500

# Boot with test disk as secondary device
qemu-system-i386 -kernel dist/kernel.elf \
    -drive format=raw,file=dist/os.img,if=ide \
    -drive format=raw,file=test_disk.img,if=ide
```

## Installing to USB Drive

**WARNING: This will COMPLETELY ERASE the USB drive!**

### Preparation

1. Build the AltoniumOS ISO:
   ```bash
   make iso-hybrid
   ```

2. Identify your USB drive:
   ```bash
   lsblk
   # Look for your USB device, e.g., /dev/sdb
   ```

3. Write the ISO to USB:
   ```bash
   sudo dd if=dist/os-hybrid.iso of=/dev/sdX bs=4M status=progress conv=fsync
   # Replace /dev/sdX with your actual USB device
   ```

### Installing to the USB from AltoniumOS

1. Boot AltoniumOS from CD/ISO
2. The USB drive should appear in the disk selection menu
3. Run `install` and select the USB drive as the target
4. Follow the wizard steps
5. The USB will be bootable after completion

## Integrity Checks

### Verifying Disk Writes

After installation, you can verify the partition table:

```bash
# On Linux host
sudo fdisk -l /dev/sdX

# Or in AltoniumOS
> diskpart
# Check that partitions are listed correctly
```

### Filesystem Verification

For FAT32 partitions:
```bash
# On Linux host
sudo fsck.vfat /dev/sdX1
```

For ext2 partitions:
```bash
# On Linux host
sudo fsck.ext2 /dev/sdX1
```

## Troubleshooting

### Installer Not Starting

**Problem:** Installer command does nothing
**Solution:** Ensure the kernel is built with all installer modules:
```bash
make clean
make build
```

### No Disks Detected

**Problem:** Disk selection shows "No storage devices detected"
**Solutions:**
- Check that a disk is attached in QEMU: `-drive format=raw,file=disk.img,if=ide`
- For real hardware, ensure BIOS detects the disk
- Try `storage` command to see detected controllers

### Partition Creation Fails

**Problem:** "Failed to create partition" error
**Solutions:**
- Check if disk has free space
- Verify MBR signature is valid (0xAA55)
- Try `diskpart` to manually inspect partition table
- Maximum 4 partitions for MBR (check with `diskpart`)

### Formatting Errors

**Problem:** Filesystem format fails or is incomplete
**Solutions:**
- FAT12: Uses existing formatter, should work reliably
- FAT32: Basic implementation, may not support all volumes
- ext2: Stub implementation, creates basic superblock only
- Check disk write permissions in QEMU

## Advanced Usage

### Scripted Installation

You can automate installation using expect scripts:

```bash
./test_installer.sh
```

This runs through a complete installation workflow for testing.

### Custom Partition Sizes

The current version creates partitions with default sizes (approx 500 MB). To customize:

1. Use `diskpart` to view current layout
2. Manually calculate free space
3. Edit partition table helpers in `lib/partition_table.c`

### Multiple Partitions

To create multiple partitions:

1. Run `diskpart`
2. Select "Create new partition" repeatedly (up to 4 times for MBR)
3. Each partition will use the next available free space
4. Use different filesystem types for each partition

### Dual-Boot Setup

To set up dual-boot with another OS:

1. Install the other OS first
2. Leave unpartitioned space
3. Boot AltoniumOS from CD/USB
4. Run `diskpart` and create partition in free space
5. The existing OS bootloader should detect AltoniumOS

**Note:** Bootloader integration is limited in this version.

## Current Limitations

### File Copy

- File copying is stubbed in this version
- The installer shows intent but does not copy actual files
- For real installations, files must be copied manually or with updated code

### Bootloader Installation

- Bootloader installation is stubbed
- Boot sector writing is not implemented
- For bootable installs, use `dd` or GRUB to install bootloader

### GPT Support

- GPT is recognized but not fully implemented
- Stick to MBR for production use

### Filesystem Support

- FAT12: Full support (existing implementation)
- FAT32: Basic formatting only, no journaling
- ext2: Stub implementation, superblock and group descriptors only

## Testing Checklist

Before deploying on real hardware, test in QEMU:

- [ ] Run `install` and complete all wizard steps
- [ ] Run `diskpart` and create a partition
- [ ] Run `diskpart` and delete a partition
- [ ] Verify confirmation dialogs appear for destructive operations
- [ ] Test ESC key to cancel operations
- [ ] Check that partition table is correctly written (use Linux `fdisk`)
- [ ] Run test scripts: `./test_installer.sh` and `./test_diskpart.sh`

## See Also

- `README.md` - General AltoniumOS documentation
- `QUICK_START.md` - Quick start guide
- `HYBRID_ISO_GUIDE.md` - Creating bootable ISO images
- `STORAGE_HAL_IMPLEMENTATION.md` - Storage driver details

## Support

For issues or questions:
- Check existing documentation in the repository
- Review error messages carefully
- Test in QEMU before attempting real hardware installation
- Ensure all dependencies are installed (`make`, `gcc`, `nasm`, `qemu`)
