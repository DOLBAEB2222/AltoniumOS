/* Minimal type definitions - no libc */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long size_t;
typedef int ssize_t;

#define VGA_BUFFER ((char *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ATTR_DEFAULT 0x07  /* White on black */

/* VGA color codes */
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

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_t;

/* VGA buffer management */
static int cursor_x = 0;
static int cursor_y = 0;

/* Command input buffer */
static char input_buffer[256];
static int input_pos = 0;
static int command_executed = 0;

/* Prompt line tracking */
static int prompt_line_start_x = 0;
static int prompt_line_y = 0;

/* FAT12 filesystem state */
static int fat_ready = 0;
#define FS_IO_BUFFER_SIZE 16384
static uint8_t fs_io_buffer[FS_IO_BUFFER_SIZE];

/* Keyboard state */
static int ctrl_pressed = 0;

/* Theme system */
typedef struct {
    const char *name;
    uint8_t text_attr;
    uint8_t status_attr;
    uint8_t cursor_attr;
} theme_t;

#define THEME_NORMAL 0
#define THEME_BLUE 1
#define THEME_GREEN 2
#define THEME_COUNT 3

static theme_t themes[THEME_COUNT] = {
    {"normal", VGA_ATTR(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK), VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GRAY), VGA_ATTR(VGA_COLOR_WHITE, VGA_COLOR_BLACK)},
    {"blue", VGA_ATTR(VGA_COLOR_WHITE, VGA_COLOR_BLUE), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_CYAN), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_BLUE)},
    {"green", VGA_ATTR(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN), VGA_ATTR(VGA_COLOR_YELLOW, VGA_COLOR_BLACK)}
};

static int current_theme = THEME_NORMAL;

/* External: defined in assembly */
extern void halt_cpu(void);

/* Include disk driver */
#include "disk.h"
#include "fat12.h"

/* Nano editor state */
#define NANO_MAX_LINES 1000
#define NANO_MAX_LINE_LENGTH 200
#define NANO_VIEWPORT_HEIGHT 23  /* Leave 2 lines for status bar */
static int nano_editor_active = 0;
static char nano_filename[FAT12_PATH_MAX];
static char nano_lines[NANO_MAX_LINES][NANO_MAX_LINE_LENGTH];
static int nano_line_lengths[NANO_MAX_LINES];
static int nano_total_lines = 0;
static int nano_cursor_x = 0;
static int nano_cursor_y = 0;
static int nano_viewport_y = 0;
static int nano_dirty = 0;

/* Forward declarations */
void kernel_main(void);
void vga_clear(void);
void console_print(const char *str);
void console_putchar(char c);
void command_handler(void);
void handle_clear(void);
void handle_echo(const char *args);
void handle_fetch(void);
void handle_help(void);
void handle_shutdown(void);
void handle_disk(void);
void handle_ls(const char *args);
void handle_pwd(void);
void handle_cd(const char *args);
void handle_cat(const char *args);
void handle_touch(const char *args);
void handle_write_command(const char *args);
void handle_mkdir_command(const char *args);
void handle_rm_command(const char *args);
void handle_nano_command(const char *args);
void handle_theme_command(const char *args);
void execute_command(const char *cmd_line);
void handle_keyboard_input(void);
void update_hardware_cursor(int x, int y);
void render_prompt_line(void);
uint8_t get_current_text_attr(void);
uint8_t get_current_status_attr(void);

/* Nano editor functions */
void nano_init_editor(const char *filename);
void nano_render_editor(void);
void nano_handle_keyboard(void);
void nano_save_file(void);
void nano_exit_editor(void);
void nano_insert_char(char c);
void nano_handle_backspace(void);
void nano_handle_enter(void);
void nano_move_cursor(int dx, int dy);
void nano_scroll_to_cursor(void);
void console_print_to_pos(int y, int x, const char *str);

/* Keyboard input functions */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a" (data), "Nd" (port));
}

void update_hardware_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    /* Write cursor position high byte */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
    
    /* Write cursor position low byte */
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
}

int keyboard_ready(void) {
    return (inb(0x64) & 1) != 0;
}

uint8_t read_keyboard(void) {
    while (!keyboard_ready()) {
        /* Wait for keyboard input */
    }
    return inb(0x60);
}

uint8_t get_current_text_attr(void) {
    return themes[current_theme].text_attr;
}

uint8_t get_current_status_attr(void) {
    return themes[current_theme].status_attr;
}

