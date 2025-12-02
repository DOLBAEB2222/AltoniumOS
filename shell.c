#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "types.h"

#define CMD_BUFFER_SIZE 256
#define MAX_TOKENS 16

static char cmd_buffer[CMD_BUFFER_SIZE];
static char* tokens[MAX_TOKENS];

static int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static int tokenize(char* str, char** tokens, int max_tokens) {
    int count = 0;
    char* current = str;
    bool in_token = false;
    
    while (*current != '\0' && count < max_tokens) {
        if (*current == ' ' || *current == '\t') {
            if (in_token) {
                *current = '\0';
                in_token = false;
            }
        } else {
            if (!in_token) {
                tokens[count++] = current;
                in_token = true;
            }
        }
        current++;
    }
    
    return count;
}

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_write("Available commands:\n");
    console_write("  help   - Show this help message\n");
    console_write("  clear  - Clear the screen\n");
    console_write("  echo   - Echo arguments back\n");
    console_write("  hello  - Print a greeting\n");
}

static void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_clear();
}

static void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        console_write(argv[i]);
        if (i < argc - 1) {
            console_write(" ");
        }
    }
    console_write("\n");
}

static void cmd_hello(int argc, char** argv) {
    (void)argc;
    (void)argv;
    console_write("Hello from the simple shell!\n");
}

static void execute_command(int argc, char** argv) {
    if (argc == 0) {
        return;
    }
    
    if (string_compare(argv[0], "help") == 0) {
        cmd_help(argc, argv);
    } else if (string_compare(argv[0], "clear") == 0) {
        cmd_clear(argc, argv);
    } else if (string_compare(argv[0], "echo") == 0) {
        cmd_echo(argc, argv);
    } else if (string_compare(argv[0], "hello") == 0) {
        cmd_hello(argc, argv);
    } else {
        console_write("Unknown command: ");
        console_write(argv[0]);
        console_write("\n");
        console_write("Type 'help' for available commands.\n");
    }
}

static void read_command(char* buffer, int max_len) {
    int pos = 0;
    
    while (1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            console_write("\n");
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                console_putchar('\b');
            }
        } else if (pos < max_len - 1) {
            buffer[pos++] = c;
            console_putchar(c);
        }
    }
}

void shell_init(void) {
    console_write("Simple Batch Shell\n");
    console_write("Type 'help' for available commands.\n\n");
}

void shell_run(void) {
    while (1) {
        console_write("> ");
        
        read_command(cmd_buffer, CMD_BUFFER_SIZE);
        
        int token_count = tokenize(cmd_buffer, tokens, MAX_TOKENS);
        
        if (token_count > 0) {
            execute_command(token_count, tokens);
        }
    }
}
