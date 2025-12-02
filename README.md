# Simple OS Kernel with PS/2 Keyboard and Shell

A bare-metal x86 kernel featuring a PS/2 keyboard driver with IRQ1 handling and a simple batch-mode shell.

## Features

- **PS/2 Keyboard Driver**: Full scancode to ASCII translation with shift key support
- **Interrupt Handling**: Proper IDT (Interrupt Descriptor Table) setup
- **PIC Configuration**: Programmable Interrupt Controller initialization for IRQ handling
- **Character Buffering**: Ring buffer for keyboard input
- **VGA Text Mode Console**: 80x25 text display with scrolling support
- **Batch-Mode Shell**: Command-line interface with tokenization and command dispatch

## Building

Requirements:
- `nasm` (Netwide Assembler)
- `gcc` with 32-bit support
- `ld` (GNU linker)

To build the kernel:

```bash
make
```

This will produce `kernel.bin`, a multiboot-compatible kernel binary.

## Running

The kernel can be run using QEMU or any multiboot-compatible bootloader:

```bash
qemu-system-i386 -kernel kernel.bin
```

## Shell Commands

The shell supports the following commands:

- `help` - Display available commands
- `clear` - Clear the screen
- `echo [args...]` - Echo arguments back to the console
- `hello` - Print a greeting message

## Architecture

### Components

1. **boot.asm**: Multiboot header and kernel entry point
2. **kernel.c**: Main kernel initialization and entry
3. **console.c/h**: VGA text mode console driver
4. **idt.c/h**: Interrupt Descriptor Table management
5. **pic.c/h**: Programmable Interrupt Controller driver
6. **keyboard.c/h**: PS/2 keyboard driver with IRQ1 handler
7. **shell.c/h**: Simple batch-mode shell with command parsing

### Interrupt Handling

- IRQ1 (keyboard) is mapped to interrupt 33 (0x21)
- The keyboard driver installs a handler that reads scancodes and converts them to ASCII
- Characters are buffered in a ring buffer for batch processing
- The shell reads complete lines (newline-terminated) before processing commands

## License

MIT License - See LICENSE file for details
