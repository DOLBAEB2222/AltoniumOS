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
void handle_write_command(const char *args);
void handle_mkdir_command(const char *args);
void handle_rm_command(const char *args);
void execute_command(const char *cmd_line);
void handle_keyboard_input(void);
void update_hardware_cursor(int x, int y);
void render_prompt_line(void);

/* External: defined in assembly */
extern void halt_cpu(void);

/* Include disk driver */
#include "disk.h"
#include "fat12.h"

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

void handle_keyboard_input(void) {
    uint8_t scancode = read_keyboard();
    
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
    
    /* Clear from prompt start to end of line */
    for (int i = prompt_line_start_x; i < VGA_WIDTH; i++) {
        size_t pos = line_y * VGA_WIDTH + i;
        VGA_BUFFER[pos * 2] = ' ';
        VGA_BUFFER[pos * 2 + 1] = VGA_ATTR_DEFAULT;
    }
    
    /* Write input buffer content */
    for (int i = 0; i < input_pos; i++) {
        size_t pos = line_y * VGA_WIDTH + (prompt_line_start_x + i);
        if (prompt_line_start_x + i < VGA_WIDTH) {
            VGA_BUFFER[pos * 2] = input_buffer[i];
            VGA_BUFFER[pos * 2 + 1] = VGA_ATTR_DEFAULT;
        }
    }
    
    /* Write prompt marker (>) at the end of input */
    if (prompt_line_start_x + input_pos < VGA_WIDTH) {
        size_t pos = line_y * VGA_WIDTH + (prompt_line_start_x + input_pos);
        VGA_BUFFER[pos * 2] = '>';
        VGA_BUFFER[pos * 2 + 1] = VGA_ATTR_DEFAULT;
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
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = VGA_ATTR_DEFAULT;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor(cursor_x, cursor_y);
}

/* Print a string */
void console_print(const char *str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        vga_write_char(str[i], VGA_ATTR_DEFAULT);
    }
}

/* Print a single character */
void console_putchar(char c) {
    vga_write_char(c, VGA_ATTR_DEFAULT);
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
    console_print("  pwd            - Show current directory\n");
    console_print("  cd PATH        - Change directory\n");
    console_print("  cat FILE       - Print file contents\n");
    console_print("  write FILE TXT - Create/overwrite a text file\n");
    console_print("  mkdir NAME     - Create a directory\n");
    console_print("  rm FILE        - Delete a file\n");
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
