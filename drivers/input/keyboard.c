#include "../../include/drivers/keyboard.h"
#include "../../include/shell/prompt.h"
#include "../../include/shell/nano.h"
#include "../../include/kernel/hw_detect.h"
#include "../../include/drivers/console.h"

static keyboard_state_t global_keyboard_state = {0, 0};

void keyboard_init(keyboard_state_t *state) {
    /* Check if PS/2 controller is available */
    if (!hw_has_ps2_controller()) {
        console_print("Warning: No PS/2 controller detected, keyboard may not work\n");
    }
    
    if (state) {
        state->ctrl_pressed = 0;
        state->extended_scancode_pending = 0;
    }
}

keyboard_state_t *keyboard_get_state(void) {
    return &global_keyboard_state;
}

int keyboard_ready(void) {
    return (inb(0x64) & 1) != 0;
}

uint8_t read_keyboard(void) {
    while (!keyboard_ready()) {
    }
    return inb(0x60);
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

void handle_keyboard_input(void) {
    uint8_t raw_scancode = read_keyboard();
    
    if (raw_scancode == 0xE0) {
        global_keyboard_state.extended_scancode_pending = 1;
        return;
    }
    
    int is_release = (raw_scancode & 0x80) ? 1 : 0;
    uint8_t base_code = raw_scancode & 0x7F;
    int extended = global_keyboard_state.extended_scancode_pending;
    global_keyboard_state.extended_scancode_pending = 0;
    uint16_t scancode = extended ? (0xE000 | base_code) : base_code;
    
    if (base_code == 0x1D) {
        global_keyboard_state.ctrl_pressed = is_release ? 0 : 1;
        return;
    }
    
    if (nano_is_active()) {
        nano_handle_scancode(scancode, is_release);
        return;
    }
    
    if (is_release) {
        return;
    }
    
    handle_console_scancode(scancode);
}

void handle_console_scancode(uint16_t scancode) {
    prompt_handle_scancode(scancode);
}

int keyboard_is_ctrl_pressed(void) {
    return global_keyboard_state.ctrl_pressed;
}
