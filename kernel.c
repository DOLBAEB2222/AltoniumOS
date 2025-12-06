/* Minimal type definitions - no libc */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long size_t;
typedef unsigned long uintptr_t;
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
static int extended_scancode_pending = 0;

/* Nano editor status */
#define NANO_EXIT_SAVE      1
#define NANO_EXIT_DISCARD   0

#define NANO_PROMPT_NONE         0
#define NANO_PROMPT_SAVE_CONFIRM 1

static int nano_prompt_state = NANO_PROMPT_NONE;
static char nano_status_message[VGA_WIDTH + 1];
static int nano_help_overlay_visible = 0;

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

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

extern uint32_t multiboot_magic_storage;
extern uint32_t multiboot_info_ptr_storage;

enum {
    BOOT_MODE_UNKNOWN = 0,
    BOOT_MODE_BIOS = 1,
    BOOT_MODE_UEFI = 2
};

static int boot_mode = BOOT_MODE_UNKNOWN;

static void detect_boot_mode(void);
static const char *get_boot_mode_name(void);
static int string_contains(const char *haystack, const char *needle);

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

char scancode_to_ascii(uint16_t scancode);
void handle_console_scancode(uint16_t scancode);

/* Nano editor functions */
void nano_init_editor(const char *filename);
void nano_render_editor(void);
void nano_handle_scancode(uint16_t scancode, int is_release);
int nano_save_file(void);
void nano_exit_editor(int save_action);
void nano_insert_char(char c);
void nano_handle_backspace(void);
void nano_handle_delete(void);
void nano_handle_enter(void);
void nano_move_cursor(int dx, int dy);
void nano_move_to_line_start(void);
void nano_move_to_line_end(void);
void nano_page_scroll(int direction);
void nano_scroll_to_cursor(void);
void nano_cycle_theme(void);
void nano_show_help_overlay(void);
void nano_render_help_overlay(void);
void nano_set_status_message(const char *msg);
void nano_reset_status_message(void);
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
    uint8_t raw_scancode = read_keyboard();
    
    if (raw_scancode == 0xE0) {
        extended_scancode_pending = 1;
        return;
    }
    
    int is_release = (raw_scancode & 0x80) ? 1 : 0;
    uint8_t base_code = raw_scancode & 0x7F;
    int extended = extended_scancode_pending;
    extended_scancode_pending = 0;
    uint16_t scancode = extended ? (0xE000 | base_code) : base_code;
    
    if (base_code == 0x1D) {
        ctrl_pressed = is_release ? 0 : 1;
        return;
    }
    
    if (nano_editor_active) {
        nano_handle_scancode(scancode, is_release);
        return;
    }
    
    if (is_release) {
        return;
    }
    
    handle_console_scancode(scancode);
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
    while (str && str[len]) len++;
    return len;
}

static int string_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return 0;
    }
    size_t needle_len = strlen_impl(needle);
    if (needle_len == 0) {
        return 0;
    }
    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (haystack[i + j] != '\0' && needle[j] != '\0' && haystack[i + j] == needle[j]) {
            j++;
        }
        if (j == needle_len) {
            return 1;
        }
    }
    return 0;
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

static void detect_boot_mode(void) {
    boot_mode = BOOT_MODE_BIOS;
    if (multiboot_magic_storage != MULTIBOOT_BOOTLOADER_MAGIC) {
        boot_mode = BOOT_MODE_UNKNOWN;
        return;
    }
    multiboot_info_t *info = (multiboot_info_t *)(uintptr_t)multiboot_info_ptr_storage;
    if (!info) {
        return;
    }
    if ((info->flags & (1u << 2)) != 0 && info->cmdline != 0) {
        const char *cmdline = (const char *)(uintptr_t)info->cmdline;
        if (string_contains(cmdline, "bootmode=uefi")) {
            boot_mode = BOOT_MODE_UEFI;
        }
    }
}

