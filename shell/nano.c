#include "../include/shell/nano.h"
#include "../include/shell/commands.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"
#include "../include/fs/vfs.h"

static nano_state_t nano_state = {0};

void nano_init_state(void) {
    nano_state.editor_active = 0;
    nano_state.total_lines = 0;
    nano_state.cursor_x = 0;
    nano_state.cursor_y = 0;
    nano_state.viewport_y = 0;
    nano_state.dirty = 0;
    nano_state.prompt_state = NANO_PROMPT_NONE;
    nano_state.help_overlay_visible = 0;
    nano_state.status_message[0] = '\0';
}

int nano_is_active(void) {
    return nano_state.editor_active;
}

void nano_init_editor(const char *filename) {
    strcpy_impl(nano_state.filename, filename);
    nano_state.total_lines = 0;
    nano_state.cursor_x = 0;
    nano_state.cursor_y = 0;
    nano_state.viewport_y = 0;
    nano_state.dirty = 0;
    nano_state.editor_active = 1;
    nano_state.prompt_state = NANO_PROMPT_NONE;
    nano_state.help_overlay_visible = 0;
    nano_reset_status_message();
    
    for (int i = 0; i < NANO_MAX_LINES; i++) {
        nano_state.line_lengths[i] = 0;
        nano_state.lines[i][0] = '\0';
    }
    
    uint32_t file_size = 0;
    uint8_t *fs_io_buffer = commands_get_io_buffer();
    int result = vfs_read_file(filename, fs_io_buffer, FS_IO_BUFFER_SIZE - 1, &file_size);
    
    if (result == VFS_OK && file_size > 0) {
        int line = 0;
        int line_pos = 0;
        
        for (uint32_t i = 0; i < file_size && line < NANO_MAX_LINES; i++) {
            char c = (char)fs_io_buffer[i];
            
            if (c == '\n') {
                nano_state.lines[line][line_pos] = '\0';
                nano_state.line_lengths[line] = line_pos;
                line++;
                line_pos = 0;
            } else if (c != '\r' && line_pos < NANO_MAX_LINE_LENGTH - 1) {
                nano_state.lines[line][line_pos++] = c;
            }
        }
        
        if (line_pos > 0 && line < NANO_MAX_LINES) {
            nano_state.lines[line][line_pos] = '\0';
            nano_state.line_lengths[line] = line_pos;
            line++;
        }
        
        nano_state.total_lines = line;
    } else {
        nano_state.total_lines = 1;
        nano_state.line_lengths[0] = 0;
        nano_state.lines[0][0] = '\0';
    }
    
    vga_clear();
    nano_render_editor();
}

