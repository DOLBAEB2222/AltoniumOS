# AltoniumOS - A Simple Console-Based Operating System

A minimal x86 operating system with console commands for system interaction and information retrieval.

## Features

This OS includes the following console commands:

- **clear** – Clear the VGA text buffer and reset the screen
- **echo** – Write text to the console
- **fetch** – Display OS and system information (name, version, architecture, build date/time, boot mode, detected memory)
- **bootlog** – Display detailed BIOS boot diagnostics (EDD support, boot method, memory, geometry)
- **disk** – Test raw disk I/O and show sector diagnostics
- **storage** – List detected storage controllers and their capabilities (PCI enumeration results)
- **beep <F> <T>** – Play a single tone (frequency in Hz for F milliseconds T)
- **beep melody NOTE:MS,...** – Play a melody (e.g., `beep melody C4:250,E4:250,G4:500`)
- **beep piano** – Enter interactive piano mode (press letter keys to play, ESC to exit)
- **ls [PATH]** – List files and directories from the current working directory or a supplied path
- **dir [PATH]** – Alias for ls, list directory contents
- **pwd** – Display the current working directory
- **cd PATH** – Change the current working directory (supports absolute/relative paths and `..`)
- **cat FILE** – Dump the contents of a file stored on the FAT12 volume
- **touch FILE** – Create a zero-length file
- **write FILE TEXT** – Create or overwrite an 8.3 text file with the provided content
- **mkdir NAME** – Create a directory inside the current working directory
- **rm FILE** – Delete a file from the current working directory
- **nano FILE** – Simple text editor with full-screen editing (Ctrl+S to save, Ctrl+X to exit)
- **theme [OPTION]** – Switch color theme (normal/blue/green) or 'list' to show available themes
- **shutdown** – Gracefully shut down the system (attempts ACPI power-off via port 0x604)
- **help** – Display all available commands and usage hints

### Storage Hardware Abstraction Layer (Storage HAL)

AltoniumOS includes a unified Storage HAL that enumerates and manages multiple storage controllers:

#### Device Detection Order
The storage manager initializes controllers in priority order:
1. **NVMe (Highest Priority)** – Modern NVMe devices (class 0x01, subclass 0x08)
2. **AHCI (Middle Priority)** – SATA AHCI controllers (class 0x01, subclass 0x06)
3. **Legacy ATA PIO (Fallback)** – Primary IDE channel (I/O ports 0x1F0-0x1F7)

This ensures the fastest available storage device is used as the primary boot disk.

#### PCI Configuration Helper
- **PCI enumeration** via configuration mechanism 1
- **Device discovery** of class 0x01 (Mass Storage) devices
- **BAR (Base Address Register) detection** for memory-mapped controller access
- Supports up to 256 PCI devices across all buses

#### AHCI Support (Stub)
- **Detection** of class 0x01/0x06 AHCI-mode SATA controllers
- **HBA (Host Bus Adapter) initialization** with BAR mapping
- **Port enumeration** and basic initialization
- **Read/Write callbacks** through the block device abstraction
- Current limitation: Polling mode stub (full DMA implementation pending)

#### NVMe Support (Stub)
- **Detection** of class 0x01/0x08 NVMe devices
- **Admin queue** initialization stub
- **IDENTIFY command** support for device discovery
- **4 KiB logical block addressing** capability
- **Read-only access** support in current release
- Current limitation: Basic stub implementation (full namespace support pending)

#### Block Device Abstraction
- **Unified interface** for read/write operations across all storage types
- **Sector size tracking** (512B for ATA/AHCI, 4KiB for NVMe)
- **Capacity tracking** for multi-device systems
- **Driver metadata** including queue depth and device type
- Used by FAT12 filesystem for transparent device access

#### Disk I/O Features

The OS includes an ATA PIO driver for disk I/O operations:

- Primary IDE channel support (I/O ports 0x1F0-0x1F7)
- LBA addressing for up to 28-bit sector addresses
- 512-byte sector read/write operations
- Multi-sector read/write support
- Built-in disk self-test and validation
- Boot sector signature detection (0x55AA)

### FAT12 Filesystem Layer

On top of the ATA driver, AltoniumOS now mounts a FAT12 volume during boot:

