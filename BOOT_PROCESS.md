# Boot Process Documentation

This document provides a detailed explanation of the boot process for this Multiboot-compliant 32-bit kernel.

## Overview

The boot process follows the Multiboot specification, allowing the kernel to be loaded by GRUB or other Multiboot-compliant bootloaders. The bootstrap code in `boot/boot.asm` performs minimal initialization before transferring control to the C kernel.

## Boot Sequence

### 1. Bootloader Stage

When the system boots:
1. BIOS loads the bootloader (GRUB) from the boot device
2. GRUB reads the filesystem and locates the kernel binary
3. GRUB scans the first 8 KB of the kernel for the Multiboot header
4. If found, GRUB loads the kernel at the 1 MB mark (0x00100000)
5. GRUB switches to 32-bit protected mode
6. GRUB sets up:
   - EAX = Multiboot magic number (0x2BADB002)
   - EBX = Physical address of Multiboot information structure
   - CS = Code segment with base 0, limit 4GB
   - DS, ES, FS, GS, SS = Data segment with base 0, limit 4GB
7. GRUB jumps to the kernel entry point (`_start`)

### 2. Bootstrap Stage (`boot/boot.asm`)

#### Step 1: Disable Interrupts
```asm
cli
```
Interrupts are disabled to prevent any interrupts from occurring during the initialization process, which could cause undefined behavior.

#### Step 2: Load GDT
```asm
lgdt [gdt_descriptor]
```
The Global Descriptor Table (GDT) is loaded with a minimal flat memory model:
- **Null Descriptor** (offset 0x00): Required by x86 architecture, never used
- **Code Segment** (offset 0x08): Base 0, limit 4GB, 32-bit, executable, readable
- **Data Segment** (offset 0x10): Base 0, limit 4GB, 32-bit, writable

This sets up a flat memory model where all segments cover the entire 4GB address space.

#### Step 3: Update Segment Registers
```asm
mov ax, DATA_SEG
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
```
All data segment registers are updated to use our new data segment descriptor.

#### Step 4: Far Jump to Flush CS
```asm
jmp CODE_SEG:.flush_cs
```
A far jump is necessary to update the CS (code segment) register with our new code segment descriptor. This cannot be done with a simple `mov` instruction.

#### Step 5: Initialize Stack
```asm
mov esp, stack_top
mov ebp, esp
```
The stack pointer (ESP) is set to the top of a 16 KB stack allocated in the BSS section. EBP is also initialized to the same value for proper stack frame tracking.

#### Step 6: Zero BSS Section
```asm
mov edi, __bss_start
mov ecx, __bss_end
sub ecx, edi
xor eax, eax
rep stosb
```
The BSS (Block Started by Symbol) section contains uninitialized global and static variables. The C standard requires these to be initialized to zero. The linker script provides `__bss_start` and `__bss_end` symbols.

#### Step 7: Pass Multiboot Parameters
```asm
push ebx
push eax
```
The Multiboot magic number (EAX) and information structure pointer (EBX) are pushed onto the stack as arguments to `kernel_main`.

#### Step 8: Call Kernel Main
```asm
call kernel_main
```
Control is transferred to the C kernel code.

