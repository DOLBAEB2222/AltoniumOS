#include "../include/lib/tui.h"
#include "../include/lib/string.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"

#define VGA_BUFFER ((volatile uint16_t *)0xB8000)
#define SCANCODE_UP 0x48
#define SCANCODE_DOWN 0x50
#define SCANCODE_ENTER 0x1C
#define SCANCODE_ESC 0x01
#define SCANCODE_Y 0x15
#define SCANCODE_N 0x31

static void draw_char_at(int x, int y, char c, uint8_t attr) {
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        VGA_BUFFER[y * 80 + x] = (uint16_t)c | ((uint16_t)attr << 8);
    }
}

static void draw_hline(int x, int y, int width, char c, uint8_t attr) {
    for (int i = 0; i < width; i++) {
        draw_char_at(x + i, y, c, attr);
    }
}

static void draw_vline(int x, int y, int height, char c, uint8_t attr) {
    for (int i = 0; i < height; i++) {
        draw_char_at(x, y + i, c, attr);
    }
}

void tui_draw_box(int x, int y, int width, int height, const char *title) {
    uint8_t attr = get_current_text_attr();
    
    draw_char_at(x, y, '+', attr);
    draw_char_at(x + width - 1, y, '+', attr);
    draw_char_at(x, y + height - 1, '+', attr);
    draw_char_at(x + width - 1, y + height - 1, '+', attr);
    
    draw_hline(x + 1, y, width - 2, '-', attr);
    draw_hline(x + 1, y + height - 1, width - 2, '-', attr);
    draw_vline(x, y + 1, height - 2, '|', attr);
    draw_vline(x + width - 1, y + 1, height - 2, '|', attr);
    
    if (title && title[0]) {
        int title_len = string_length(title);
        int title_x = x + (width - title_len - 2) / 2;
        if (title_x < x + 1) title_x = x + 1;
        draw_char_at(title_x, y, ' ', attr);
        for (int i = 0; i < title_len && title_x + 1 + i < x + width - 1; i++) {
            draw_char_at(title_x + 1 + i, y, title[i], attr);
        }
        draw_char_at(title_x + title_len + 1, y, ' ', attr);
    }
}

void tui_draw_window(const tui_window_t *window) {
    tui_draw_box(window->x, window->y, window->width, window->height, window->title);
}

void tui_draw_text(int x, int y, const char *text, uint8_t attr) {
    for (int i = 0; text[i] && x + i < 80; i++) {
        draw_char_at(x + i, y, text[i], attr);
    }
}

void tui_draw_centered_text(int y, const char *text, uint8_t attr) {
    int len = string_length(text);
    int x = (80 - len) / 2;
    if (x < 0) x = 0;
    tui_draw_text(x, y, text, attr);
}

void tui_clear_area(int x, int y, int width, int height) {
    uint8_t attr = get_current_text_attr();
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            draw_char_at(x + col, y + row, ' ', attr);
        }
    }
}

void tui_init_list(tui_list_t *list, int x, int y, int width, int height, const char *title) {
    list->window.x = x;
    list->window.y = y;
    list->window.width = width;
    list->window.height = height;
    string_copy(list->window.title, title);
    list->item_count = 0;
    list->selected_index = 0;
    list->scroll_offset = 0;
}

void tui_add_list_item(tui_list_t *list, const char *text, int enabled) {
    if (list->item_count >= TUI_MAX_ITEMS) return;
    string_copy(list->items[list->item_count].text, text);
    list->items[list->item_count].enabled = enabled;
    list->item_count++;
}

void tui_draw_list(const tui_list_t *list) {
    uint8_t normal_attr = get_current_text_attr();
    uint8_t selected_attr = get_current_status_attr();
    
    tui_draw_window(&list->window);
    
    int visible_rows = list->window.height - 2;
    for (int i = 0; i < visible_rows && i + list->scroll_offset < list->item_count; i++) {
        int item_idx = i + list->scroll_offset;
        int draw_y = list->window.y + 1 + i;
        uint8_t attr = (item_idx == list->selected_index) ? selected_attr : normal_attr;
        
        char prefix = (item_idx == list->selected_index) ? '>' : ' ';
        draw_char_at(list->window.x + 1, draw_y, prefix, attr);
        
        const char *text = list->items[item_idx].text;
        int max_text_width = list->window.width - 3;
        for (int j = 0; j < max_text_width; j++) {
            char c = (j < string_length(text)) ? text[j] : ' ';
            draw_char_at(list->window.x + 2 + j, draw_y, c, attr);
        }
    }
}

int tui_handle_list_input(tui_list_t *list, uint8_t scancode) {
    if (list->item_count == 0) return 0;
    
    if (scancode == SCANCODE_UP) {
        if (list->selected_index > 0) {
            list->selected_index--;
            if (list->selected_index < list->scroll_offset) {
                list->scroll_offset = list->selected_index;
            }
        }
        return 0;
    } else if (scancode == SCANCODE_DOWN) {
        if (list->selected_index < list->item_count - 1) {
            list->selected_index++;
            int visible_rows = list->window.height - 2;
            if (list->selected_index >= list->scroll_offset + visible_rows) {
                list->scroll_offset = list->selected_index - visible_rows + 1;
            }
        }
        return 0;
    } else if (scancode == SCANCODE_ENTER) {
        if (list->items[list->selected_index].enabled) {
            return 1;
        }
        return 0;
    } else if (scancode == SCANCODE_ESC) {
        return -1;
    }
    
    return 0;
}

int tui_get_selected_index(const tui_list_t *list) {
    return list->selected_index;
}

int tui_show_confirmation(const char *title, const char *message, const char *yes_text, const char *no_text) {
    int box_width = 50;
    int box_height = 8;
    int box_x = (80 - box_width) / 2;
    int box_y = (25 - box_height) / 2;
    
    tui_draw_box(box_x, box_y, box_width, box_height, title);
    
    uint8_t attr = get_current_text_attr();
    tui_draw_centered_text(box_y + 2, message, attr);
    
    char prompt[64];
    string_copy(prompt, yes_text);
    string_concat(prompt, " / ");
    string_concat(prompt, no_text);
    tui_draw_centered_text(box_y + 4, prompt, attr);
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            if (scancode == SCANCODE_Y) {
                return 1;
            } else if (scancode == SCANCODE_N || scancode == SCANCODE_ESC) {
                return 0;
            }
        }
    }
}

void tui_show_message(const char *title, const char *message) {
    int box_width = 50;
    int box_height = 6;
    int box_x = (80 - box_width) / 2;
    int box_y = (25 - box_height) / 2;
    
    tui_draw_box(box_x, box_y, box_width, box_height, title);
    
    uint8_t attr = get_current_text_attr();
    tui_draw_centered_text(box_y + 2, message, attr);
    tui_draw_centered_text(box_y + 4, "Press any key to continue...", attr);
    
    while (!keyboard_ready()) {}
    keyboard_get_scancode();
}

