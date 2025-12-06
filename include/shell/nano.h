#ifndef SHELL_NANO_H
#define SHELL_NANO_H

#include "../lib/string.h"
#include "../../fat12.h"

#define NANO_MAX_LINES 1000
#define NANO_MAX_LINE_LENGTH 200
#define NANO_VIEWPORT_HEIGHT 23

#define NANO_EXIT_SAVE      1
#define NANO_EXIT_DISCARD   0

#define NANO_PROMPT_NONE         0
#define NANO_PROMPT_SAVE_CONFIRM 1

typedef struct {
    int editor_active;
    char filename[FAT12_PATH_MAX];
    char lines[NANO_MAX_LINES][NANO_MAX_LINE_LENGTH];
    int line_lengths[NANO_MAX_LINES];
    int total_lines;
    int cursor_x;
    int cursor_y;
    int viewport_y;
    int dirty;
    int prompt_state;
    char status_message[81];
    int help_overlay_visible;
} nano_state_t;

void nano_init_state(void);
int nano_is_active(void);
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

#endif