- Parses the BIOS Parameter Block and caches both FAT copies and the root directory
- Computes root/data offsets for a 10 MB, 8-sector-per-cluster FAT12 layout (128 reserved sectors keep the kernel contiguous)
- Seeds the disk image with sample content (`README.TXT`, `SYSTEM.CFG`, and `DOCS/INFO.TXT`)
- Shell commands (`ls`, `pwd`, `cd`, `cat`, `write`, `mkdir`, `rm`) call into the FAT12 core for traversal and file manipulation
- Every update touches both FAT copies and flushes directory metadata back to disk
- Limitations: 8.3 uppercase filenames, small text-only writes via the shell (16 KB buffer), and simple error handling (invalid names, disk full, non-directory targets)

### Theme System

AltoniumOS includes a theme system that allows you to customize the console appearance:

- **Three built-in themes:**
  - **normal** – Classic white text on black background (default)
  - **blue** – White text on blue background with cyan/yellow status bar
  - **green** – Light green text on black background with green status bar

- **Theme persistence:** The selected theme persists during the current session and applies to all screens including the nano text editor

- **Switch themes:** Use the `theme` command:
  ```bash
  theme normal      # Switch to normal theme
  theme blue        # Switch to blue theme
  theme green       # Switch to green theme
  theme list        # Show all available themes
  theme             # Show current theme
  ```

- **Nano editor integration:** The current theme is displayed in the nano status bar and automatically applied to all nano editor sessions

## Building

### Prerequisites

- `nasm` (Netwide Assembler) for assembling x86 code
- `gcc` with i386 support (32-bit) for compiling C code
- `binutils` (ld, objcopy) for linking and binary conversion
- `python3` (for generating the FAT12 disk image)
- `make` for building
- `qemu-system-i386` for testing (optional)
- `gnu-efi`, `grub-efi-amd64-bin`, and `ovmf` for building and testing the UEFI boot path

### Build Instructions

```bash
# Build the kernel ELF binary
make build

# Create hybrid BIOS/UEFI ISO image (RECOMMENDED - boots on both firmware types)
make iso-hybrid

# Or build using the simpler 'iso' target (now defaults to hybrid)
make iso

# Legacy separate ISO targets (still available):
make iso-bios    # BIOS-only ISO image
make iso-uefi    # UEFI-only ISO image

# Create the FAT12 disk image used by legacy BIOS boot
make img

# Clean build artifacts
make clean
```

## Running the OS

### With QEMU (Recommended)

```bash
# Run the kernel directly with multiboot support (recommended)
qemu-system-i386 -kernel dist/kernel.elf

# Run from a bootable disk image (real hardware simulation)
qemu-system-i386 -drive file=dist/os.img,format=raw

# Run with disk I/O support (for testing disk commands)
qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide

# Alternative disk image boot
qemu-system-i386 -hda dist/os.img

# For debugging with GDB
qemu-system-i386 -kernel dist/kernel.elf -s -S &
gdb dist/kernel.elf -ex "target remote :1234"
```

### Booting with UEFI firmware and Hybrid ISOs

The project includes a **hybrid BIOS/UEFI ISO** (`make iso-hybrid`) that boots on both legacy BIOS and modern UEFI systems without any manual configuration. The hybrid ISO contains:

- **BIOS boot path:** GRUB with El Torito boot sector
- **UEFI boot path:** Native UEFI bootstrapper (`EFI/BOOT/BOOTX64.EFI`) + GRUB EFI loader
- **Multiple kernel targets:** x86 32-bit kernel and x64 placeholder
- **Interactive menu:** Choose between BIOS/UEFI boot modes and architectures

```bash
# Build the hybrid ISO (RECOMMENDED)
make iso-hybrid

# Test in BIOS mode with QEMU
qemu-system-i386 -cdrom dist/os-hybrid.iso

# Test in UEFI mode with QEMU + OVMF firmware
qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom dist/os-hybrid.iso

# Write to USB flash drive for hardware testing (replace /dev/sdX)
# This single image boots on BOTH BIOS and UEFI systems!
sudo dd if=dist/os-hybrid.iso of=/dev/sdX bs=4M status=progress conv=fsync
```

