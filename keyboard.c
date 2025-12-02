#include "keyboard.h"
#include "pic.h"
#include "idt.h"
#include "console.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_start = 0;
static volatile int buffer_end = 0;
static bool shift_pressed = false;

static const char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

extern void keyboard_irq_handler(void);

__attribute__((naked)) void keyboard_irq_handler(void) {
    __asm__ volatile (
        "pusha\n"
        "call keyboard_handler\n"
        "popa\n"
        "iret\n"
    );
}

void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = false;
        }
    } else {
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = true;
        } else {
            char ascii = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
            if (ascii != 0) {
                int next = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
                if (next != buffer_start) {
                    keyboard_buffer[buffer_end] = ascii;
                    buffer_end = next;
                }
            }
        }
    }
    
    pic_send_eoi(1);
}

void keyboard_init(void) {
    buffer_start = 0;
    buffer_end = 0;
    shift_pressed = false;
    
    idt_set_gate(33, (uint32_t)keyboard_irq_handler, 0x08, 0x8E);
    pic_clear_mask(1);
}

bool keyboard_buffer_empty(void) {
    return buffer_start == buffer_end;
}

char keyboard_getchar(void) {
    while (keyboard_buffer_empty()) {
        __asm__ volatile ("hlt");
    }
    
    char c = keyboard_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
