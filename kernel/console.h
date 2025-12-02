#ifndef CONSOLE_H
#define CONSOLE_H

#include "../types.h"

// Console colors
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

// Core console functions
void console_init(void);
void console_putchar(char c);
void console_write(const char* str);
void console_clear(void);

// Cursor control functions
void console_set_cursor(size_t row, size_t col);
void console_get_cursor(size_t* row, size_t* col);
void console_enable_cursor(bool enabled);
void console_update_cursor_hw(void);

// Color functions
void console_set_color(uint8_t fg, uint8_t bg);
void console_reset_color(void);

// Formatted output helpers
void console_printf(const char* format, ...);
void console_putint(int value);
void console_puthex(uint32_t value);
void console_putuint(uint32_t value);

#endif