On a physical machine, the firmware will automatically detect whether you're booting in BIOS or UEFI mode. In both cases, you'll see the GRUB menu allowing you to select the appropriate kernel configuration.

**UEFI Hardware Compatibility:**
- Console output uses pure ASCII characters (no UTF-8 or extended characters) for maximum compatibility
- Comprehensive diagnostics and memory map reporting
- Automatic verification of kernel.elf and GRUB presence before boot
- Detailed error messages with wait-for-key on failures (prevents unexpected reboots)
- Tested on AMD E1-7010 systems and OVMF firmware
- See `UEFI_BOOT_GUIDE.md` for detailed troubleshooting information

### BIOS Boot Troubleshooting

For systems that experience BIOS boot issues (such as Lenovo 110-15ACL or systems with Alder Lake processors), the OS includes enhanced boot diagnostics and a safe mode entry:

**Boot Diagnostics:**
The `bootlog` command displays detailed BIOS boot information:
```
bootlog
Boot diagnostics:
  Extensions:    EDD supported
  Boot method:   EDD
  INT13 status:  0x00
  Retry count:   0
  Memory:        8192 MB
  Cylinders:     1024
  Heads:         16
  Sectors/track: 63
  BIOS vendor:   [vendor string]
```

**Safe BIOS Mode:**
If you experience disk read errors on problematic hardware, use the "Safe BIOS Mode" GRUB entry:
- Select "AltoniumOS - BIOS Safe Mode (No EDD)" from the GRUB menu
- This disables INT 13h extensions and uses traditional CHS addressing
- Useful for older BIOS implementations or firmware bugs related to EDD

**EDD (Enhanced Disk Drive) Support:**
- Modern systems use INT 13h extensions (AH=42h) for larger disk support
- Older systems or problematic BIOS implementations fall back to CHS addressing
- The bootlog shows which path was used and any error codes encountered
- If "Boot method: Error" appears, check BIOS settings for disk controllers or try Safe BIOS Mode

**Memory Detection:**
The kernel detects system memory during boot using INT 15h E820 calls and reports the total available memory (up to 32 GB with PAE support in future versions).

### Testing Disk I/O

To test the disk I/O functionality:

1. **Boot with disk support:**
   ```bash
   qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide
   ```

2. **At the command prompt, type:**
   ```
   disk
   ```

3. **Expected output:**
   - Disk initialization status
   - Sector 0 read test
   - First 64 bytes hex dump of MBR
   - Disk self-test results

The disk driver will automatically initialize during boot and report any errors if the disk cannot be detected.

### Working with the FAT12 volume

Once the disk driver is online, the FAT12 layer is mounted automatically. The bundled `dist/os.img` contains a few helper files you can experiment with:

```text
ls
[DIR] DOCS
      README.TXT (92 bytes)
      SYSTEM.CFG (42 bytes)
```

```
cd docs
pwd
/DOCS
cat INFO.TXT
Sample notes live inside DOCS/INFO.TXT
```

The `write` command stores the remainder of the line as file contents (up to ~16 KB per write), `mkdir` creates new directories, and `rm` deletes regular files. All filenames must follow the DOS 8.3 convention (uppercase letters, numbers, `_` or `-`).

### Nano Text Editor

AltoniumOS includes a simple nano-like text editor with the following features:

- **Full-screen editing** with viewport scrolling
- **Line-based text buffer** supporting up to 1000 lines of 200 characters each  
- **Cursor navigation** using arrow keys
- **Text editing** with character insertion, backspace, and Enter for new lines
- **File operations** via FAT12 filesystem integration
- **Status bar** showing filename and modification state

#### Usage

```bash
nano filename.txt
```

#### Controls

- **Arrow Keys** - Move cursor up/down/left/right
- **Text Characters** - Insert at cursor position
- **Enter** - Create new line (splits current line)
- **Backspace** - Delete character before cursor or join lines
- **'s' at start of empty file** - Save file
- **'x' at start of empty file** - Exit editor

#### Example Workflow

```bash
# Create or edit a file
nano README.TXT

# Type your content using the keyboard
# Navigate with arrow keys
# Press 's' at beginning of empty file to save
# Press 'x' at beginning of empty file to exit

# Verify the file was saved
cat README.TXT
```

