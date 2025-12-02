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

/* External: defined in assembly */
extern void halt_cpu(void);

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
}

/* Clear VGA buffer */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = VGA_ATTR_DEFAULT;
    }
    cursor_x = 0;
    cursor_y = 0;
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
    console_print("  clear     - Clear the screen\n");
    console_print("  echo TEXT - Print text to the screen\n");
    console_print("  fetch     - Print OS and system information\n");
    console_print("  shutdown  - Shut down the system\n");
    console_print("  help      - Display this help message\n");
}

void handle_shutdown(void) {
    console_print("Attempting system shutdown...\n");
    console_print("Halting CPU...\n");
    halt_cpu();
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
    console_print("Type 'help' for available commands\n\n");
    
    /* Simple command loop - read from simulated input */
    while (1) {
        console_print("> ");

        /* In a real implementation, we would read from keyboard/serial */
        /* For now, we'll demonstrate with pre-programmed commands */
        
        /* This loop would normally read keyboard input */
        while (1) {
            /* Simulate command input - in real implementation would read from kbd */
            /* For testing: auto-run help and fetch commands */
            break;
        }
    }
}