void handle_keyboard_input(void) {
    uint8_t scancode = read_keyboard();
    
    /* Track Ctrl key state */
    if (scancode == 0x1D) {
        ctrl_pressed = 1;
        return;
    } else if (scancode == 0x9D) {
        ctrl_pressed = 0;
        return;
    }
    
    /* Handle differently if in nano editor mode */
    if (nano_editor_active) {
        nano_handle_keyboard();
        return;
    }
    
    /* Convert scancode to ASCII (US layout) */
    char c = 0;
    
    /* Only handle key press (not release) - high bit not set */
    if (!(scancode & 0x80)) {
        /* US keyboard layout mapping (unshifted) */
        switch (scancode) {
            /* Numbers 1-0 */
            case 0x02: c = '1'; break;
            case 0x03: c = '2'; break;
            case 0x04: c = '3'; break;
            case 0x05: c = '4'; break;
            case 0x06: c = '5'; break;
            case 0x07: c = '6'; break;
            case 0x08: c = '7'; break;
            case 0x09: c = '8'; break;
            case 0x0A: c = '9'; break;
            case 0x0B: c = '0'; break;
            
            /* Special characters on number row */
            case 0x0C: c = '-'; break;  /* - = */
            case 0x0D: c = '='; break;  /* = + */
            
            /* Top letter row (Q-P) */
            case 0x10: c = 'q'; break;
            case 0x11: c = 'w'; break;
            case 0x12: c = 'e'; break;
            case 0x13: c = 'r'; break;
            case 0x14: c = 't'; break;
            case 0x15: c = 'y'; break;
            case 0x16: c = 'u'; break;
            case 0x17: c = 'i'; break;
            case 0x18: c = 'o'; break;
            case 0x19: c = 'p'; break;
            
            /* Special characters on top row */
            case 0x1A: c = '['; break;  /* [ { */
            case 0x1B: c = ']'; break;  /* ] } */
            
            /* Middle letter row (A-L) */
            case 0x1E: c = 'a'; break;
            case 0x1F: c = 's'; break;
            case 0x20: c = 'd'; break;
            case 0x21: c = 'f'; break;
            case 0x22: c = 'g'; break;
            case 0x23: c = 'h'; break;
            case 0x24: c = 'j'; break;
            case 0x25: c = 'k'; break;
            case 0x26: c = 'l'; break;
            
            /* Special characters on middle row */
            case 0x27: c = ';'; break;  /* ; : */
            case 0x28: c = '\''; break; /* ' " */
            
            /* Backtick and backslash */
            case 0x29: c = '`'; break;  /* ` ~ */
            case 0x2B: c = '\\'; break; /* \ | */
            
            /* Bottom letter row (Z-M) */
            case 0x2C: c = 'z'; break;
            case 0x2D: c = 'x'; break;
            case 0x2E: c = 'c'; break;
            case 0x2F: c = 'v'; break;
            case 0x30: c = 'b'; break;
            case 0x31: c = 'n'; break;
            case 0x32: c = 'm'; break;
            
            /* Special characters on bottom row */
            case 0x33: c = ','; break;  /* , < */
            case 0x34: c = '.'; break;  /* . > */
            case 0x35: c = '/'; break;  /* / ? */
            
            /* Space bar */
            case 0x39: c = ' '; break;
            
            /* Control keys */
            case 0x0E: c = '\b'; break;  /* Backspace */
            case 0x1C: c = '\n'; break;  /* Enter */
            case 0x0F: c = '\t'; break;  /* Tab */
        }
        
        /* Process the character */
        if (c == '\n') {
            /* Enter key - execute command */
            console_putchar('\n');
            input_buffer[input_pos] = '\0';
            execute_command(input_buffer);
            input_pos = 0;
            command_executed = 1;  /* Mark command as executed */
        } else if (c == '\b') {
            /* Backspace key - remove from buffer and redraw prompt line */
            if (input_pos > 0) {
                input_pos--;
                render_prompt_line();
            }
        } else if (c >= ' ' && c <= '~') {
            /* Printable character - add to buffer and redraw prompt line */
            if (input_pos < (int)(sizeof(input_buffer) - 1)) {
                input_buffer[input_pos++] = c;
                render_prompt_line();
            }
        }
    }
}

/* Get build date/time - these are string literals for now */
static const char *os_name = "AltoniumOS";
static const char *os_version = "1.0.0";
static const char *os_arch = "x86";
static const char *build_date = __DATE__;
static const char *build_time = __TIME__;

/* Simple CPUID wrapper */
void get_cpuid(uint32_t eax, cpuid_t *regs) {
    /* This would be implemented in assembly */
    regs->eax = eax;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
}

void render_prompt_line(void) {
    int line_y = prompt_line_y;
    uint8_t text_attr = get_current_text_attr();
    
    /* Clear from prompt start to end of line */
    for (int i = prompt_line_start_x; i < VGA_WIDTH; i++) {
        size_t pos = line_y * VGA_WIDTH + i;
        VGA_BUFFER[pos * 2] = ' ';
        VGA_BUFFER[pos * 2 + 1] = text_attr;
    }
    
    /* Write input buffer content */
    for (int i = 0; i < input_pos; i++) {
        size_t pos = line_y * VGA_WIDTH + (prompt_line_start_x + i);
        if (prompt_line_start_x + i < VGA_WIDTH) {
            VGA_BUFFER[pos * 2] = input_buffer[i];
            VGA_BUFFER[pos * 2 + 1] = text_attr;
        }
    }
    
    /* Write prompt marker (>) at the end of input */
    if (prompt_line_start_x + input_pos < VGA_WIDTH) {
        size_t pos = line_y * VGA_WIDTH + (prompt_line_start_x + input_pos);
        VGA_BUFFER[pos * 2] = '>';
        VGA_BUFFER[pos * 2 + 1] = text_attr;
    }
    
    /* Update hardware cursor position (after the input, before the >) */
    cursor_x = prompt_line_start_x + input_pos;
    cursor_y = line_y;
    update_hardware_cursor(cursor_x, cursor_y);
}

/* Write a character to VGA buffer */
void vga_write_char(char c, uint8_t attr) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            vga_clear();
            cursor_y = 0;
        }
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x += 4;
    } else {
        size_t pos = cursor_y * VGA_WIDTH + cursor_x;
        VGA_BUFFER[pos * 2] = c;
        VGA_BUFFER[pos * 2 + 1] = attr;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) {
            vga_clear();
            cursor_y = 0;
        }
    }
    
    /* Update hardware cursor */
    update_hardware_cursor(cursor_x, cursor_y);
}