static const char *get_boot_mode_name(void) {
    switch (boot_mode) {
        case BOOT_MODE_UEFI: return "UEFI";
        case BOOT_MODE_BIOS: return "BIOS";
        default: return "Unknown";
    }
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

    console_print("Boot Mode: ");
    console_print(get_boot_mode_name());
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
    console_print("  nano FILE      - Text editor (Ctrl+S/Ctrl+X/Ctrl+T/Ctrl+H)\n");
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

char scancode_to_ascii(uint16_t scancode) {
    if (scancode & 0xE000) {
        return 0;
    }
    switch (scancode) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x29: return '`';
        case 0x2B: return '\\';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        case 0x0E: return '\b';
        case 0x1C: return '\n';
        case 0x0F: return '\t';
        default: return 0;
    }
}

void handle_console_scancode(uint16_t scancode) {
    char c = scancode_to_ascii(scancode);
    
    if (c == '\n') {
        console_putchar('\n');
        input_buffer[input_pos] = '\0';
        execute_command(input_buffer);
        input_pos = 0;
        command_executed = 1;
    } else if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            render_prompt_line();
        }
    } else if (c >= ' ' && c <= '~') {
        if (input_pos < (int)(sizeof(input_buffer) - 1)) {
            input_buffer[input_pos++] = c;
            render_prompt_line();
        }
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
    nano_prompt_state = NANO_PROMPT_NONE;
    nano_help_overlay_visible = 0;
    nano_reset_status_message();
    
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
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = text_attr;
    }
    
    for (int row = 0; row < NANO_VIEWPORT_HEIGHT && row < VGA_HEIGHT; row++) {
        int line_index = nano_viewport_y + row;
        if (line_index >= 0 && line_index < nano_total_lines) {
            for (int col = 0; col < nano_line_lengths[line_index] && col < VGA_WIDTH; col++) {
                size_t pos = row * VGA_WIDTH + col;
                VGA_BUFFER[pos * 2] = nano_lines[line_index][col];
                VGA_BUFFER[pos * 2 + 1] = text_attr;
            }
        }
    }
    
    int status_line = NANO_VIEWPORT_HEIGHT;
    if (status_line < VGA_HEIGHT) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            size_t pos = status_line * VGA_WIDTH + col;
            VGA_BUFFER[pos * 2] = ' ';
            VGA_BUFFER[pos * 2 + 1] = status_attr;
        }
        int status_x = 0;
        console_print_to_pos(status_line, status_x, nano_filename);
        status_x += strlen_impl(nano_filename);
        if (status_x < VGA_WIDTH) {
            console_print_to_pos(status_line, status_x, " | L:");
            status_x += 5;
        }
        char line_buf[16];
        int line_num = nano_cursor_y + 1;
        if (line_num < 1) line_num = 1;
        int temp = line_num;
        int idx = 0;
        char temp_digits[16];
        if (temp == 0) {
            line_buf[idx++] = '0';
        } else {
            int digit_count = 0;
            while (temp > 0 && digit_count < (int)sizeof(temp_digits)) {
                temp_digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            while (digit_count > 0 && idx < (int)sizeof(line_buf) - 1) {
                line_buf[idx++] = temp_digits[--digit_count];
            }
        }
        line_buf[idx] = '\0';
        console_print_to_pos(status_line, status_x, line_buf);
        status_x += strlen_impl(line_buf);
        console_print_to_pos(status_line, status_x, ",C:");
        status_x += 3;
        char col_buf[16];
        int col_num = nano_cursor_x + 1;
        if (col_num < 1) col_num = 1;
        temp = col_num;
        idx = 0;
        if (temp == 0) {
            col_buf[idx++] = '0';
        } else {
            int digit_count = 0;
            while (temp > 0 && digit_count < (int)sizeof(temp_digits)) {
                temp_digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            while (digit_count > 0 && idx < (int)sizeof(col_buf) - 1) {
                col_buf[idx++] = temp_digits[--digit_count];
            }
        }
        col_buf[idx] = '\0';
        console_print_to_pos(status_line, status_x, col_buf);
        status_x += strlen_impl(col_buf);
        if (nano_dirty) {
            console_print_to_pos(status_line, status_x, " | [MODIFIED]");
            status_x += 13;
        }
        console_print_to_pos(status_line, status_x, " | Theme: ");
        status_x += 10;
        char upper_theme[16];
        int theme_idx = 0;
        for (; themes[current_theme].name[theme_idx] != '\0' && theme_idx < (int)sizeof(upper_theme) - 1; theme_idx++) {
            char c = themes[current_theme].name[theme_idx];
            if (c >= 'a' && c <= 'z') {
                c -= 32;
            }
            upper_theme[theme_idx] = c;
        }
        upper_theme[theme_idx] = '\0';
        console_print_to_pos(status_line, status_x, upper_theme);
    }
    
    int message_line = status_line + 1;
    if (message_line < VGA_HEIGHT) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            size_t pos = message_line * VGA_WIDTH + col;
            VGA_BUFFER[pos * 2] = ' ';
            VGA_BUFFER[pos * 2 + 1] = status_attr;
        }
        if (nano_status_message[0] != '\0') {
            console_print_to_pos(message_line, 0, nano_status_message);
        }
    }
    
    int screen_x = nano_cursor_x;
    int screen_y = nano_cursor_y - nano_viewport_y;
    if (screen_y >= 0 && screen_y < NANO_VIEWPORT_HEIGHT && screen_x < VGA_WIDTH) {
        update_hardware_cursor(screen_x, screen_y);
    }
}