The editor automatically creates new files if they don't exist and saves changes to the FAT12 filesystem when you exit.

### Multiboot Support

AltoniumOS includes a Multiboot header compliant with the Multiboot specification. This allows it to be loaded by:
- QEMU's `-kernel` option
- GRUB bootloader
- Other Multiboot-compliant boot loaders

The Multiboot header is located at the beginning of the kernel image and contains:
- Magic number: 0x1BADB002
- Flags: 0x00000003
- Checksum: -(magic + flags)

### Scripted Commands via QEMU

Commands can be batched and executed programmatically using QEMU's keyboard input simulation. This is useful for automated testing and demonstrations.

#### Method 1: Using QEMU Monitor Commands

```bash
# Run QEMU with monitor on stdio
qemu-system-i386 -kernel dist/kernel.elf -monitor stdio

# In QEMU monitor, simulate keyboard input
(qemu) sendkey h e l p Return
(qemu) sendkey c l e a r Return
(qemu) sendkey e c h o space H e l l o Return
```

#### Method 2: Using QMP (QEMU Machine Protocol)

```bash
# Start QEMU with QMP socket
qemu-system-i386 -kernel dist/kernel.elf -qmp unix:/tmp/qemu-socket,server,nowait &

# Send commands via QMP (requires jq and socat)
echo '{"execute": "input-send-event", "arguments": {"device": "ps2kbd", "events": [{"type": "key", "data": {"key": "h"}}]}}' | socat - UNIX-CONNECT:/tmp/qemu-socket
```

#### Method 3: Using sendkey in QEMU Interactive Monitor

```bash
# Connect to QEMU monitor via telnet or console
# Type commands directly:
sendkey shift+h
sendkey e l p Return
```

#### Method 4: Sending Keystrokes via Python/Expect

Example Python script to automate commands:

```python
#!/usr/bin/env python3
import socket
import json
import time

def send_qemu_command(sock, *keys):
    """Send keyboard commands to QEMU via monitor"""
    for key in keys:
        cmd = f'sendkey {key}\n'
        sock.send(cmd.encode())
        time.sleep(0.1)

# Connect to QEMU monitor
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('/tmp/qemu_monitor.sock')

# Send help command
send_qemu_command(sock, 'h', 'e', 'l', 'p', 'Return')
time.sleep(0.5)

# Send clear command
send_qemu_command(sock, 'c', 'l', 'e', 'a', 'r', 'Return')
time.sleep(0.5)

# Send echo command
send_qemu_command(sock, 'e', 'c', 'h', 'o', 'space', 'H', 'e', 'l', 'l', 'o', 'Return')
time.sleep(0.5)

# Send fetch command
send_qemu_command(sock, 'f', 'e', 't', 'c', 'h', 'Return')
time.sleep(0.5)

sock.close()
```

Example Expect script:

```expect
#!/usr/bin/expect
set timeout 10

# Connect to QEMU monitor
spawn telnet localhost 55555

expect "(qemu)"

# Send help command
send "sendkey h e l p Return\r"
expect "(qemu)"

# Send clear command
send "sendkey c l e a r Return\r"
expect "(qemu)"

# Send echo command
send "sendkey e c h o space H e l l o Return\r"
expect "(qemu)"

# Send fetch command
send "sendkey f e t c h Return\r"
expect "(qemu)"

# Exit
send "quit\r"
interact
```

#### Method 5: Shell Script with QEMU Monitor

```bash
#!/bin/bash

# Connect to QEMU monitor and send commands
{
    sleep 1
    echo "sendkey h e l p Return"
    sleep 1
    echo "sendkey c l e a r Return"
    sleep 1
    echo "sendkey e c h o space T e s t Return"
    sleep 1
    echo "sendkey f e t c h Return"
    sleep 2
    echo "quit"
} | qemu-system-i386 -kernel dist/kernel.elf -monitor stdio
```

## Usage Examples

### Interactive Mode

When the OS boots, you'll see a welcome message and a prompt:

```
Welcome to AltoniumOS 1.0.0
Type 'help' for available commands

>
```

As you type characters, they appear before the prompt marker `>`. For example, typing "hello" shows:

```
hello>
```

The blinking cursor appears at the position between your input and the `>` marker, providing visual feedback of where the next character will appear.

### Commands

#### help
Display all available commands:
```
help>
Available commands:
  clear     - Clear the screen
  echo TEXT - Print text to the screen
  fetch     - Print OS and system information
  shutdown  - Shut down the system
  help      - Display this help message

>
```

#### echo
Print text to the console:
```
echo Hello, World!>
Hello, World!

>
```

#### fetch
Display system information:
```
fetch>
OS: AltoniumOS
Version: 1.0.0
Architecture: x86
Build Date: Dec  4 2025
Build Time: 05:32:00

>
```

#### clear
Clear the screen and reset prompt:
```
clear>
(screen clears)

>
```

#### dir / ls / pwd / cd
Navigate the FAT12 filesystem (both `ls` and `dir` share the same output which includes entry type and size information):
```
dir>
[DIR] DOCS
      README.TXT (92 bytes)
      SYSTEM.CFG (42 bytes)

cd docs>
pwd>
/DOCS
ls>
      INFO.TXT (32 bytes)
```

#### cat
Print file contents:
```
cat README.TXT>
Welcome to AltoniumOS FAT12 volume!
Use 'ls', 'cat', and 'write' inside the kernel shell.
```

#### touch
Create an empty file (useful for quickly seeding directories before writing data):
```
touch DOCS/EMPTY.TXT>
dir docs
[DIR] INFO.TXT (32 bytes)
      EMPTY.TXT (0 bytes)
```

#### write
Create or overwrite a text file (8.3 name, rest of the line becomes content):
```
write NOTES.TXT Remember to demo FAT12>
Wrote 22 bytes
cat NOTES.TXT>
Remember to demo FAT12
```

#### shutdown
Shut down the system (initiates ACPI power-off):
```
shutdown>
Attempting system shutdown...
Halting CPU...
```

## Architecture

### Project Structure

```
.
├── boot.asm           - x86 16-bit bootloader (now with a FAT12 BPB)
├── bootloader/
│   └── uefi_loader.c  - UEFI bootstrap that chain-loads GRUB and prints firmware logs
├── kernel_entry.asm   - x86 32-bit kernel entry point and low-level I/O
├── kernel.c           - Main kernel code with shell and command handlers
├── disk.c / disk.h    - ATA PIO driver for raw sector access
├── fat12.c / fat12.h  - FAT12 filesystem core (BPB parsing, directory/tree logic)
├── linker.ld          - Linker script for ELF binary layout
├── scripts/
│   └── build_fat12_image.py - Helper used by `make img` to lay out the FAT structures
├── Makefile           - Build system configuration
├── README.md          - This file
└── LICENSE            - MIT License
```

### Execution Flow

1. **Bootloader** (`boot.asm`) - 16-bit real mode code that:
   - Loads the kernel from disk (64 sectors = 32KB)
   - Enables the A20 line for extended memory access
   - Sets up a Global Descriptor Table (GDT)
   - Switches to 32-bit protected mode
   - Jumps to the kernel entry point at 0x10000

2. **Kernel Entry** (`kernel_entry.asm`) - 32-bit protected mode entry:
   - Includes Multiboot header for GRUB/QEMU compatibility
   - Sets up the stack at 0x800000 (8MB)
   - Calls the C kernel main function
   - Provides CPU halt function for shutdown

3. **Kernel Main** (`kernel.c`) - Implements command parsing and execution
4. **VGA Driver** - Direct VGA buffer manipulation for text output (0xB8000)
5. **Command Handlers** - Individual handlers for each console command

### UEFI Boot Pipeline

UEFI-capable systems boot from `EFI/BOOT/BOOTX64.EFI`, a small PE/COFF application built from `bootloader/uefi_loader.c`. The loader uses `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` to locate `EFI/ALTONIUM/GRUBX64.EFI`, prints status information through `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL`, and loads the standalone GRUB image via `BootServices->LoadImage`. GRUB remains responsible for loading the Multiboot kernel, but it now passes `bootmode=uefi` on the kernel command line so the C code can adjust its behavior/display. Legacy BIOS mode continues to use the existing boot sector and GRUB ISO flow.

