# UEFI Boot Guide for AltoniumOS

## Overview

AltoniumOS supports UEFI boot on modern hardware including AMD E1-7010 systems. This guide explains how to build, test, and troubleshoot UEFI boot.

## Architecture

The UEFI boot process consists of three components:

1. **UEFI Bootloader** (`EFI/BOOT/BOOTX64.EFI`) - First stage bootstrap
   - Runs in 64-bit UEFI environment
   - Initializes console output (ASCII only)
   - Loads GRUB chainloader
   - Provides detailed diagnostic messages

2. **GRUB Bootloader** (`EFI/ALTONIUM/GRUBX64.EFI`) - Second stage
   - Standalone GRUB EFI image
   - Uses Multiboot protocol
   - Loads 32-bit kernel
   - Passes boot parameters

3. **32-bit Kernel** (`boot/kernel.elf`) - Operating system
   - Runs in 32-bit protected mode
   - VGA console with theme support
   - FAT12 filesystem
   - All OS commands and features

## Building UEFI ISO

### Prerequisites

```bash
sudo apt-get install gnu-efi grub-efi-amd64-bin xorriso ovmf
```

### Build Commands

```bash
# Build UEFI bootable ISO
make iso-uefi

# Build both BIOS and UEFI ISOs
make iso

# Clean and rebuild everything
make clean && make iso-uefi
```

### Build Output

- `dist/os-uefi.iso` - UEFI bootable ISO image
- `dist/EFI/BOOT/BOOTX64.EFI` - First stage bootloader
- `dist/EFI/ALTONIUM/GRUBX64.EFI` - GRUB chainloader
- `dist/kernel.elf` - 32-bit kernel

## Testing with QEMU

### Basic UEFI Boot Test

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom dist/os-uefi.iso
```

### With Serial Console (for debugging)

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom dist/os-uefi.iso -serial stdio
```

### With Memory Debugging

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom dist/os-uefi.iso -m 512M -no-reboot -no-shutdown
```

## Writing to USB for Real Hardware

### Linux

```bash
# Identify USB device (e.g., /dev/sdb)
lsblk

# Write ISO to USB (REPLACE /dev/sdX with your device!)
sudo dd if=dist/os-uefi.iso of=/dev/sdX bs=4M status=progress conv=fsync

