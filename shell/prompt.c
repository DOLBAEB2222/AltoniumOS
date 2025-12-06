#include "../include/shell/prompt.h"
#include "../include/shell/commands.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"

static shell_context_t global_prompt_ctx = {{0}, 0, 0, 0, 0};

void prompt_init(shell_context_t *ctx) {
    if (ctx) {
        ctx->input_pos = 0;
        ctx->command_executed = 0;
        ctx->prompt_line_start_x = console_get_cursor_x();
        ctx->prompt_line_y = console_get_cursor_y();
    }
}

shell_context_t *prompt_get_context(void) {
    return &global_prompt_ctx;
}

void render_prompt_line(void) {
    int line_y = global_prompt_ctx.prompt_line_y;
    uint8_t text_attr = get_current_text_attr();
    
    char *VGA_BUFFER = (char *)0xB8000;
    
    for (int i = global_prompt_ctx.prompt_line_start_x; i < VGA_WIDTH; i++) {
        size_t pos = line_y * VGA_WIDTH + i;
        VGA_BUFFER[pos * 2] = ' ';
        VGA_BUFFER[pos * 2 + 1] = text_attr;
    }
    
    for (int i = 0; i < global_prompt_ctx.input_pos; i++) {
        size_t pos = line_y * VGA_WIDTH + (global_prompt_ctx.prompt_line_start_x + i);
        if (global_prompt_ctx.prompt_line_start_x + i < VGA_WIDTH) {
            VGA_BUFFER[pos * 2] = global_prompt_ctx.input_buffer[i];
            VGA_BUFFER[pos * 2 + 1] = text_attr;
        }
    }
    
    if (global_prompt_ctx.prompt_line_start_x + global_prompt_ctx.input_pos < VGA_WIDTH) {
        size_t pos = line_y * VGA_WIDTH + (global_prompt_ctx.prompt_line_start_x + global_prompt_ctx.input_pos);
        VGA_BUFFER[pos * 2] = '>';
        VGA_BUFFER[pos * 2 + 1] = text_attr;
    }
    
    int cursor_x = global_prompt_ctx.prompt_line_start_x + global_prompt_ctx.input_pos;
    int cursor_y = line_y;
    console_set_cursor(cursor_x, cursor_y);
}

void prompt_reset(void) {
    global_prompt_ctx.input_pos = 0;
    global_prompt_ctx.command_executed = 0;
    global_prompt_ctx.prompt_line_start_x = console_get_cursor_x();
    global_prompt_ctx.prompt_line_y = console_get_cursor_y();
}

void prompt_handle_scancode(uint16_t scancode) {
    char c = scancode_to_ascii(scancode);
    
    if (c == '\n') {
        console_putchar('\n');
        global_prompt_ctx.input_buffer[global_prompt_ctx.input_pos] = '\0';
        execute_command(global_prompt_ctx.input_buffer);
        global_prompt_ctx.input_pos = 0;
        global_prompt_ctx.command_executed = 1;
    } else if (c == '\b') {
        if (global_prompt_ctx.input_pos > 0) {
            global_prompt_ctx.input_pos--;
            render_prompt_line();
        }
    } else if (c >= ' ' && c <= '~') {
        if (global_prompt_ctx.input_pos < PROMPT_BUFFER_SIZE - 1) {
            global_prompt_ctx.input_buffer[global_prompt_ctx.input_pos++] = c;
            render_prompt_line();
        }
    }
}

int prompt_command_executed(void) {
    return global_prompt_ctx.command_executed;
}

void prompt_clear_executed_flag(void) {
    global_prompt_ctx.command_executed = 0;
}
