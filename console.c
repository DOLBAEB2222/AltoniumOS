#include "console.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static uint16_t* vga_buffer;
static size_t console_row;
static size_t console_col;
static uint8_t console_color;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void console_init(void) {
    vga_buffer = (uint16_t*) VGA_MEMORY;
    console_row = 0;
    console_col = 0;
    console_color = vga_entry_color(15, 0);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', console_color);
        }
    }
}

static void console_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', console_color);
    }
    
    console_row = VGA_HEIGHT - 1;
}

void console_putchar(char c) {
    if (c == '\n') {
        console_col = 0;
        if (++console_row == VGA_HEIGHT) {
            console_scroll();
        }
        return;
    }
    
    if (c == '\b') {
        if (console_col > 0) {
            console_col--;
            const size_t index = console_row * VGA_WIDTH + console_col;
            vga_buffer[index] = vga_entry(' ', console_color);
        }
        return;
    }
    
    const size_t index = console_row * VGA_WIDTH + console_col;
    vga_buffer[index] = vga_entry(c, console_color);
    
    if (++console_col == VGA_WIDTH) {
        console_col = 0;
        if (++console_row == VGA_HEIGHT) {
            console_scroll();
        }
    }
}

void console_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        console_putchar(str[i]);
    }
}

void console_clear(void) {
    console_row = 0;
    console_col = 0;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', console_color);
        }
    }
}