# Sync to ensure all data is written
sync
```

### Windows

Use tools like:
- Rufus (recommended)
- Etcher
- Win32 Disk Imager

**Settings for Rufus:**
- Partition scheme: GPT
- Target system: UEFI (non CSM)
- File system: FAT32
- Write in DD mode

## Booting on Real Hardware (AMD E1-7010)

### BIOS Settings

1. Enter BIOS/UEFI setup (usually F2, F10, or Del during boot)
2. Disable Secure Boot (required for unsigned bootloaders)
3. Enable USB Boot
4. Set boot mode to UEFI (not Legacy/CSM)
5. Set USB as first boot device
6. Save and exit

### Boot Process

1. Insert USB drive
2. Power on or reboot
3. Select UEFI boot entry (not Legacy)
4. You should see:
   ```
   ======================================
   AltoniumOS UEFI Bootstrap v1.0
   AMD E1-7010 Compatible
   ======================================
   
   [UEFI] System Table initialized
   [UEFI] Boot Services available
   [UEFI] Console output ready
   
   [UEFI] Boot device handle acquired
   [UEFI] Attempting to load GRUB bootloader
   [UEFI] File system protocol acquired
   [UEFI] Root volume opened
   [UEFI] Opening file: \EFI\ALTONIUM\GRUBX64.EFI
   [UEFI] File opened successfully
   [UEFI] File loaded successfully
   [UEFI] Loading GRUB image into memory
   [UEFI] GRUB image loaded successfully
   
   [UEFI] Launching GRUB bootloader
   [UEFI] Transferring control to GRUB
   ======================================
   ```

5. GRUB loads and immediately boots kernel
6. AltoniumOS welcome screen appears:
   ```
   Welcome to AltoniumOS 1.0.0
   Boot mode: UEFI
   Initializing disk driver... OK
   Running disk self-test... OK
   Initializing FAT12 filesystem... OK
   Mounted volume at /
   Type 'help' for available commands
   ```

## Troubleshooting

### Issue: Console Shows Garbage Characters

**Symptoms:**
- Question marks (?)
- Square boxes (□)
- Random symbols

**Cause:** UTF-8 encoding or extended characters

**Solution:** The UEFI bootloader now uses ASCII-only output (fixed in v1.0)
- No ellipsis (...) characters
- No UTF-8 encoded characters
- Only characters in range 0x20-0x7E

### Issue: System Reboots to Main Disk

**Symptoms:**
- USB boots briefly then reboots to Windows/Linux
- "No bootable device" message
- Falls back to internal disk

**Causes:**
1. GRUB file not found
2. Incorrect ISO layout
3. Secure Boot enabled
4. Wrong boot mode selected

**Solutions:**

1. **Verify ISO structure:**
   ```bash
   isoinfo -l -i dist/os-uefi.iso | grep -E "(BOOTX64|GRUBX64|kernel)"
   ```
   Should show:
   - `/EFI/BOOT/BOOTX64.EFI`
   - `/EFI/ALTONIUM/GRUBX64.EFI`
   - `/boot/kernel.elf`

2. **Disable Secure Boot** in BIOS

3. **Select correct boot entry:**
   - Choose "UEFI: [USB Drive Name]"
   - NOT "USB: [Drive Name]" (Legacy mode)

4. **Rebuild ISO:**
   ```bash
   make clean && make iso-uefi
   ```

5. **Rewrite USB:**
   ```bash
   sudo dd if=dist/os-uefi.iso of=/dev/sdX bs=4M conv=fsync status=progress
   ```

### Issue: Bootloader Hangs

**Symptoms:**
- Screen stays blank
- No output after UEFI logo
- System appears frozen

**Solutions:**

1. **Wait 10 seconds** - Some firmware is slow to initialize

2. **Check OVMF path** (QEMU only):
   ```bash
   ls /usr/share/OVMF/OVMF_CODE*.fd
   # Try different version if available
   ```

3. **Try verbose mode** (QEMU):
   ```bash
   qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd \
     -cdrom dist/os-uefi.iso -serial stdio -D qemu.log
   ```

4. **Real hardware:** Press Esc or F2 to see firmware messages

### Issue: GRUB Not Found Error

**Symptoms:**
```
[UEFI] FATAL: Failed to load GRUB bootloader
[UEFI] Please check:
  1. GRUB is at: \EFI\ALTONIUM\GRUBX64.EFI
  2. ISO was created correctly
  3. USB was written properly
