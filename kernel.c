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
void execute_command(const char *cmd_line);
void handle_keyboard_input(void);
void update_hardware_cursor(int x, int y);
void render_prompt_line(void);

/* External: defined in assembly */
extern void halt_cpu(void);

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