void nano_handle_scancode(uint16_t scancode, int is_release) {
    if (nano_help_overlay_visible) {
        if (!is_release) {
            nano_help_overlay_visible = 0;
            nano_reset_status_message();
            nano_render_editor();
        }
        return;
    }
    
    if (nano_prompt_state == NANO_PROMPT_SAVE_CONFIRM) {
        if (is_release) {
            return;
        }
        if (scancode == 0x01) {
            nano_prompt_state = NANO_PROMPT_NONE;
            nano_exit_editor(NANO_EXIT_DISCARD);
            return;
        }
        char response = scancode_to_ascii(scancode);
        if (response == 'y') {
            nano_prompt_state = NANO_PROMPT_NONE;
            if (!nano_save_file()) {
                nano_set_status_message("Save failed.");
                nano_render_editor();
                return;
            }
            nano_exit_editor(NANO_EXIT_SAVE);
        } else if (response == 'n') {
            nano_prompt_state = NANO_PROMPT_NONE;
            nano_exit_editor(NANO_EXIT_DISCARD);
        }
        return;
    }
    
    if (is_release) {
        return;
    }
    
    if (ctrl_pressed) {
        switch (scancode) {
            case 0x1F: /* Ctrl+S */
                if (nano_save_file()) {
                    nano_set_status_message("Saved.");
                } else {
                    nano_set_status_message("Save failed.");
                }
                nano_render_editor();
                return;
            case 0x2D: /* Ctrl+X */
                if (nano_dirty) {
                    nano_prompt_state = NANO_PROMPT_SAVE_CONFIRM;
                    nano_set_status_message("Save changes? (Y=Yes, N=No, Esc=Discard)");
                    nano_render_editor();
                } else {
                    nano_exit_editor(NANO_EXIT_SAVE);
                }
                return;
            case 0x14: /* Ctrl+T */
                nano_cycle_theme();
                return;
            case 0x23: /* Ctrl+H */
                nano_show_help_overlay();
                return;
        }
    }
    
    switch (scancode) {
        case 0x01: /* Escape */
            nano_exit_editor(NANO_EXIT_DISCARD);
            return;
        case 0x0E: /* Backspace */
            nano_handle_backspace();
            break;
        case 0x1C: /* Enter */
            nano_handle_enter();
            break;
        case 0xE053:
        case 0x53: /* Delete */
            nano_handle_delete();
            break;
        case 0xE048:
        case 0x48: /* Up */
            nano_move_cursor(0, -1);
            break;
        case 0xE050:
        case 0x50: /* Down */
            nano_move_cursor(0, 1);
            break;
        case 0xE04B:
        case 0x4B: /* Left */
            nano_move_cursor(-1, 0);
            break;
        case 0xE04D:
        case 0x4D: /* Right */
            nano_move_cursor(1, 0);
            break;
        case 0xE047:
        case 0x47: /* Home */
            nano_move_to_line_start();
            break;
        case 0xE04F:
        case 0x4F: /* End */
            nano_move_to_line_end();
            break;
        case 0xE049:
        case 0x49: /* Page Up */
            nano_page_scroll(-1);
            break;
        case 0xE051:
        case 0x51: /* Page Down */
            nano_page_scroll(1);
            break;
        default: {
            char c = scancode_to_ascii(scancode);
            if (c >= ' ' && c <= '~') {
                nano_insert_char(c);
            } else if (c == '\t') {
                for (int i = 0; i < 4; i++) {
                    nano_insert_char(' ');
                }
            } else {
                return;
            }
        }
    }
    
    nano_render_editor();
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

int nano_save_file(void) {
    if (!nano_dirty) {
        return 1;
    }
    
    uint32_t file_size = 0;
    for (int i = 0; i < nano_total_lines && file_size < FS_IO_BUFFER_SIZE - 1; i++) {
        for (int j = 0; j < nano_line_lengths[i] && file_size < FS_IO_BUFFER_SIZE - 2; j++) {
            fs_io_buffer[file_size++] = (uint8_t)nano_lines[i][j];
        }
        if (file_size < FS_IO_BUFFER_SIZE - 1) {
            fs_io_buffer[file_size++] = '\n';
        }
    }
    
    int result = fat12_write_file(nano_filename, fs_io_buffer, file_size);
    if (result == FAT12_OK) {
        nano_dirty = 0;
        return 1;
    }
    return 0;
}

void nano_exit_editor(int save_action) {
    int had_dirty = nano_dirty;
    if (save_action == NANO_EXIT_SAVE && nano_dirty) {
        if (!nano_save_file()) {
            nano_set_status_message("Save failed.");
            nano_render_editor();
            return;
        }
        had_dirty = 1;
    } else if (save_action == NANO_EXIT_DISCARD) {
        nano_dirty = 0;
    }
    
    nano_editor_active = 0;
    nano_prompt_state = NANO_PROMPT_NONE;
    nano_help_overlay_visible = 0;
    
    vga_clear();
    console_print("Welcome to ");
    console_print(os_name);
    console_print(" ");
    console_print(os_version);
    console_print("\n\n");
    
    if (save_action == NANO_EXIT_SAVE && had_dirty) {
        console_print("File saved: ");
        console_print(nano_filename);
        console_print("\n");
    } else if (save_action == NANO_EXIT_DISCARD && had_dirty) {
        console_print("Changes discarded: ");
        console_print(nano_filename);
        console_print("\n");
    }
    
    if (fat_ready) {
        console_print("Current directory: ");
        console_print(fat12_get_cwd());
        console_print("\n");
    }
}

void nano_handle_delete(void) {
    if (nano_cursor_y >= NANO_MAX_LINES || nano_cursor_y >= nano_total_lines) {
        return;
    }
    
    int line_len = nano_line_lengths[nano_cursor_y];
    if (nano_cursor_x < line_len) {
        for (int i = nano_cursor_x; i < line_len - 1; i++) {
            nano_lines[nano_cursor_y][i] = nano_lines[nano_cursor_y][i + 1];
        }
        nano_line_lengths[nano_cursor_y]--;
        nano_lines[nano_cursor_y][nano_line_lengths[nano_cursor_y]] = '\0';
        nano_dirty = 1;
    } else if (nano_cursor_y < nano_total_lines - 1) {
        int next_line_len = nano_line_lengths[nano_cursor_y + 1];
        if (line_len + next_line_len < NANO_MAX_LINE_LENGTH) {
            for (int i = 0; i < next_line_len; i++) {
                nano_lines[nano_cursor_y][line_len + i] = nano_lines[nano_cursor_y + 1][i];
            }
            nano_line_lengths[nano_cursor_y] += next_line_len;
            nano_lines[nano_cursor_y][nano_line_lengths[nano_cursor_y]] = '\0';
            
            for (int i = nano_cursor_y + 1; i < nano_total_lines - 1; i++) {
                nano_line_lengths[i] = nano_line_lengths[i + 1];
                strcpy_impl(nano_lines[i], nano_lines[i + 1]);
            }
            nano_total_lines--;
            nano_dirty = 1;
        }
    }
}

void nano_move_to_line_start(void) {
    nano_cursor_x = 0;
}

void nano_move_to_line_end(void) {
    if (nano_cursor_y < nano_total_lines) {
        nano_cursor_x = nano_line_lengths[nano_cursor_y];
    }
}

void nano_page_scroll(int direction) {
    int lines_to_scroll = NANO_VIEWPORT_HEIGHT - 1;
    if (direction < 0) {
        nano_cursor_y -= lines_to_scroll;
        if (nano_cursor_y < 0) {
            nano_cursor_y = 0;
        }
    } else {
        nano_cursor_y += lines_to_scroll;
        if (nano_cursor_y >= nano_total_lines) {
            nano_cursor_y = nano_total_lines - 1;
        }
        if (nano_cursor_y < 0) {
            nano_cursor_y = 0;
        }
    }
    
    if (nano_cursor_x > nano_line_lengths[nano_cursor_y]) {
        nano_cursor_x = nano_line_lengths[nano_cursor_y];
    }
    
    nano_scroll_to_cursor();
}

void nano_cycle_theme(void) {
    current_theme = (current_theme + 1) % THEME_COUNT;
    nano_set_status_message("Theme changed.");
    nano_render_editor();
}

void nano_show_help_overlay(void) {
    nano_help_overlay_visible = 1;
    nano_render_help_overlay();
}

void nano_render_help_overlay(void) {
    uint8_t help_attr = VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
    
    const char *help_lines[] = {
        "                       NANO EDITOR HELP                        ",
        "",
        "  Navigation:                      Editing:",
        "    Arrow Keys - Move cursor         Backspace - Delete before cursor",
        "    Home       - Start of line       Delete    - Delete at cursor",
        "    End        - End of line         Enter     - New line",
        "    Page Up    - Scroll up",
        "    Page Down  - Scroll down",
        "",
        "  Commands:                        Theme:",
        "    Ctrl+S - Save file                Ctrl+T - Cycle theme",
        "    Ctrl+X - Exit (prompt if dirty)   Current: ",
        "    Ctrl+H - This help",
        "    Escape - Exit without saving",
        "",
        "                   Press any key to close help",
        0
    };
    
    int start_y = 3;
    int start_x = 5;
    
    for (int i = 0; help_lines[i] != 0; i++) {
        for (int j = 0; help_lines[i][j] != '\0' && j < VGA_WIDTH - start_x - 5; j++) {
            size_t pos = (start_y + i) * VGA_WIDTH + (start_x + j);
            VGA_BUFFER[pos * 2] = help_lines[i][j];
            VGA_BUFFER[pos * 2 + 1] = help_attr;
        }
    }
    
    size_t pos = (start_y + 11) * VGA_WIDTH + (start_x + 47);
    for (int j = 0; themes[current_theme].name[j] != '\0'; j++) {
        VGA_BUFFER[pos * 2] = themes[current_theme].name[j];
        VGA_BUFFER[pos * 2 + 1] = help_attr;
        pos++;
    }
}

void nano_set_status_message(const char *msg) {
    int i = 0;
    while (msg[i] != '\0' && i < VGA_WIDTH) {
        nano_status_message[i] = msg[i];
        i++;
    }
    nano_status_message[i] = '\0';
}

void nano_reset_status_message(void) {
    nano_status_message[0] = '\0';
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
    detect_boot_mode();
    vga_clear();
    console_print("Welcome to ");
    console_print(os_name);
    console_print(" ");
    console_print(os_version);
    console_print("\n");

    console_print("Boot mode: ");
    console_print(get_boot_mode_name());
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
