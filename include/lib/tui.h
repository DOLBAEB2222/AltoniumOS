#ifndef LIB_TUI_H
#define LIB_TUI_H

#include "../lib/string.h"

#define TUI_MAX_ITEMS 32
#define TUI_MAX_TITLE_LEN 64
#define TUI_MAX_TEXT_LEN 128

typedef struct {
    char text[TUI_MAX_TEXT_LEN];
    int enabled;
} tui_list_item_t;

typedef struct {
    char title[TUI_MAX_TITLE_LEN];
    int x;
    int y;
    int width;
    int height;
} tui_window_t;

typedef struct {
    tui_window_t window;
    tui_list_item_t items[TUI_MAX_ITEMS];
    int item_count;
    int selected_index;
    int scroll_offset;
} tui_list_t;

void tui_draw_window(const tui_window_t *window);
void tui_draw_box(int x, int y, int width, int height, const char *title);
void tui_draw_text(int x, int y, const char *text, uint8_t attr);
void tui_draw_centered_text(int y, const char *text, uint8_t attr);
int tui_show_confirmation(const char *title, const char *message, const char *yes_text, const char *no_text);
void tui_init_list(tui_list_t *list, int x, int y, int width, int height, const char *title);
void tui_add_list_item(tui_list_t *list, const char *text, int enabled);
void tui_draw_list(const tui_list_t *list);
int tui_handle_list_input(tui_list_t *list, uint8_t scancode);
int tui_get_selected_index(const tui_list_t *list);
void tui_clear_area(int x, int y, int width, int height);
void tui_show_message(const char *title, const char *message);

#endif
