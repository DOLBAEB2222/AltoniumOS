#ifndef SHELL_PROMPT_H
#define SHELL_PROMPT_H

#include "../lib/string.h"

#define PROMPT_BUFFER_SIZE 256

typedef struct {
    char input_buffer[PROMPT_BUFFER_SIZE];
    int input_pos;
    int command_executed;
    int prompt_line_start_x;
    int prompt_line_y;
} shell_context_t;

void prompt_init(shell_context_t *ctx);
shell_context_t *prompt_get_context(void);
void render_prompt_line(void);
void prompt_reset(void);
void prompt_handle_scancode(uint16_t scancode);
int prompt_command_executed(void);
void prompt_clear_executed_flag(void);

#endif