### VGA Buffer Management

The OS communicates with the display via the VGA text buffer at memory address `0xB8000`. Each character occupies 2 bytes (character + color attribute). The buffer supports:

- 80 columns × 25 rows (80×25 text mode)
- Automatic scrolling when reaching the bottom
- Color attributes (default: white on black)

### Prompt and Cursor Management

The shell uses an intelligent prompt rendering system that maintains proper cursor alignment:

**Prompt Rendering (`render_prompt_line()`):**
- The prompt line displays input as: `[typed_characters]>`
- The `>` character acts as a right-hand caret marker, appearing after all typed input
- When typing "hello", the display shows: `hello>` with the cursor between the input and the marker

**Hardware Cursor Updates:**
- VGA hardware cursor position is updated via I/O ports 0x3D4/0x3D5
- The cursor position register (0x0E, 0x0F) is updated after every character, backspace, and newline
- This ensures the blinking cursor aligns with the actual position in the text buffer

**Keyboard Input Processing:**
- Printable characters are added to the input buffer and the line is redrawn
- Backspace removes the last character and redraws the line
- Enter executes the command and advances to a new line
- The entire line is redrawn to maintain proper cursor/prompt alignment

**Buffer Overflow Prevention:**
- Cursor position bounds are checked against VGA_WIDTH to prevent underflow/overflow
- Input buffer size is bounded to prevent buffer overruns
- Line clearing respects the prompt line starting position

### Shutdown Mechanism

The `shutdown` command attempts to power off the system through:

1. **ACPI Power-Off** (Primary) - Writes to port 0x604 (ACPI power-off port)
2. **CPU Halt** (Fallback) - Executes `CLI` (clear interrupts) + `HLT` (halt) for graceful shutdown on real hardware

## System Requirements

### Minimum Hardware
- x86 processor (32-bit or higher)
- 1 MB RAM
- VGA-compatible graphics card or serial terminal
- BIOS with multiboot support (for loading kernel)

### Testing Platform
This OS has been tested with:
- QEMU with x86 emulation (`qemu-system-i386`)
- Bochs x86 emulator
- Real x86 hardware (with bootloader)

## Batch Command Execution

### Via QEMU Monitor Scripting

Create a file `commands.txt`:
```
sendkey h e l p Return
sendkey e c h o space S t a r t i n g Return
sendkey f e t c h Return
sendkey c l e a r Return
```

Run with:
```bash
qemu-system-i386 -kernel dist/kernel.elf -monitor commands.txt
```

### Automated Testing Script

```bash
#!/bin/bash

QEMU_MONITOR="/tmp/qemu_monitor.sock"

run_os() {
    qemu-system-i386 \
        -kernel dist/kernel.elf \
        -chardev socket,id=mon,path=$QEMU_MONITOR,server,nowait \
        -mon chardev=mon,mode=control \
        &
    QEMU_PID=$!
    sleep 2
}

send_command() {
    local cmd="$1"
    echo "Sending command: $cmd"
    echo "sendkey $(echo $cmd | sed 's/./& /g') Return" | socat - UNIX-CONNECT:$QEMU_MONITOR
    sleep 1
}

run_os

send_command "help"
send_command "fetch"
send_command "echo Test Message"
send_command "clear"

kill $QEMU_PID
```

## Development

### Adding New Commands

To add a new command, follow these steps in `kernel.c`:

1. Create a handler function:
```c
void handle_newcmd(const char *args) {
    console_print("New command executed\n");
}
```

2. Add parsing in `execute_command()`:
```c
} else if (strncmp_impl(cmd_line, "newcmd", 6) == 0 && 
           (cmd_line[6] == '\0' || cmd_line[6] == ' ' || cmd_line[6] == '\n')) {
    const char *args = cmd_line + 6;
    handle_newcmd(args);
}
```

3. Update the `handle_help()` function to document the new command

### Debugging

Build with debugging symbols:
```bash
# Modify Makefile to add -g flag to CFLAGS
# Then build and debug with GDB

qemu-system-i386 -kernel dist/kernel.elf -s -S &
gdb -ex "target remote :1234" dist/kernel.elf
```

## Modular Architecture

