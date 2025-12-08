#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include "../lib/string.h"

typedef struct {
    int ctrl_pressed;
    int extended_scancode_pending;
} keyboard_state_t;

void keyboard_init(keyboard_state_t *state);
keyboard_state_t *keyboard_get_state(void);
int keyboard_ready(void);
uint8_t read_keyboard(void);
uint8_t keyboard_get_scancode(void);
char scancode_to_ascii(uint16_t scancode);
void handle_keyboard_input(void);
void handle_console_scancode(uint16_t scancode);
int keyboard_is_ctrl_pressed(void);

#endif