/* Clear VGA buffer */
void vga_clear(void) {
    uint8_t text_attr = get_current_text_attr();
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = text_attr;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor(cursor_x, cursor_y);
}

/* Print a string */
void console_print(const char *str) {
    if (!str) return;
    uint8_t text_attr = get_current_text_attr();
    for (int i = 0; str[i] != '\0'; i++) {
        vga_write_char(str[i], text_attr);
    }
}

/* Print a single character */
void console_putchar(char c) {
    uint8_t text_attr = get_current_text_attr();
    vga_write_char(c, text_attr);
}

/* Simple string utilities */
int strcmp_impl(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

int strncmp_impl(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!a[i] || !b[i] || a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}

void strcpy_impl(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

size_t strlen_impl(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

const char *skip_whitespace(const char *str) {
    if (!str) {
        return str;
    }
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

int read_token(const char **input, char *dest, int max_len) {
    if (!input || !dest || max_len <= 0) {
        return 0;
    }
    const char *ptr = skip_whitespace(*input);
    if (!ptr || *ptr == '\0') {
        return 0;
    }
    int len = 0;
    while (*ptr && *ptr != ' ' && *ptr != '\t') {
        if (len < max_len - 1) {
            dest[len++] = *ptr;
        }
        ptr++;
    }
    dest[len] = '\0';
    *input = ptr;
    return len;
}

int copy_path_argument(const char *input, char *dest, size_t max_len) {
    if (!input || !dest || max_len == 0) {
        if (dest && max_len > 0) {
            dest[0] = '\0';
        }
        return 0;
    }
    size_t write = 0;
    while (*input && *input != '\n' && *input != '\r') {
        if (write + 1 >= max_len) {
            dest[write] = '\0';
            return -1;
        }
        dest[write++] = *input++;
    }
    while (write > 0 && (dest[write - 1] == ' ' || dest[write - 1] == '\t')) {
        write--;
    }
    dest[write] = '\0';
    return (int)write;
}

void print_unsigned(uint32_t value) {
    char buffer[16];
    int pos = 0;
    if (value == 0) {
        console_putchar('0');
        return;
    }
    while (value > 0 && pos < (int)sizeof(buffer)) {
        buffer[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        console_putchar(buffer[i]);
    }
}

void print_decimal(int value) {
    uint32_t magnitude;
    if (value < 0) {
        console_putchar('-');
        magnitude = (uint32_t)(- (value + 1)) + 1;
    } else {
        magnitude = (uint32_t)value;
    }
    print_unsigned(magnitude);
}

const char *fat12_error_string(int code) {
    switch (code) {
        case FAT12_OK: return "ok";
        case FAT12_ERR_IO: return "io";
        case FAT12_ERR_BAD_BPB: return "bad bpb";
        case FAT12_ERR_NOT_FAT12: return "not fat12";
        case FAT12_ERR_OUT_OF_RANGE: return "range";
        case FAT12_ERR_NO_FREE_CLUSTER: return "disk full";
        case FAT12_ERR_INVALID_NAME: return "name";
        case FAT12_ERR_NOT_FOUND: return "not found";
        case FAT12_ERR_NOT_DIRECTORY: return "not dir";
        case FAT12_ERR_ALREADY_EXISTS: return "exists";
        case FAT12_ERR_DIR_FULL: return "dir full";
        case FAT12_ERR_BUFFER_SMALL: return "buffer";
        case FAT12_ERR_NOT_FILE: return "not file";
        case FAT12_ERR_NOT_INITIALIZED: return "fs offline";
        default: return "unknown";
    }
}

void print_fs_error(int code) {
    console_print(" (");
    console_print(fat12_error_string(code));
    console_print(" code ");
    print_decimal(code);
    console_print(")");
}

typedef struct {
    int count;
} ls_context_t;

static int ls_callback(const fat12_dir_entry_info_t *entry, void *context) {
    ls_context_t *ctx = (ls_context_t *)context;
    ctx->count++;
    if (entry->attr & FAT12_ATTR_DIRECTORY) {
        console_print("[DIR] ");
    } else {
        console_print("      ");
    }
    console_print(entry->name);
    if ((entry->attr & FAT12_ATTR_DIRECTORY) == 0) {
        console_print(" (");
        print_unsigned(entry->size);
        console_print(" bytes)");
    }
    console_print("\n");
    return 0;
}

/* Command handlers */
void handle_clear(void) {
    vga_clear();
}

void handle_echo(const char *args) {
    if (args && *args) {
        console_print(args);
    }
    console_print("\n");
}

void handle_fetch(void) {
    console_print("OS: ");
    console_print(os_name);
    console_print("\n");
    
    console_print("Version: ");
    console_print(os_version);
    console_print("\n");
    
    console_print("Architecture: ");
    console_print(os_arch);
    console_print("\n");
    
    console_print("Build Date: ");
    console_print(build_date);
    console_print("\n");
    
    console_print("Build Time: ");
    console_print(build_time);
    console_print("\n");
}

void handle_help(void) {
    console_print("Available commands:\n");
    console_print("  clear          - Clear the screen\n");
    console_print("  echo TEXT      - Print text to the screen\n");
    console_print("  fetch          - Print OS and system information\n");
    console_print("  disk           - Test disk I/O and show disk information\n");
    console_print("  ls [PATH]      - List files in the current or given directory\n");
    console_print("  dir [PATH]     - Alias for ls\n");
    console_print("  pwd            - Show current directory\n");
    console_print("  cd PATH        - Change directory\n");
    console_print("  cat FILE       - Print file contents\n");
    console_print("  touch FILE     - Create a zero-length file\n");
    console_print("  write FILE TXT - Create/overwrite a text file\n");
    console_print("  mkdir NAME     - Create a directory\n");
    console_print("  rm FILE        - Delete a file\n");
    console_print("  nano FILE      - Text editor (Ctrl+S save, Ctrl+X exit)\n");
    console_print("  theme [OPTION] - Switch theme (normal/blue/green) or 'list'\n");
    console_print("  shutdown       - Shut down the system\n");
    console_print("  help           - Display this help message\n");
}

void handle_shutdown(void) {
    console_print("Attempting system shutdown...\n");
    if (fat_ready) {
        fat12_flush();
    }
    console_print("Halting CPU...\n");
    halt_cpu();
}

void handle_disk(void) {
    console_print("Disk Information:\n");
    
    /* Test disk initialization */
    int result = disk_init();
    if (result != 0) {
        console_print("  Disk initialization FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
        return;
    }
    
    console_print("  Disk initialization: OK\n");
    
    /* Test reading first sector */
    uint8_t buffer[512];
    result = disk_read_sector(0, buffer);
    if (result != 0) {
        console_print("  Sector 0 read: FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
        return;
    }
    
    console_print("  Sector 0 read: OK\n");
    
    /* Show first 64 bytes of MBR as hex dump */
    console_print("  First 64 bytes of sector 0:\n");
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) {
            console_print("    ");
        }
        
        /* Print hex value (simplified) */
        uint8_t byte = buffer[i];
        char hex_chars[] = "0123456789ABCDEF";
        char hex_byte[3];
        hex_byte[0] = hex_chars[(byte >> 4) & 0xF];
        hex_byte[1] = hex_chars[byte & 0xF];
        hex_byte[2] = '\0';
        console_print(" ");
        console_print(hex_byte);
        
        if (i % 16 == 15) {
            console_print("\n");
        }
    }
    if (64 % 16 != 0) {
        console_print("\n");
    }
    
    console_print("  Disk self-test: ");
    result = disk_self_test();
    if (result != 0) {
        console_print("FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
    } else {
        console_print("OK\n");
    }
}

void handle_ls(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[FAT12_PATH_MAX];
    int path_len = 0;
    if (path && *path) {
        path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
        if (path_len < 0) {
            console_print("ls failed (path too long)\n");
            return;
        }
    }
    ls_context_t ctx;
    ctx.count = 0;
    int result;
    if (path_len > 0) {
        result = fat12_iterate_path(path_buf, ls_callback, &ctx);
    } else {
        result = fat12_iterate_current_directory(ls_callback, &ctx);
    }
    if (result != FAT12_OK) {
        console_print("ls failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    if (ctx.count == 0) {
        console_print("(empty)\n");
    }
}

void handle_pwd(void) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    console_print(fat12_get_cwd());
    console_print("\n");
}

void handle_cd(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[FAT12_PATH_MAX];
    int path_len = 0;
    if (path && *path) {
        path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
        if (path_len < 0) {
            console_print("cd failed (path too long)\n");
            return;
        }
    }
    if (path_len == 0) {
        path_buf[0] = '/';
        path_buf[1] = '\0';
    }
    int result = fat12_change_directory(path_buf);
    if (result != FAT12_OK) {
        console_print("cd failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print(fat12_get_cwd());
    console_print("\n");
}

void handle_cat(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[FAT12_PATH_MAX];
    int path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
    if (path_len < 0) {
        console_print("cat failed (path too long)\n");
        return;
    }
    if (path_len == 0) {
        console_print("Usage: cat FILE\n");
        return;
    }
    uint32_t size = 0;
    int result = fat12_read_file(path_buf, fs_io_buffer, FS_IO_BUFFER_SIZE - 1, &size);
    if (result != FAT12_OK) {
        console_print("cat failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    for (uint32_t i = 0; i < size; i++) {
        console_putchar((char)fs_io_buffer[i]);
    }
    if (size == 0 || fs_io_buffer[size - 1] != '\n') {
        console_print("\n");
    }
}

void handle_touch(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[FAT12_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: touch NAME\n");
        return;
    }
    int result = fat12_write_file(name_buf, 0, 0);
    if (result != FAT12_OK) {
        console_print("touch failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Created empty file: ");
    console_print(name_buf);
    console_print("\n");
}

void handle_write_command(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[FAT12_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: write NAME TEXT\n");
        return;
    }
    const char *payload = skip_whitespace(cursor);
    uint32_t length = 0;
    if (payload) {
        while (payload[length] != '\0') {
            if (length >= FS_IO_BUFFER_SIZE) {
                console_print("write failed (data too large)\n");
                return;
            }
            fs_io_buffer[length] = (uint8_t)payload[length];
            length++;
        }
    }
    int result = fat12_write_file(name_buf, fs_io_buffer, length);
    if (result != FAT12_OK) {
        console_print("write failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Wrote ");
    print_unsigned(length);
    console_print(" bytes\n");
}

void handle_mkdir_command(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[FAT12_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: mkdir NAME\n");
        return;
    }
    int result = fat12_create_directory(name_buf);
    if (result != FAT12_OK) {
        console_print("mkdir failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Directory created\n");
}

void handle_rm_command(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[FAT12_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: rm NAME\n");
        return;
    }
    int result = fat12_delete_file(name_buf);
    if (result != FAT12_OK) {
        console_print("rm failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("File deleted\n");
}

void handle_theme_command(const char *args) {
    const char *cursor = args;
    char option_buf[32];
    if (read_token(&cursor, option_buf, sizeof(option_buf)) == 0) {
        console_print("Current theme: ");
        console_print(themes[current_theme].name);
        console_print("\nUsage: theme [normal|blue|green|list]\n");
        return;
    }
    
    if (strcmp_impl(option_buf, "list") == 0) {
        console_print("Available themes:\n");
        for (int i = 0; i < THEME_COUNT; i++) {
            console_print("  ");
            console_print(themes[i].name);
            if (i == current_theme) {
                console_print(" (current)");
            }
            console_print("\n");
        }
        return;
    }
    
    int found = -1;
    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcmp_impl(option_buf, themes[i].name) == 0) {
            found = i;
            break;
        }
    }
    
    if (found == -1) {
        console_print("Unknown theme: ");
        console_print(option_buf);
        console_print("\nAvailable: normal, blue, green\n");
        return;
    }
    
    current_theme = found;
    console_print("Theme changed to: ");
    console_print(themes[current_theme].name);
    console_print("\n");
    
    vga_clear();
    console_print("Theme applied: ");
    console_print(themes[current_theme].name);
    console_print("\n\n");
}

/* Parse and execute commands */
void execute_command(const char *cmd_line) {
    if (!cmd_line || *cmd_line == '\0') {
        return;
    }

    /* Skip leading whitespace */
    while (*cmd_line && (*cmd_line == ' ' || *cmd_line == '\t')) {
        cmd_line++;
    }

    /* Parse command */
    if (strncmp_impl(cmd_line, "clear", 5) == 0 && 
        (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        handle_clear();
    } else if (strncmp_impl(cmd_line, "echo ", 5) == 0) {
        const char *args = cmd_line + 5;
        handle_echo(args);
    } else if (strncmp_impl(cmd_line, "fetch", 5) == 0 && 
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        handle_fetch();
    } else if (strncmp_impl(cmd_line, "disk", 4) == 0 && 
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        handle_disk();
    } else if (strncmp_impl(cmd_line, "ls", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_ls(args);
    } else if (strncmp_impl(cmd_line, "dir", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        const char *args = cmd_line + 3;
        handle_ls(args);
    } else if (strncmp_impl(cmd_line, "pwd", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        handle_pwd();
    } else if (strncmp_impl(cmd_line, "cd", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_cd(args);
    } else if (strncmp_impl(cmd_line, "cat", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        const char *args = cmd_line + 3;
        handle_cat(args);
    } else if (strncmp_impl(cmd_line, "touch", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_touch(args);
    } else if (strncmp_impl(cmd_line, "write", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_write_command(args);
    } else if (strncmp_impl(cmd_line, "mkdir", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_mkdir_command(args);
    } else if (strncmp_impl(cmd_line, "rm", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_rm_command(args);
    } else if (strncmp_impl(cmd_line, "nano", 4) == 0 &&
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        const char *args = cmd_line + 4;
        handle_nano_command(args);
    } else if (strncmp_impl(cmd_line, "theme", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_theme_command(args);
    } else if (strncmp_impl(cmd_line, "shutdown", 8) == 0 && 
               (cmd_line[8] == '\0' || cmd_line[8] == ' ' || cmd_line[8] == '\n')) {
        handle_shutdown();
    } else if (strncmp_impl(cmd_line, "help", 4) == 0 && 
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        handle_help();
    } else {
        console_print("Unknown command: ");
        console_print(cmd_line);
        console_print("\n");
    }
}

/* Nano editor implementation */
void handle_nano_command(const char *args) {
    if (!fat_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    
    const char *cursor = args;
    char filename_buf[FAT12_PATH_MAX];
    if (read_token(&cursor, filename_buf, sizeof(filename_buf)) == 0) {
        console_print("Usage: nano FILENAME\n");
        return;
    }
    
    nano_init_editor(filename_buf);
}

void nano_init_editor(const char *filename) {
    /* Initialize editor state */
    strcpy_impl(nano_filename, filename);
    nano_total_lines = 0;
    nano_cursor_x = 0;
    nano_cursor_y = 0;
    nano_viewport_y = 0;
    nano_dirty = 0;
    nano_editor_active = 1;
    
    /* Clear all lines */
    for (int i = 0; i < NANO_MAX_LINES; i++) {
        nano_line_lengths[i] = 0;
        nano_lines[i][0] = '\0';
    }
    
    /* Try to load existing file */
    uint32_t file_size = 0;
    int result = fat12_read_file(filename, fs_io_buffer, FS_IO_BUFFER_SIZE - 1, &file_size);
    
    if (result == FAT12_OK && file_size > 0) {
        /* Parse file into lines */
        int line = 0;
        int line_pos = 0;
        
        for (uint32_t i = 0; i < file_size && line < NANO_MAX_LINES; i++) {
            char c = (char)fs_io_buffer[i];
            
            if (c == '\n') {
                /* End of line */
                nano_lines[line][line_pos] = '\0';
                nano_line_lengths[line] = line_pos;
                line++;
                line_pos = 0;
            } else if (c != '\r' && line_pos < NANO_MAX_LINE_LENGTH - 1) {
                /* Regular character */
                nano_lines[line][line_pos++] = c;
            }
        }
        
        /* Handle last line if file doesn't end with newline */
        if (line_pos > 0 && line < NANO_MAX_LINES) {
            nano_lines[line][line_pos] = '\0';
            nano_line_lengths[line] = line_pos;
            line++;
        }
        
        nano_total_lines = line;
    } else {
        /* File doesn't exist or is empty, start with one empty line */
        nano_total_lines = 1;
        nano_line_lengths[0] = 0;
        nano_lines[0][0] = '\0';
    }
    
    /* Clear screen and start editor */
    vga_clear();
    nano_render_editor();
}

void nano_render_editor(void) {
    uint8_t text_attr = get_current_text_attr();
    uint8_t status_attr = get_current_status_attr();
    
    /* Clear screen */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = text_attr;
    }
    
    /* Render text viewport */
    for (int i = 0; i < NANO_VIEWPORT_HEIGHT; i++) {
        int line_index = nano_viewport_y + i;
        if (line_index < nano_total_lines) {
            /* Render line content */
            for (int j = 0; j < nano_line_lengths[line_index] && j < VGA_WIDTH; j++) {
                size_t pos = i * VGA_WIDTH + j;
                VGA_BUFFER[pos * 2] = nano_lines[line_index][j];
                VGA_BUFFER[pos * 2 + 1] = text_attr;
            }
        }
    }
    
    /* Render status bar */
    int status_line = NANO_VIEWPORT_HEIGHT;
    for (int i = 0; i < VGA_WIDTH; i++) {
        size_t pos = status_line * VGA_WIDTH + i;
        VGA_BUFFER[pos * 2] = ' ';
        VGA_BUFFER[pos * 2 + 1] = status_attr;
    }
    
    /* Status bar content */
    const char *status_text = "\"";
    console_print_to_pos(status_line, 0, status_text);
    console_print_to_pos(status_line, 1, nano_filename);
    console_print_to_pos(status_line, 1 + strlen_impl(nano_filename), "\" ");
    
    if (nano_dirty) {
        console_print_to_pos(status_line, 1 + strlen_impl(nano_filename) + 3, "[Modified] ");
    }
    
    console_print_to_pos(status_line, VGA_WIDTH - 30, "Theme:");
    console_print_to_pos(status_line, VGA_WIDTH - 23, themes[current_theme].name);
    console_print_to_pos(status_line, VGA_WIDTH - 20, " Ctrl+S:Save Ctrl+X:Exit");
    
    /* Update hardware cursor position */
    int screen_x = nano_cursor_x;
    int screen_y = nano_cursor_y - nano_viewport_y;
    if (screen_y >= 0 && screen_y < NANO_VIEWPORT_HEIGHT && screen_x < VGA_WIDTH) {
        update_hardware_cursor(screen_x, screen_y);
    }
}

void nano_handle_keyboard(void) {
    uint8_t scancode = read_keyboard();
    
    /* Track Ctrl key state */
    if (scancode == 0x1D) {
        ctrl_pressed = 1;
        return;
    } else if (scancode == 0x9D) {
        ctrl_pressed = 0;
        return;
    }
    
    /* Only handle key press (not release) */
    if (scancode & 0x80) {
        return;
    }
    
    /* Handle Ctrl key combinations */
    if (ctrl_pressed) {
        if (scancode == 0x1F) {
            /* Ctrl+S - Save */
            nano_save_file();
            nano_render_editor();
            return;
        } else if (scancode == 0x2D) {
            /* Ctrl+X - Exit */
            nano_exit_editor();
            return;
        }
    }
    
    /* Handle special key combinations first */
    switch (scancode) {
        case 0x48:  /* Up arrow */
            nano_move_cursor(0, -1);
            nano_render_editor();
            return;
        case 0x50:  /* Down arrow */
            nano_move_cursor(0, 1);
            nano_render_editor();
            return;
        case 0x4B:  /* Left arrow */
            nano_move_cursor(-1, 0);
            nano_render_editor();
            return;
        case 0x4D:  /* Right arrow */
            nano_move_cursor(1, 0);
            nano_render_editor();
            return;
        case 0x0E:  /* Backspace */
            nano_handle_backspace();
            nano_render_editor();
            return;
        case 0x1C:  /* Enter */
            nano_handle_enter();
            nano_render_editor();
            return;
    }
    
    /* Convert scancode to ASCII for regular keys */
    char c = 0;
    switch (scancode) {
        case 0x02: c = '1'; break;
        case 0x03: c = '2'; break;
        case 0x04: c = '3'; break;
        case 0x05: c = '4'; break;
        case 0x06: c = '5'; break;
        case 0x07: c = '6'; break;
        case 0x08: c = '7'; break;
        case 0x09: c = '8'; break;
        case 0x0A: c = '9'; break;
        case 0x0B: c = '0'; break;
        case 0x0C: c = '-'; break;
        case 0x0D: c = '='; break;
        case 0x10: c = 'q'; break;
        case 0x11: c = 'w'; break;
        case 0x12: c = 'e'; break;
        case 0x13: c = 'r'; break;
        case 0x14: c = 't'; break;
        case 0x15: c = 'y'; break;
        case 0x16: c = 'u'; break;
        case 0x17: c = 'i'; break;
        case 0x18: c = 'o'; break;
        case 0x19: c = 'p'; break;
        case 0x1A: c = '['; break;
        case 0x1B: c = ']'; break;
        case 0x1E: c = 'a'; break;
        case 0x1F: c = 's'; break;
        case 0x20: c = 'd'; break;
        case 0x21: c = 'f'; break;
        case 0x22: c = 'g'; break;
        case 0x23: c = 'h'; break;
        case 0x24: c = 'j'; break;
        case 0x25: c = 'k'; break;
        case 0x26: c = 'l'; break;
        case 0x27: c = ';'; break;
        case 0x28: c = '\''; break;
        case 0x29: c = '`'; break;
        case 0x2B: c = '\\'; break;
        case 0x2C: c = 'z'; break;
        case 0x2D: c = 'x'; break;
        case 0x2E: c = 'c'; break;
        case 0x2F: c = 'v'; break;
        case 0x30: c = 'b'; break;
        case 0x31: c = 'n'; break;
        case 0x32: c = 'm'; break;
        case 0x33: c = ','; break;
        case 0x34: c = '.'; break;
        case 0x35: c = '/'; break;
        case 0x39: c = ' '; break;
    }
    
    if (c >= ' ' && c <= '~') {
        nano_insert_char(c);
        nano_render_editor();
    }
}

void nano_insert_char(char c) {
    if (nano_cursor_y >= NANO_MAX_LINES) {
        return;
    }
    
    if (nano_cursor_x >= nano_line_lengths[nano_cursor_y] && nano_cursor_x < NANO_MAX_LINE_LENGTH - 1) {
        /* Append to end of line */
        nano_lines[nano_cursor_y][nano_cursor_x] = c;
        nano_lines[nano_cursor_y][nano_cursor_x + 1] = '\0';
        nano_line_lengths[nano_cursor_y]++;
        nano_cursor_x++;
    } else if (nano_cursor_x < nano_line_lengths[nano_cursor_y] && nano_cursor_x < NANO_MAX_LINE_LENGTH - 1) {
        /* Insert in middle of line - shift characters right */
        for (int i = nano_line_lengths[nano_cursor_y]; i > nano_cursor_x; i--) {
            if (i < NANO_MAX_LINE_LENGTH - 1) {
                nano_lines[nano_cursor_y][i] = nano_lines[nano_cursor_y][i - 1];
            }
        }
        nano_lines[nano_cursor_y][nano_cursor_x] = c;
        if (nano_line_lengths[nano_cursor_y] < NANO_MAX_LINE_LENGTH - 1) {
            nano_line_lengths[nano_cursor_y]++;
        }
        nano_lines[nano_cursor_y][nano_line_lengths[nano_cursor_y]] = '\0';
        nano_cursor_x++;
    }
    
    nano_dirty = 1;
}

void nano_handle_backspace(void) {
    if (nano_cursor_y >= NANO_MAX_LINES) {
        return;
    }
    
    if (nano_cursor_x > 0) {
        /* Delete character within line */
        for (int i = nano_cursor_x - 1; i < nano_line_lengths[nano_cursor_y]; i++) {
            nano_lines[nano_cursor_y][i] = nano_lines[nano_cursor_y][i + 1];
        }
        if (nano_line_lengths[nano_cursor_y] > 0) {
            nano_line_lengths[nano_cursor_y]--;
        }
        nano_lines[nano_cursor_y][nano_line_lengths[nano_cursor_y]] = '\0';
        nano_cursor_x--;
    } else if (nano_cursor_y > 0) {
        /* Join with previous line */
        int prev_line_len = nano_line_lengths[nano_cursor_y - 1];
        int current_line_len = nano_line_lengths[nano_cursor_y];
        
        if (prev_line_len + current_line_len < NANO_MAX_LINE_LENGTH) {
            /* Move current line content to previous line */
            for (int i = 0; i < current_line_len; i++) {
                nano_lines[nano_cursor_y - 1][prev_line_len + i] = nano_lines[nano_cursor_y][i];
            }
            nano_line_lengths[nano_cursor_y - 1] += current_line_len;
            nano_lines[nano_cursor_y - 1][nano_line_lengths[nano_cursor_y - 1]] = '\0';
            
            /* Remove current line and shift lines up */
            for (int i = nano_cursor_y; i < nano_total_lines - 1; i++) {
                nano_line_lengths[i] = nano_line_lengths[i + 1];
                strcpy_impl(nano_lines[i], nano_lines[i + 1]);
            }
            nano_total_lines--;
            nano_cursor_y--;
            nano_cursor_x = prev_line_len;
        }
    }
    
    nano_dirty = 1;
}

void nano_handle_enter(void) {
    if (nano_total_lines >= NANO_MAX_LINES) {
        return;
    }
    
    /* Split current line at cursor position */
    int current_line_len = nano_line_lengths[nano_cursor_y];
    
    /* Shift all lines below down */
    for (int i = nano_total_lines; i > nano_cursor_y + 1; i--) {
        nano_line_lengths[i] = nano_line_lengths[i - 1];
        strcpy_impl(nano_lines[i], nano_lines[i - 1]);
    }
    
    /* Create new line with content after cursor */
    int new_line_len = 0;
    for (int i = nano_cursor_x; i < current_line_len; i++) {
        nano_lines[nano_cursor_y + 1][new_line_len++] = nano_lines[nano_cursor_y][i];
    }
    nano_lines[nano_cursor_y + 1][new_line_len] = '\0';
    nano_line_lengths[nano_cursor_y + 1] = new_line_len;
    
    /* Truncate current line at cursor */
    nano_lines[nano_cursor_y][nano_cursor_x] = '\0';
    nano_line_lengths[nano_cursor_y] = nano_cursor_x;
    
    nano_total_lines++;
    nano_cursor_y++;
    nano_cursor_x = 0;
    nano_dirty = 1;
}

void nano_move_cursor(int dx, int dy) {
    int new_x = nano_cursor_x + dx;
    int new_y = nano_cursor_y + dy;
    
    /* Clamp Y */
    if (new_y < 0) new_y = 0;
    if (new_y >= nano_total_lines) new_y = nano_total_lines - 1;
    
    /* Clamp X */
    if (new_x < 0) new_x = 0;
    if (new_x > nano_line_lengths[new_y]) new_x = nano_line_lengths[new_y];
    
    nano_cursor_x = new_x;
    nano_cursor_y = new_y;
    
    nano_scroll_to_cursor();
}

void nano_scroll_to_cursor(void) {
    if (nano_cursor_y < nano_viewport_y) {
        nano_viewport_y = nano_cursor_y;
    } else if (nano_cursor_y >= nano_viewport_y + NANO_VIEWPORT_HEIGHT) {
        nano_viewport_y = nano_cursor_y - NANO_VIEWPORT_HEIGHT + 1;
    }
    
    if (nano_viewport_y < 0) nano_viewport_y = 0;
    if (nano_viewport_y > nano_total_lines - NANO_VIEWPORT_HEIGHT) {
        nano_viewport_y = nano_total_lines - NANO_VIEWPORT_HEIGHT;
    }
    if (nano_viewport_y < 0) nano_viewport_y = 0;
}

void nano_save_file(void) {
    if (!nano_dirty) {
        /* File is already saved */
        return;
    }
    
    /* Convert lines back to file format */
    uint32_t file_size = 0;
    for (int i = 0; i < nano_total_lines && file_size < FS_IO_BUFFER_SIZE - 1; i++) {
        for (int j = 0; j < nano_line_lengths[i] && file_size < FS_IO_BUFFER_SIZE - 2; j++) {
            fs_io_buffer[file_size++] = (uint8_t)nano_lines[i][j];
        }
        if (file_size < FS_IO_BUFFER_SIZE - 1) {
            fs_io_buffer[file_size++] = '\n';
        }
    }
    
    /* Write to file */
    int result = fat12_write_file(nano_filename, fs_io_buffer, file_size);
    if (result == FAT12_OK) {
        nano_dirty = 0;
    }
}

void nano_exit_editor(void) {
    int was_dirty = nano_dirty;
    
    if (nano_dirty) {
        nano_save_file();
    }
    
    nano_editor_active = 0;
    
    /* Restore shell screen */
    vga_clear();
    console_print("Welcome to ");
    console_print(os_name);
    console_print(" ");
    console_print(os_version);
    console_print("\n\n");
    
    if (was_dirty) {
        console_print("File saved: ");
        console_print(nano_filename);
        console_print("\n");
    }
    
    /* Show current directory */
    if (fat_ready) {
        console_print("Current directory: ");
        console_print(fat12_get_cwd());
        console_print("\n");
    }
}

/* Helper function to print at specific position (for status bar) */
void console_print_to_pos(int y, int x, const char *str) {
    if (!str) return;
    uint8_t status_attr = get_current_status_attr();
    for (int i = 0; str[i] != '\0' && x + i < VGA_WIDTH; i++) {
        size_t pos = y * VGA_WIDTH + (x + i);
        VGA_BUFFER[pos * 2] = str[i];
        VGA_BUFFER[pos * 2 + 1] = status_attr;
    }
}

/* Main kernel function */
void kernel_main(void) {
    vga_clear();
    console_print("Welcome to ");
    console_print(os_name);
    console_print(" ");
    console_print(os_version);
    console_print("\n");
    
    /* Initialize disk driver */
    console_print("Initializing disk driver... ");
    int disk_result = disk_init();
    if (disk_result != 0) {
        console_print("FAILED (error ");
        /* Print error code */
        if (disk_result < 0) {
            console_print("-");
            disk_result = -disk_result;
        }
        char error_str[16];
        int pos = 0;
        if (disk_result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (disk_result > 0) {
                temp[temp_pos++] = '0' + (disk_result % 10);
                disk_result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
    } else {
        console_print("OK\n");
        
        /* Run disk self-test */
        console_print("Running disk self-test... ");
        int test_result = disk_self_test();
        if (test_result != 0) {
            console_print("FAILED (error ");
            if (test_result < 0) {
                console_print("-");
                test_result = -test_result;
            }
            char error_str[16];
            int pos = 0;
            if (test_result == 0) {
                error_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (test_result > 0) {
                    temp[temp_pos++] = '0' + (test_result % 10);
                    test_result /= 10;
                }
                for (int i = temp_pos - 1; i >= 0; i--) {
                    error_str[pos++] = temp[i];
                }
            }
            error_str[pos] = '\0';
            console_print(error_str);
            console_print(")\n");
        } else {
            console_print("OK\n");
        }
    }
    
    if (disk_result == 0) {
        console_print("Initializing FAT12 filesystem... ");
        int fat_result = fat12_init(0);
        if (fat_result != FAT12_OK) {
            console_print("FAILED");
            print_fs_error(fat_result);
            console_print("\n");
        } else {
            console_print("OK\n");
            fat_ready = 1;
            console_print("Mounted volume at ");
            console_print(fat12_get_cwd());
            console_print("\n");
        }
    }
    
    console_print("Type 'help' for available commands\n\n");
    
    /* Main command loop */
    while (1) {
        /* Initialize prompt line at current cursor position */
        prompt_line_start_x = cursor_x;
        prompt_line_y = cursor_y;
        
        /* Reset input buffer */
        input_pos = 0;
        command_executed = 0;
        
        /* Draw initial prompt (just the > marker) */
        render_prompt_line();
        
        /* Wait for command input */
        while (1) {
            if (keyboard_ready()) {
                handle_keyboard_input();
                /* Check if command was executed (Enter pressed) */
                if (command_executed) {
                    /* Command was executed, break to show new prompt */
                    break;
                }
            }
        }
    }
}
