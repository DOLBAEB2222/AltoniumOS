#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256

void keyboard_init(void);
void keyboard_handler(void);
bool keyboard_buffer_empty(void);
char keyboard_getchar(void);

#endif