#### Step 9: Halt on Return
```asm
cli
.hang:
    hlt
    jmp .hang
```
If `kernel_main` ever returns (which it shouldn't), the system is halted. The loop is necessary because some hardware may continue execution after `hlt` due to interrupts or other events.

### 3. Kernel Stage (`kernel/main.c`)

The kernel entry point receives two parameters:
- `multiboot_magic`: Should be 0x2BADB002 if loaded by a Multiboot bootloader
- `multiboot_info`: Pointer to the Multiboot information structure

The current implementation:
1. Clears the VGA text buffer
2. Displays a boot message
3. Enters an infinite halt loop

## Memory Layout

```
0x00000000 - 0x000003FF: Real Mode IVT (not used in protected mode)
0x00000400 - 0x000004FF: BIOS Data Area
0x00000500 - 0x00007BFF: Unused
0x00007C00 - 0x00007DFF: Bootloader
0x00007E00 - 0x0009FFFF: Free
0x000A0000 - 0x000BFFFF: Video memory
0x000C0000 - 0x000FFFFF: BIOS ROM
0x00100000 - 0x0010000B: .multiboot section (12 bytes)
0x00101000 - 0x00101092: .text section (code)
0x00102000 - 0x00102047: .rodata section (read-only data)
0x00103000 - 0x0010301D: .data section (GDT + initialized data)
0x00104000 - 0x00107FFF: .bss section (stack, 16 KB)
0x00108000+           : Heap (not yet implemented)
```

## GDT Structure

The GDT is defined in `boot/boot.asm` and contains:

| Offset | Descriptor | Base | Limit    | Access | Flags |
|--------|------------|------|----------|--------|-------|
| 0x00   | Null       | 0    | 0        | 0x00   | 0x00  |
| 0x08   | Code       | 0    | 0xFFFFF  | 0x9A   | 0xCF  |
| 0x10   | Data       | 0    | 0xFFFFF  | 0x92   | 0xCF  |

### Access Byte Breakdown

**Code Segment (0x9A)**:
- Bit 7: Present = 1
- Bits 6-5: DPL (privilege level) = 0 (kernel)
- Bit 4: Descriptor type = 1 (code/data)
- Bit 3: Executable = 1
- Bit 2: Direction/Conforming = 0
- Bit 1: Readable = 1
- Bit 0: Accessed = 0

**Data Segment (0x92)**:
- Bit 7: Present = 1
- Bits 6-5: DPL = 0 (kernel)
- Bit 4: Descriptor type = 1 (code/data)
- Bit 3: Executable = 0
- Bit 2: Direction/Conforming = 0
- Bit 1: Writable = 1
- Bit 0: Accessed = 0

### Flags Byte Breakdown (0xCF)

- Bit 7: Granularity = 1 (4 KB blocks, so limit is 4 GB)
- Bit 6: Size = 1 (32-bit)
- Bit 5: Long mode = 0 (not 64-bit)
- Bit 4: Reserved = 0
- Bits 3-0: Limit bits 16-19 = 0xF

## Multiboot Header

The Multiboot header is placed at the beginning of the kernel binary:

```
Offset  | Size | Field
--------|------|------------------
0       | 4    | Magic (0x1BADB002)
4       | 4    | Flags (0x00000003)
8       | 4    | Checksum
```

**Flags** (bits):
- Bit 0: Page align all boot modules
- Bit 1: Provide memory information
- Bit 16: Use addresses in Multiboot header (not used)

**Checksum**: Must satisfy the equation:
```
magic + flags + checksum = 0
checksum = -(magic + flags)
checksum = -(0x1BADB002 + 0x00000003) = 0xE4524FFB
```

## Testing and Verification

### QEMU Testing
```bash
make run
# or
qemu-system-i386 -kernel kernel.bin
```

QEMU can directly load Multiboot kernels without needing GRUB.

### GRUB Testing
```bash
make iso
qemu-system-i386 -cdrom myos.iso
```

This creates a bootable ISO with GRUB as the bootloader.

### Real Hardware
1. Create ISO: `make iso`
2. Write to USB: `dd if=myos.iso of=/dev/sdX bs=4M`
3. Boot from USB

### Debugging
```bash
# Start QEMU with GDB server
qemu-system-i386 -kernel kernel.bin -s -S

# In another terminal
gdb kernel.bin
(gdb) target remote localhost:1234
(gdb) break _start
(gdb) continue
(gdb) step
```

## Compatibility Notes

### GRUB Compatibility
- ✓ GRUB Legacy (0.97)
- ✓ GRUB 2 (all versions)
- ✓ Other Multiboot-compliant bootloaders

### QEMU Compatibility
- ✓ QEMU 2.0+
- ✓ Direct kernel loading with `-kernel`
- ✓ ISO boot with `-cdrom`

### Hardware Compatibility
- ✓ Legacy BIOS systems
- ✓ Modern systems with CSM (Compatibility Support Module)
- ✗ UEFI-only systems (requires UEFI bootloader)

## References

- [Multiboot Specification v0.6.96](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [Intel® 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [OSDev Wiki - GDT](https://wiki.osdev.org/GDT)
- [OSDev Wiki - Multiboot](https://wiki.osdev.org/Multiboot)
