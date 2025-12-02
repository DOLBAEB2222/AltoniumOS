# AltoniumOS - Console Module

This directory contains the VGA text-mode console implementation for AltoniumOS.

## Overview

The console module (`kernel/console.c` + `kernel/console.h`) provides a complete VGA text-mode console driver with initialization, cursor control, clearing, scrolling, and basic formatted output helpers. On kernel start, `console_init()` is called and displays the required "Welcome to AltoniumOS" message before the kernel continues execution.

## Features

### Core Functions
- `console_init()` - Initialize the VGA text-mode console and display welcome message
- `console_putchar(char c)` - Write a single character to the console
- `console_write(const char* str)` - Write a null-terminated string to the console
- `console_clear()` - Clear the entire screen

### Cursor Control
- `console_set_cursor(size_t row, size_t col)` - Set cursor position
- `console_get_cursor(size_t* row, size_t* col)` - Get current cursor position
- `console_enable_cursor(bool enabled)` - Enable/disable hardware cursor
- `console_update_cursor_hw()` - Update hardware cursor position via VGA I/O ports

### Color Support
- `console_set_color(uint8_t fg, uint8_t bg)` - Set text and background colors
- `console_reset_color()` - Reset to default colors (light grey on black)
- 16 VGA color constants defined in `vga_color_t` enum:
  - Black, Blue, Green, Cyan, Red, Magenta, Brown, Light Grey
  - Dark Grey, Light Blue, Light Green, Light Cyan, Light Red, Light Magenta, Light Brown, White

### Formatted Output Helpers
- `console_printf(const char* format, ...)` - Formatted output with %d, %u, %x, %s, %c, %%
- `console_putint(int value)` - Print signed integer
- `console_puthex(uint32_t value)` - Print hexadecimal value with "0x" prefix
- `console_putuint(uint32_t value)` - Print unsigned integer

### Special Character Handling
- Newline (`\n`) - Move to beginning of next line
- Carriage return (`\r`) - Move to beginning of current line  
- Tab (`\t`) - Move to next 8-column boundary
- Backspace (`\b`) - Move back and erase character (handles line boundaries)
- Automatic scrolling when screen is full

### Hardware Interface
- Direct VGA memory access at `0xB8000`
- Hardware cursor control via VGA I/O ports `0x3D4`/`0x3D5`
- 80×25 character text mode
- No external library dependencies (kernel-friendly)

## Usage

```c
#include "kernel/console.h"

void example_usage(void) {
    // Initialize console (displays "Welcome to AltoniumOS")
    console_init();
    
    // Basic output
    console_write("Hello, World!\n");
    
    // Cursor control
    console_set_cursor(10, 20);
    console_putchar('X');
    
    // Color output
    console_set_color(VGA_COLOR_RED, VGA_COLOR_WHITE);
    console_write("Red text on white background\n");
    console_reset_color();
    
    // Formatted output
    console_printf("Integer: %d, Hex: 0x%x, String: %s\n", -42, 0xDEADBEEF, "test");
}
```

## Architecture

The console module uses standard VGA text-mode with:
- **Memory**: Direct access to VGA framebuffer at `0xB8000`
- **Resolution**: 80 columns × 25 rows
- **Colors**: 16 foreground colors, 8 background colors
- **Cursor**: Hardware cursor with enable/disable control
- **Performance**: Direct memory manipulation for optimal speed

## Building

```bash
make clean && make
```

This builds `kernel.bin` which contains the console module and demonstration code.

## API for Other Subsystems

The console module exposes clean APIs that other kernel subsystems (shell, commands, etc.) can use:

- All screen output should use `console_write()` or `console_printf()`
- Character input handling should use `console_putchar()` for echoing
- Screen clearing should use `console_clear()`
- Color changes should use `console_set_color()`/`console_reset_color()`

The console is thread-safe for single-threaded kernel use and handles all VGA hardware interaction internally.