```

**Solutions:**

1. **Verify GRUB was built:**
   ```bash
   ls -lh dist/EFI/ALTONIUM/GRUBX64.EFI
   # Should be 1-3 MB
   ```

2. **Check grub-mkstandalone is installed:**
   ```bash
   which grub-mkstandalone
   sudo apt-get install grub-efi-amd64-bin
   ```

3. **Manual GRUB build:**
   ```bash
   grub-mkstandalone -O x86_64-efi \
     -o dist/EFI/ALTONIUM/GRUBX64.EFI \
     "boot/grub/grub.cfg=grub-uefi.cfg"
   ```

4. **Rebuild ISO:**
   ```bash
   make clean
   make iso-uefi
   ```

### Issue: Kernel Doesn't Load

**Symptoms:**
- GRUB loads successfully
- Screen clears then nothing happens
- Or immediate reboot

**Solutions:**

1. **Verify kernel.elf exists:**
   ```bash
   ls -lh dist/kernel.elf
   file dist/kernel.elf
   # Should show: ELF 32-bit LSB executable, Intel 80386
   ```

2. **Check GRUB config:**
   ```bash
   cat grub-uefi.cfg
   ```
   Should contain:
   ```
   menuentry "AltoniumOS (UEFI)" {
       multiboot /boot/kernel.elf bootmode=uefi
   }
   ```

3. **Verify Multiboot header:**
   ```bash
   hexdump -C dist/kernel.elf | head -20 | grep "1b ad b0 02"
   ```
   Should find the Multiboot magic number

4. **Rebuild kernel:**
   ```bash
   make clean
   make build
   make iso-uefi
   ```

## Memory Map Issues

If you see memory-related errors:

1. **Insufficient memory:**
   - QEMU: `-m 512M` (minimum 256M)
   - Real hardware: Verify RAM working

2. **Memory overlap:**
   - Kernel loads at 0x10000 (64KB)
   - Stack at 0x800000 (8MB)
   - Should not conflict with UEFI memory map

3. **Page table issues:**
   - GRUB sets up initial page tables
   - Kernel runs in 32-bit protected mode (no paging)

## Boot Verification Checklist

- [ ] UEFI bootloader loads and displays banner
- [ ] Console shows ASCII text (no garbage)
- [ ] File system protocol initialized
- [ ] GRUB file found and loaded
- [ ] GRUB starts successfully
- [ ] Kernel loads via Multiboot
- [ ] Welcome message displays
- [ ] Disk driver initializes
- [ ] FAT12 filesystem mounts
- [ ] Command prompt appears
- [ ] Commands work (help, clear, echo, fetch)
- [ ] No random reboots

## Advanced Debugging

### Enable UEFI Debug Messages

Edit `bootloader/uefi_loader.c` and add more debug output:

```c
print_ascii(L"[DEBUG] Before operation X\r\n");
// ... your code ...
print_ascii(L"[DEBUG] After operation X\r\n");
```

Then rebuild:
```bash
make clean && make iso-uefi
```

### QEMU GDB Debugging

```bash
# Terminal 1: Start QEMU with GDB stub
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd \
  -cdrom dist/os-uefi.iso -s -S

# Terminal 2: Connect GDB
gdb dist/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### Serial Console Capture

```bash
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd \
  -cdrom dist/os-uefi.iso -serial file:serial.log

# Check output
cat serial.log
```

## Known Limitations

1. **32-bit kernel on 64-bit UEFI:**
   - GRUB handles the mode switch
   - Multiboot protocol enables compatibility
   - Not all UEFI features available to kernel

2. **No Secure Boot:**
   - Bootloader is not signed
   - Must disable Secure Boot in firmware
   - Not suitable for secure environments (yet)

3. **No UEFI GOP:**
   - Kernel uses VGA text mode
   - No graphical framebuffer
   - 80x25 character display only

4. **No UEFI runtime services:**
   - Kernel cannot call UEFI after boot
   - Time/date not available from firmware
   - Must use CMOS/RTC for time

## Success Indicators

### QEMU Test

Should boot in under 2 seconds and display full console.

### Real Hardware Test

1. USB LED activity during boot
2. UEFI bootstrap banner appears
3. GRUB loads quickly (< 1 second)
4. Kernel boots and shows welcome message
5. Commands respond immediately
6. No timeouts or hangs

## Getting Help

If you still have issues:

1. Check build output for errors:
   ```bash
   make clean && make iso-uefi 2>&1 | tee build.log
   ```

2. Verify ISO structure:
   ```bash
   isoinfo -l -i dist/os-uefi.iso > iso_contents.txt
   ```

3. Test in QEMU first before real hardware

4. Document exact error messages and boot behavior

5. Note hardware model and BIOS version

## References

- UEFI Specification: https://uefi.org/specifications
- GNU-EFI Documentation: https://sourceforge.net/projects/gnu-efi/
- GRUB Manual: https://www.gnu.org/software/grub/manual/
- Multiboot Specification: https://www.gnu.org/software/grub/manual/multiboot/