void nano_render_editor(void) {
    uint8_t text_attr = get_current_text_attr();
    uint8_t status_attr = get_current_status_attr();
    
    char *VGA_BUFFER = (char *)0xB8000;
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i * 2] = ' ';
        VGA_BUFFER[i * 2 + 1] = text_attr;
    }
    
    for (int row = 0; row < NANO_VIEWPORT_HEIGHT && row < VGA_HEIGHT; row++) {
        int line_index = nano_state.viewport_y + row;
        if (line_index >= 0 && line_index < nano_state.total_lines) {
            for (int col = 0; col < nano_state.line_lengths[line_index] && col < VGA_WIDTH; col++) {
                size_t pos = row * VGA_WIDTH + col;
                VGA_BUFFER[pos * 2] = nano_state.lines[line_index][col];
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
        console_print_to_pos(status_line, status_x, nano_state.filename);
        status_x += strlen_impl(nano_state.filename);
        if (status_x < VGA_WIDTH) {
            console_print_to_pos(status_line, status_x, " | L:");
            status_x += 5;
        }
        char line_buf[16];
        int line_num = nano_state.cursor_y + 1;
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
        int col_num = nano_state.cursor_x + 1;
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
        if (nano_state.dirty) {
            console_print_to_pos(status_line, status_x, " | [MODIFIED]");
            status_x += 13;
        }
        console_print_to_pos(status_line, status_x, " | Theme: ");
        status_x += 10;
        char upper_theme[16];
        const theme_t *themes = console_get_themes();
        int theme_idx = 0;
        for (; themes[console_get_theme()].name[theme_idx] != '\0' && theme_idx < (int)sizeof(upper_theme) - 1; theme_idx++) {
            char c = themes[console_get_theme()].name[theme_idx];
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
        if (nano_state.status_message[0] != '\0') {
            console_print_to_pos(message_line, 0, nano_state.status_message);
        }
    }
    
    int screen_x = nano_state.cursor_x;
    int screen_y = nano_state.cursor_y - nano_state.viewport_y;
    if (screen_y >= 0 && screen_y < NANO_VIEWPORT_HEIGHT && screen_x < VGA_WIDTH) {
        update_hardware_cursor(screen_x, screen_y);
    }
}

void nano_handle_scancode(uint16_t scancode, int is_release) {
    if (nano_state.help_overlay_visible) {
        if (!is_release) {
            nano_state.help_overlay_visible = 0;
            nano_reset_status_message();
            nano_render_editor();
        }
        return;
    }
    
    if (nano_state.prompt_state == NANO_PROMPT_SAVE_CONFIRM) {
        if (is_release) {
            return;
        }
        if (scancode == 0x01) {
            nano_state.prompt_state = NANO_PROMPT_NONE;
            nano_exit_editor(NANO_EXIT_DISCARD);
            return;
        }
        char response = scancode_to_ascii(scancode);
        if (response == 'y') {
            nano_state.prompt_state = NANO_PROMPT_NONE;
            if (!nano_save_file()) {
                nano_set_status_message("Save failed.");
                nano_render_editor();
                return;
            }
            nano_exit_editor(NANO_EXIT_SAVE);
        } else if (response == 'n') {
            nano_state.prompt_state = NANO_PROMPT_NONE;
            nano_exit_editor(NANO_EXIT_DISCARD);
        }
        return;
    }
    
    if (is_release) {
        return;
    }
    
    if (keyboard_is_ctrl_pressed()) {
        switch (scancode) {
            case 0x1F:
                if (nano_save_file()) {
                    nano_set_status_message("Saved.");
                } else {
                    nano_set_status_message("Save failed.");
                }
                nano_render_editor();
                return;
            case 0x2D:
                if (nano_state.dirty) {
                    nano_state.prompt_state = NANO_PROMPT_SAVE_CONFIRM;
                    nano_set_status_message("Save changes? (Y=Yes, N=No, Esc=Discard)");
                    nano_render_editor();
                } else {
                    nano_exit_editor(NANO_EXIT_SAVE);
                }
                return;
            case 0x14:
                nano_cycle_theme();
                return;
            case 0x23:
                nano_show_help_overlay();
                return;
        }
    }
    
    switch (scancode) {
        case 0x01:
            nano_exit_editor(NANO_EXIT_DISCARD);
            return;
        case 0x0E:
            nano_handle_backspace();
            break;
        case 0x1C:
            nano_handle_enter();
            break;
        case 0xE053:
        case 0x53:
            nano_handle_delete();
            break;
        case 0xE048:
        case 0x48:
            nano_move_cursor(0, -1);
            break;
        case 0xE050:
        case 0x50:
            nano_move_cursor(0, 1);
            break;
        case 0xE04B:
        case 0x4B:
            nano_move_cursor(-1, 0);
            break;
        case 0xE04D:
        case 0x4D:
            nano_move_cursor(1, 0);
            break;
        case 0xE047:
        case 0x47:
            nano_move_to_line_start();
            break;
        case 0xE04F:
        case 0x4F:
            nano_move_to_line_end();
            break;
        case 0xE049:
        case 0x49:
            nano_page_scroll(-1);
            break;
        case 0xE051:
        case 0x51:
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
    if (nano_state.cursor_y >= NANO_MAX_LINES) {
        return;
    }
    
    if (nano_state.cursor_x >= nano_state.line_lengths[nano_state.cursor_y] && nano_state.cursor_x < NANO_MAX_LINE_LENGTH - 1) {
        nano_state.lines[nano_state.cursor_y][nano_state.cursor_x] = c;
        nano_state.lines[nano_state.cursor_y][nano_state.cursor_x + 1] = '\0';
        nano_state.line_lengths[nano_state.cursor_y]++;
        nano_state.cursor_x++;
    } else if (nano_state.cursor_x < nano_state.line_lengths[nano_state.cursor_y] && nano_state.cursor_x < NANO_MAX_LINE_LENGTH - 1) {
        for (int i = nano_state.line_lengths[nano_state.cursor_y]; i > nano_state.cursor_x; i--) {
            if (i < NANO_MAX_LINE_LENGTH - 1) {
                nano_state.lines[nano_state.cursor_y][i] = nano_state.lines[nano_state.cursor_y][i - 1];
            }
        }
        nano_state.lines[nano_state.cursor_y][nano_state.cursor_x] = c;
        if (nano_state.line_lengths[nano_state.cursor_y] < NANO_MAX_LINE_LENGTH - 1) {
            nano_state.line_lengths[nano_state.cursor_y]++;
        }
        nano_state.lines[nano_state.cursor_y][nano_state.line_lengths[nano_state.cursor_y]] = '\0';
        nano_state.cursor_x++;
    }
    
    nano_state.dirty = 1;
}

void nano_handle_backspace(void) {
    if (nano_state.cursor_y >= NANO_MAX_LINES) {
        return;
    }
    
    if (nano_state.cursor_x > 0) {
        for (int i = nano_state.cursor_x - 1; i < nano_state.line_lengths[nano_state.cursor_y]; i++) {
            nano_state.lines[nano_state.cursor_y][i] = nano_state.lines[nano_state.cursor_y][i + 1];
        }
        if (nano_state.line_lengths[nano_state.cursor_y] > 0) {
            nano_state.line_lengths[nano_state.cursor_y]--;
        }
        nano_state.lines[nano_state.cursor_y][nano_state.line_lengths[nano_state.cursor_y]] = '\0';
        nano_state.cursor_x--;
    } else if (nano_state.cursor_y > 0) {
        int prev_line_len = nano_state.line_lengths[nano_state.cursor_y - 1];
        int current_line_len = nano_state.line_lengths[nano_state.cursor_y];
        
        if (prev_line_len + current_line_len < NANO_MAX_LINE_LENGTH) {
            for (int i = 0; i < current_line_len; i++) {
                nano_state.lines[nano_state.cursor_y - 1][prev_line_len + i] = nano_state.lines[nano_state.cursor_y][i];
            }
            nano_state.line_lengths[nano_state.cursor_y - 1] += current_line_len;
            nano_state.lines[nano_state.cursor_y - 1][nano_state.line_lengths[nano_state.cursor_y - 1]] = '\0';
            
            for (int i = nano_state.cursor_y; i < nano_state.total_lines - 1; i++) {
                nano_state.line_lengths[i] = nano_state.line_lengths[i + 1];
                strcpy_impl(nano_state.lines[i], nano_state.lines[i + 1]);
            }
            nano_state.total_lines--;
            nano_state.cursor_y--;
            nano_state.cursor_x = prev_line_len;
        }
    }
    
    nano_state.dirty = 1;
}

void nano_handle_enter(void) {
    if (nano_state.total_lines >= NANO_MAX_LINES) {
        return;
    }
    
    int current_line_len = nano_state.line_lengths[nano_state.cursor_y];
    
    for (int i = nano_state.total_lines; i > nano_state.cursor_y + 1; i--) {
        nano_state.line_lengths[i] = nano_state.line_lengths[i - 1];
        strcpy_impl(nano_state.lines[i], nano_state.lines[i - 1]);
    }
    
    int new_line_len = 0;
    for (int i = nano_state.cursor_x; i < current_line_len; i++) {
        nano_state.lines[nano_state.cursor_y + 1][new_line_len++] = nano_state.lines[nano_state.cursor_y][i];
    }
    nano_state.lines[nano_state.cursor_y + 1][new_line_len] = '\0';
    nano_state.line_lengths[nano_state.cursor_y + 1] = new_line_len;
    
    nano_state.lines[nano_state.cursor_y][nano_state.cursor_x] = '\0';
    nano_state.line_lengths[nano_state.cursor_y] = nano_state.cursor_x;
    
    nano_state.total_lines++;
    nano_state.cursor_y++;
    nano_state.cursor_x = 0;
    nano_state.dirty = 1;
}

void nano_move_cursor(int dx, int dy) {
    int new_x = nano_state.cursor_x + dx;
    int new_y = nano_state.cursor_y + dy;
    
    if (new_y < 0) new_y = 0;
    if (new_y >= nano_state.total_lines) new_y = nano_state.total_lines - 1;
    
    if (new_x < 0) new_x = 0;
    if (new_x > nano_state.line_lengths[new_y]) new_x = nano_state.line_lengths[new_y];
    
    nano_state.cursor_x = new_x;
    nano_state.cursor_y = new_y;
    
    nano_scroll_to_cursor();
}

void nano_scroll_to_cursor(void) {
    if (nano_state.cursor_y < nano_state.viewport_y) {
        nano_state.viewport_y = nano_state.cursor_y;
    } else if (nano_state.cursor_y >= nano_state.viewport_y + NANO_VIEWPORT_HEIGHT) {
        nano_state.viewport_y = nano_state.cursor_y - NANO_VIEWPORT_HEIGHT + 1;
    }
    
    if (nano_state.viewport_y < 0) nano_state.viewport_y = 0;
    if (nano_state.viewport_y > nano_state.total_lines - NANO_VIEWPORT_HEIGHT) {
        nano_state.viewport_y = nano_state.total_lines - NANO_VIEWPORT_HEIGHT;
    }
    if (nano_state.viewport_y < 0) nano_state.viewport_y = 0;
}

int nano_save_file(void) {
    if (!nano_state.dirty) {
        return 1;
    }
    
    uint8_t *fs_io_buffer = commands_get_io_buffer();
    uint32_t file_size = 0;
    for (int i = 0; i < nano_state.total_lines && file_size < FS_IO_BUFFER_SIZE - 1; i++) {
        for (int j = 0; j < nano_state.line_lengths[i] && file_size < FS_IO_BUFFER_SIZE - 2; j++) {
            fs_io_buffer[file_size++] = (uint8_t)nano_state.lines[i][j];
        }
        if (file_size < FS_IO_BUFFER_SIZE - 1) {
            fs_io_buffer[file_size++] = '\n';
        }
    }
    
    int result = vfs_write_file(nano_state.filename, fs_io_buffer, file_size);
    if (result == VFS_OK) {
        nano_state.dirty = 0;
        return 1;
    }
    return 0;
}

void nano_exit_editor(int save_action) {
    int had_dirty = nano_state.dirty;
    if (save_action == NANO_EXIT_SAVE && nano_state.dirty) {
        if (!nano_save_file()) {
            nano_set_status_message("Save failed.");
            nano_render_editor();
            return;
        }
        had_dirty = 1;
    } else if (save_action == NANO_EXIT_DISCARD) {
        nano_state.dirty = 0;
    }
    
    nano_state.editor_active = 0;
    nano_state.prompt_state = NANO_PROMPT_NONE;
    nano_state.help_overlay_visible = 0;
    
    vga_clear();
    console_print("Welcome to AltoniumOS 1.0.0\n\n");
    
    if (save_action == NANO_EXIT_SAVE && had_dirty) {
        console_print("File saved: ");
        console_print(nano_state.filename);
        console_print("\n");
    } else if (save_action == NANO_EXIT_DISCARD && had_dirty) {
        console_print("Changes discarded: ");
        console_print(nano_state.filename);
        console_print("\n");
    }
    
    if (commands_is_fs_ready()) {
        console_print("Current directory: ");
        console_print(fat12_get_cwd());
        console_print("\n");
    }
}

void nano_handle_delete(void) {
    if (nano_state.cursor_y >= NANO_MAX_LINES || nano_state.cursor_y >= nano_state.total_lines) {
        return;
    }
    
    int line_len = nano_state.line_lengths[nano_state.cursor_y];
    if (nano_state.cursor_x < line_len) {
        for (int i = nano_state.cursor_x; i < line_len - 1; i++) {
            nano_state.lines[nano_state.cursor_y][i] = nano_state.lines[nano_state.cursor_y][i + 1];
        }
        nano_state.line_lengths[nano_state.cursor_y]--;
        nano_state.lines[nano_state.cursor_y][nano_state.line_lengths[nano_state.cursor_y]] = '\0';
        nano_state.dirty = 1;
    } else if (nano_state.cursor_y < nano_state.total_lines - 1) {
        int next_line_len = nano_state.line_lengths[nano_state.cursor_y + 1];
        if (line_len + next_line_len < NANO_MAX_LINE_LENGTH) {
            for (int i = 0; i < next_line_len; i++) {
                nano_state.lines[nano_state.cursor_y][line_len + i] = nano_state.lines[nano_state.cursor_y + 1][i];
            }
            nano_state.line_lengths[nano_state.cursor_y] += next_line_len;
            nano_state.lines[nano_state.cursor_y][nano_state.line_lengths[nano_state.cursor_y]] = '\0';
            
            for (int i = nano_state.cursor_y + 1; i < nano_state.total_lines - 1; i++) {
                nano_state.line_lengths[i] = nano_state.line_lengths[i + 1];
                strcpy_impl(nano_state.lines[i], nano_state.lines[i + 1]);
            }
            nano_state.total_lines--;
            nano_state.dirty = 1;
        }
    }
}

void nano_move_to_line_start(void) {
    nano_state.cursor_x = 0;
}

void nano_move_to_line_end(void) {
    if (nano_state.cursor_y < nano_state.total_lines) {
        nano_state.cursor_x = nano_state.line_lengths[nano_state.cursor_y];
    }
}

void nano_page_scroll(int direction) {
    int lines_to_scroll = NANO_VIEWPORT_HEIGHT - 1;
    if (direction < 0) {
        nano_state.cursor_y -= lines_to_scroll;
        if (nano_state.cursor_y < 0) {
            nano_state.cursor_y = 0;
        }
    } else {
        nano_state.cursor_y += lines_to_scroll;
        if (nano_state.cursor_y >= nano_state.total_lines) {
            nano_state.cursor_y = nano_state.total_lines - 1;
        }
        if (nano_state.cursor_y < 0) {
            nano_state.cursor_y = 0;
        }
    }
    
    if (nano_state.cursor_x > nano_state.line_lengths[nano_state.cursor_y]) {
        nano_state.cursor_x = nano_state.line_lengths[nano_state.cursor_y];
    }
    
    nano_scroll_to_cursor();
}

void nano_cycle_theme(void) {
    int current = console_get_theme();
    console_set_theme((current + 1) % THEME_COUNT);
    nano_set_status_message("Theme changed.");
    nano_render_editor();
}

void nano_show_help_overlay(void) {
    nano_state.help_overlay_visible = 1;
    nano_render_help_overlay();
}

void nano_render_help_overlay(void) {
    uint8_t help_attr = VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
    
    char *VGA_BUFFER = (char *)0xB8000;
    
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
    
    const theme_t *themes = console_get_themes();
    size_t pos = (start_y + 11) * VGA_WIDTH + (start_x + 47);
    for (int j = 0; themes[console_get_theme()].name[j] != '\0'; j++) {
        VGA_BUFFER[pos * 2] = themes[console_get_theme()].name[j];
        VGA_BUFFER[pos * 2 + 1] = help_attr;
        pos++;
    }
}

void nano_set_status_message(const char *msg) {
    int i = 0;
    while (msg[i] != '\0' && i < VGA_WIDTH) {
        nano_state.status_message[i] = msg[i];
        i++;
    }
    nano_state.status_message[i] = '\0';
}

void nano_reset_status_message(void) {
    nano_state.status_message[0] = '\0';
}