AltoniumOS has been refactored into a modular architecture for better maintainability and readability. The codebase is organized into logical subsystems:

### Directory Structure

```
include/
  drivers/
    console.h      - VGA console and theme system interface
    keyboard.h     - Keyboard input and scancode handling interface
  shell/
    commands.h     - Command handler interface
    prompt.h       - Shell prompt and input buffer interface
    nano.h         - Nano text editor interface
  lib/
    string.h       - Common types, string utilities, and I/O helpers
  kernel/
    main.h         - Kernel initialization and boot mode detection

drivers/
  console/
    vga_console.c  - VGA text-mode rendering, cursor management, theme system
  input/
    keyboard.c     - PS/2 keyboard driver, scancode conversion, Ctrl/extended key tracking

shell/
  commands.c       - All command handlers (ls, cd, cat, echo, theme, etc.)
  prompt.c         - Shell prompt rendering and input handling
  nano.c           - Full-featured text editor with save/exit/navigation/help

lib/
  string.c         - String manipulation, number printing, tokenization

kernel/
  main.c           - Kernel entry point, initialization, main command loop

disk.c             - ATA PIO disk driver (legacy root location)
fat12.c            - FAT12 filesystem implementation (legacy root location)
```

### Module Responsibilities

**Console Driver** (`drivers/console/`):
- VGA buffer management and character rendering
- Cursor positioning via hardware I/O ports
- Theme system with three built-in color schemes
- Status bar rendering for nano editor

**Keyboard Driver** (`drivers/input/`):
- PS/2 keyboard port polling and scancode reading
- Extended scancode handling (0xE0 prefix for Delete, Home, End, etc.)
- Ctrl key state tracking
- ASCII conversion for printable characters
- Routing input to shell prompt or nano editor

**Shell Subsystem** (`shell/`):
- **prompt.c**: Input buffer management, prompt line rendering, scancode handling
- **commands.c**: Command parsing, dispatch, and all handler implementations
- **nano.c**: Full-screen text editor with viewport scrolling, file I/O, status bar

**Library** (`lib/`):
- Type definitions (uint8_t, uint32_t, size_t, etc.)
- String utilities (strcmp, strcpy, strlen, tokenization)
- Number printing and formatting
- Hardware I/O primitives (inb/outb inline functions)

**Kernel Core** (`kernel/`):
- Boot mode detection (BIOS vs UEFI via Multiboot command line)
- Disk and filesystem initialization
- Main command loop coordinating prompt and keyboard input

### Adding New Features

**To add a new console command:**

1. Add a handler function prototype to `include/shell/commands.h`
2. Implement the handler in `shell/commands.c`
3. Add parsing logic in `execute_command()` in `shell/commands.c`
4. Update `handle_help()` to document the command

**To add a new driver:**

1. Create a subdirectory under `drivers/` (e.g., `drivers/timer/`)
2. Add a header in `include/drivers/` (e.g., `include/drivers/timer.h`)
3. Implement the driver and update the Makefile to compile it
4. Initialize the driver in `kernel/main.c`

**To extend the nano editor:**

1. Modify `shell/nano.c` for new features
2. Add new keybindings in `nano_handle_scancode()`
3. Update the help overlay in `nano_render_help_overlay()`

### Build System

The Makefile compiles each module into a separate object file and links them together:

```makefile
KERNEL_OBJS = $(BUILD_DIR)/kernel_entry.o \
              $(BUILD_DIR)/main.o \
              $(BUILD_DIR)/string.o \
              $(BUILD_DIR)/vga_console.o \
              $(BUILD_DIR)/keyboard.o \
              $(BUILD_DIR)/prompt.o \
              $(BUILD_DIR)/commands.o \
              $(BUILD_DIR)/nano.o \
              $(BUILD_DIR)/disk.o \
              $(BUILD_DIR)/fat12.o
```

Each module is compiled with the same flags and linked into the final `kernel.elf`.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or issues.

## References

- [x86 Architecture Reference](https://en.wikipedia.org/wiki/X86)
- [OSDev Wiki](https://wiki.osdev.org/)
- [QEMU Documentation](https://www.qemu.org/documentation/)
- [NASM Documentation](https://www.nasm.us/)
