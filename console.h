#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

void console_init(void);
void console_putchar(char c);
void console_write(const char* str);
void console_clear(void);

#endif
