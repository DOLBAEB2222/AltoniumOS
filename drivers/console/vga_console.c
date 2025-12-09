#include "../../include/drivers/console.h"

#define VGA_BUFFER ((char *)0xB8000)

static theme_t themes[THEME_COUNT] = {
    {"normal", VGA_ATTR(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK), VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GRAY), VGA_ATTR(VGA_COLOR_WHITE, VGA_COLOR_BLACK)},
    {"blue", VGA_ATTR(VGA_COLOR_WHITE, VGA_COLOR_BLUE), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_CYAN), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_BLUE)},
    {"green", VGA_ATTR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_BLACK)}
};

static console_state_t global_console_state = {0, 0, THEME_NORMAL};
static console_buffer_t console_buffer = {0};
static int console_enabled = 1;

void console_init(console_state_t *state) {
    if (state) {
        state->cursor_x = 0;
        state->cursor_y = 0;
        state->current_theme = THEME_NORMAL;
    }
}

console_state_t *console_get_state(void) {
    return &global_console_state;
}

uint8_t get_current_text_attr(void) {
    return themes[global_console_state.current_theme].text_attr;
}

uint8_t get_current_status_attr(void) {
    return themes[global_console_state.current_theme].status_attr;
}

void update_hardware_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
}

void vga_write_char(char c, uint8_t attr) {
    // Always buffer output, regardless of video state
    console_buffer_putchar(c);
    
    // Only write to VGA if console is enabled
    if (!console_enabled) {
        return;
    }
    
    if (c == '\n') {
        global_console_state.cursor_x = 0;
        global_console_state.cursor_y++;
        if (global_console_state.cursor_y >= VGA_HEIGHT) {
            vga_clear();
            global_console_state.cursor_y = 0;
        }
    } else if (c == '\r') {
        global_console_state.cursor_x = 0;
    } else if (c == '\t') {
        global_console_state.cursor_x += 4;
    } else {
        size_t pos = global_console_state.cursor_y * VGA_WIDTH + global_console_state.cursor_x;
        VGA_BUFFER[pos * 2] = c;
        VGA_BUFFER[pos * 2 + 1] = attr;
        global_console_state.cursor_x++;
    }

    if (global_console_state.cursor_x >= VGA_WIDTH) {
        global_console_state.cursor_x = 0;
        global_console_state.cursor_y++;
        if (global_console_state.cursor_y >= VGA_HEIGHT) {
            vga_clear();
            global_console_state.cursor_y = 0;
        }
    }
    
    update_hardware_cursor(global_console_state.cursor_x, global_console_state.cursor_y);
}

void vga_clear(void) {
    uint8_t text_attr = get_current_text_attr();
    
    // Only clear VGA memory if console is enabled
    if (console_enabled) {
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            VGA_BUFFER[i * 2] = ' ';
            VGA_BUFFER[i * 2 + 1] = text_attr;
        }
    }
    
    global_console_state.cursor_x = 0;
    global_console_state.cursor_y = 0;
    
    // Only update hardware cursor if console is enabled
    if (console_enabled) {
        update_hardware_cursor(global_console_state.cursor_x, global_console_state.cursor_y);
    }
}

void console_print(const char *str) {
    if (!str) return;
    uint8_t text_attr = get_current_text_attr();
    for (int i = 0; str[i] != '\0'; i++) {
        vga_write_char(str[i], text_attr);
    }
}

void console_putchar(char c) {
    uint8_t text_attr = get_current_text_attr();
    vga_write_char(c, text_attr);
}

void console_print_to_pos(int y, int x, const char *str) {
    if (!str) return;
    uint8_t status_attr = get_current_status_attr();
    for (int i = 0; str[i] != '\0' && x + i < VGA_WIDTH; i++) {
        size_t pos = y * VGA_WIDTH + (x + i);
        VGA_BUFFER[pos * 2] = str[i];
        VGA_BUFFER[pos * 2 + 1] = status_attr;
    }
}

int console_get_cursor_x(void) {
    return global_console_state.cursor_x;
}

int console_get_cursor_y(void) {
    return global_console_state.cursor_y;
}

void console_set_cursor(int x, int y) {
    global_console_state.cursor_x = x;
    global_console_state.cursor_y = y;
    update_hardware_cursor(x, y);
}

int console_get_theme(void) {
    return global_console_state.current_theme;
}

void console_set_theme(int theme) {
    if (theme >= 0 && theme < THEME_COUNT) {
        global_console_state.current_theme = theme;
    }
}

const theme_t *console_get_themes(void) {
    return themes;
}

void console_set_enabled(int enabled) {
    console_enabled = enabled;
    console_buffer.enabled = enabled;
}

int console_is_enabled(void) {
    return console_enabled;
}

void console_buffer_init(void) {
    console_buffer.head = 0;
    console_buffer.tail = 0;
    console_buffer.count = 0;
    console_buffer.enabled = 1;
}

void console_buffer_putchar(char c) {
    if (console_buffer.count < CONSOLE_BUFFER_SIZE) {
        console_buffer.buffer[console_buffer.tail] = c;
        console_buffer.tail = (console_buffer.tail + 1) % CONSOLE_BUFFER_SIZE;
        console_buffer.count++;
    }
}

void console_buffer_puts(const char *str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        console_buffer_putchar(str[i]);
    }
}

int console_buffer_get(char *buf, int size) {
    if (!buf || size <= 0) return 0;
    
    int copied = 0;
    while (copied < size && console_buffer.count > 0) {
        buf[copied] = console_buffer.buffer[console_buffer.head];
        console_buffer.head = (console_buffer.head + 1) % CONSOLE_BUFFER_SIZE;
        console_buffer.count--;
        copied++;
    }
    return copied;
}
