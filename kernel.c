#include "console.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "shell.h"

void kernel_main(void) {
    console_init();
    
    __asm__ volatile ("cli");
    
    idt_init();
    pic_init();
    keyboard_init();
    
    __asm__ volatile ("sti");
    
    shell_init();
    shell_run();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}
