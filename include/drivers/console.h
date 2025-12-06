#ifndef DRIVERS_CONSOLE_H
#define DRIVERS_CONSOLE_H

#include "../lib/string.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define VGA_COLOR_CYAN 0x3
#define VGA_COLOR_RED 0x4
#define VGA_COLOR_MAGENTA 0x5
#define VGA_COLOR_BROWN 0x6
#define VGA_COLOR_LIGHT_GRAY 0x7
#define VGA_COLOR_DARK_GRAY 0x8
#define VGA_COLOR_LIGHT_BLUE 0x9
#define VGA_COLOR_LIGHT_GREEN 0xA
#define VGA_COLOR_LIGHT_CYAN 0xB
#define VGA_COLOR_LIGHT_RED 0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW 0xE
#define VGA_COLOR_WHITE 0xF

#define VGA_ATTR(fg, bg) ((bg << 4) | fg)

#define THEME_NORMAL 0
#define THEME_BLUE 1
#define THEME_GREEN 2
#define THEME_COUNT 3

typedef struct {
    const char *name;
    uint8_t text_attr;
    uint8_t status_attr;
    uint8_t cursor_attr;
} theme_t;

typedef struct {
    int cursor_x;
    int cursor_y;
    int current_theme;
} console_state_t;

void console_init(console_state_t *state);
console_state_t *console_get_state(void);
void vga_clear(void);
void console_print(const char *str);
void console_putchar(char c);
void console_print_to_pos(int y, int x, const char *str);
void update_hardware_cursor(int x, int y);
uint8_t get_current_text_attr(void);
uint8_t get_current_status_attr(void);
int console_get_cursor_x(void);
int console_get_cursor_y(void);
void console_set_cursor(int x, int y);
int console_get_theme(void);
void console_set_theme(int theme);
const theme_t *console_get_themes(void);

#endif
