#include "console.h"

// Simple va_args implementation for kernel use
typedef void* va_list;
#define va_start(ap, last) ((ap) = (va_list)(&last + 1))
#define va_arg(ap, type) (*(type*)(ap)++)
#define va_end(ap) ((ap) = (va_list)0)

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000
#define VGA_CONTROL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

// VGA cursor I/O ports
#define VGA_CURSOR_HIGH_CMD 14
#define VGA_CURSOR_LOW_CMD 15

static uint16_t* vga_buffer;
static size_t console_row;
static size_t console_col;
static uint8_t console_color;
static bool cursor_enabled = true;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static void console_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', console_color);
    }
    
    // Move cursor to the last line
    console_row = VGA_HEIGHT - 1;
}

void console_init(void) {
    vga_buffer = (uint16_t*) VGA_MEMORY;
    console_row = 0;
    console_col = 0;
    console_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', console_color);
        }
    }
    
    // Enable and set cursor to top-left
    console_enable_cursor(true);
    console_set_cursor(0, 0);
    
    // Display welcome message
    console_write("Welcome to AltoniumOS\n");
}

void console_putchar(char c) {
    if (c == '\n') {
        console_col = 0;
        if (++console_row == VGA_HEIGHT) {
            console_scroll();
        }
    } else if (c == '\r') {
        console_col = 0;
    } else if (c == '\t') {
        // Tab moves to next 8-column boundary
        console_col = (console_col + 8) & ~7;
        if (console_col >= VGA_WIDTH) {
            console_col = 0;
            if (++console_row == VGA_HEIGHT) {
                console_scroll();
            }
        }
    } else if (c == '\b') {
        if (console_col > 0) {
            console_col--;
            const size_t index = console_row * VGA_WIDTH + console_col;
            vga_buffer[index] = vga_entry(' ', console_color);
        } else if (console_row > 0) {
            // Move to end of previous line
            console_row--;
            console_col = VGA_WIDTH - 1;
            const size_t index = console_row * VGA_WIDTH + console_col;
            vga_buffer[index] = vga_entry(' ', console_color);
        }
    } else if (c >= ' ') { // Printable characters
        const size_t index = console_row * VGA_WIDTH + console_col;
        vga_buffer[index] = vga_entry(c, console_color);
        
        if (++console_col == VGA_WIDTH) {
            console_col = 0;
            if (++console_row == VGA_HEIGHT) {
                console_scroll();
            }
        }
    }
    
    // Update hardware cursor
    if (cursor_enabled) {
        console_update_cursor_hw();
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
    
    if (cursor_enabled) {
        console_update_cursor_hw();
    }
}

void console_set_cursor(size_t row, size_t col) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        console_row = row;
        console_col = col;
        
        if (cursor_enabled) {
            console_update_cursor_hw();
        }
    }
}

void console_get_cursor(size_t* row, size_t* col) {
    if (row) *row = console_row;
    if (col) *col = console_col;
}

void console_enable_cursor(bool enabled) {
    cursor_enabled = enabled;
    
    if (enabled) {
        // Enable cursor (scanline start at 0, end at 15)
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xA), "Nd"(VGA_CONTROL_PORT));
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x0), "Nd"(VGA_DATA_PORT));
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xB), "Nd"(VGA_CONTROL_PORT));
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xF), "Nd"(VGA_DATA_PORT));
        
        console_update_cursor_hw();
    } else {
        // Disable cursor
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xA), "Nd"(VGA_CONTROL_PORT));
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"(VGA_DATA_PORT));
    }
}

void console_update_cursor_hw(void) {
    uint16_t position = console_row * VGA_WIDTH + console_col;
    
    // Send high byte
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)VGA_CURSOR_HIGH_CMD), "Nd"(VGA_CONTROL_PORT));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)(position >> 8)), "Nd"(VGA_DATA_PORT));
    
    // Send low byte
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)VGA_CURSOR_LOW_CMD), "Nd"(VGA_CONTROL_PORT));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)(position & 0xFF)), "Nd"(VGA_DATA_PORT));
}

void console_set_color(uint8_t fg, uint8_t bg) {
    console_color = vga_entry_color(fg, bg);
}

void console_reset_color(void) {
    console_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// Simple printf implementation with va_args
void console_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    const char* p = format;
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'd': {
                    int value = va_arg(args, int);
                    console_putint(value);
                    break;
                }
                case 'x': {
                    uint32_t value = va_arg(args, uint32_t);
                    console_puthex(value);
                    break;
                }
                case 'u': {
                    uint32_t value = va_arg(args, uint32_t);
                    console_putuint(value);
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str) {
                        console_write(str);
                    } else {
                        console_write("(null)");
                    }
                    break;
                }
                case 'c': {
                    char c = va_arg(args, int);
                    console_putchar(c);
                    break;
                }
                case '%': {
                    console_putchar('%');
                    break;
                }
                default: {
                    console_putchar('%');
                    console_putchar(*p);
                    break;
                }
            }
        } else {
            console_putchar(*p);
        }
        p++;
    }
    
    va_end(args);
}

void console_putint(int value) {
    if (value == 0) {
        console_putchar('0');
        return;
    }
    
    if (value < 0) {
        console_putchar('-');
        value = -value;
    }
    
    // Buffer to store digits (reversed)
    char buffer[32];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Print digits in correct order
    for (int j = i - 1; j >= 0; j--) {
        console_putchar(buffer[j]);
    }
}

void console_puthex(uint32_t value) {
    console_write("0x");
    
    if (value == 0) {
        console_putchar('0');
        return;
    }
    
    // Buffer to store hex digits (reversed)
    char buffer[32];
    int i = 0;
    
    while (value > 0) {
        uint8_t digit = value & 0xF;
        if (digit < 10) {
            buffer[i++] = '0' + digit;
        } else {
            buffer[i++] = 'A' + (digit - 10);
        }
        value >>= 4;
    }
    
    // Print digits in correct order
    for (int j = i - 1; j >= 0; j--) {
        console_putchar(buffer[j]);
    }
}

void console_putuint(uint32_t value) {
    if (value == 0) {
        console_putchar('0');
        return;
    }
    
    // Buffer to store digits (reversed)
    char buffer[32];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Print digits in correct order
    for (int j = i - 1; j >= 0; j--) {
        console_putchar(buffer[j]);
    }
}