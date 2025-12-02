# MiniOS - A Simple Console-Based Operating System

A minimal x86 operating system with console commands for system interaction and information retrieval.

## Features

This OS includes the following console commands:

- **clear** - Clear the VGA text buffer and reset the screen
- **echo** - Write text to the console
- **fetch** - Display OS and system information (name, version, architecture, build date/time)
- **shutdown** - Gracefully shut down the system (attempts ACPI power-off via port 0x604)
- **help** - Display all available commands

## Building

### Prerequisites

- `nasm` (Netwide Assembler) for assembling x86 code
- `gcc` with i386 support (32-bit) for compiling C code
- `binutils` (ld, objcopy) for linking and binary conversion
- `make` for building
- `qemu-system-i386` for testing (optional)

### Build Instructions

```bash
# Build the kernel ELF binary
make build

# Create a bootable disk image (requires parted/fdisk)
make bootable

# Clean build artifacts
make clean
```

## Running the OS

### With QEMU (Recommended)

```bash
# Run the kernel directly with multiboot support
qemu-system-i386 -kernel dist/kernel.elf

# Run from a bootable disk image
qemu-system-i386 -drive file=dist/os.img,format=raw

# For debugging with GDB
qemu-system-i386 -kernel dist/kernel.elf -s -S &
gdb dist/kernel.elf -ex "target remote :1234"
```

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

When the OS boots, you'll see a prompt:

```
Welcome to MiniOS 1.0.0
Type 'help' for available commands

>
```

### Commands

#### help
Display all available commands:
```
> help
Available commands:
  clear     - Clear the screen
  echo TEXT - Print text to the screen
  fetch     - Print OS and system information
  shutdown  - Shut down the system
  help      - Display this help message
```

#### echo
Print text to the console:
```
> echo Hello, World!
Hello, World!
```

#### fetch
Display system information:
```
> fetch
OS: MiniOS
Version: 1.0.0
Architecture: x86
Build Date: Dec  2 2024
Build Time: 14:08:42
```

#### clear
Clear the screen:
```
> clear
```

#### shutdown
Shut down the system (initiates ACPI power-off):
```
> shutdown
Attempting system shutdown...
Halting CPU...
```

## Architecture

### Project Structure

```
.
├── boot.asm           - x86 16-bit bootloader
├── kernel_entry.asm   - x86 32-bit kernel entry point and low-level I/O
├── kernel.c           - Main kernel code with command handlers
├── linker.ld          - Linker script for ELF binary layout
├── Makefile           - Build system configuration
├── README.md          - This file
└── LICENSE            - MIT License
```

### Execution Flow

1. **Bootloader** (`boot.asm`) - 16-bit real mode code that loads the kernel from disk
2. **Kernel Entry** (`kernel_entry.asm`) - Initializes stack and interrupts, transitions to 32-bit protected mode
3. **Kernel Main** (`kernel.c`) - Implements command parsing and execution
4. **VGA Driver** - Direct VGA buffer manipulation for text output (0xB8000)
5. **Command Handlers** - Individual handlers for each console command

### VGA Buffer Management

The OS communicates with the display via the VGA text buffer at memory address `0xB8000`. Each character occupies 2 bytes (character + color attribute). The buffer supports:

- 80 columns × 25 rows (80×25 text mode)
- Automatic scrolling when reaching the bottom
- Color attributes (default: white on black)

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

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or issues.

## References

- [x86 Architecture Reference](https://en.wikipedia.org/wiki/X86)
- [OSDev Wiki](https://wiki.osdev.org/)
- [QEMU Documentation](https://www.qemu.org/documentation/)
- [NASM Documentation](https://www.nasm.us/)
