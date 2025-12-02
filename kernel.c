#include "kernel/console.h"

void kernel_main(void) {
    // Initialize the console (displays "Welcome to AltoniumOS")
    console_init();
    
    // Demonstrate console features
    console_write("Testing console functionality...\n\n");
    
    // Test colors
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    console_write("Green text\n");
    
    console_set_color(VGA_COLOR_RED, VGA_COLOR_LIGHT_GREY);
    console_write("Red text on grey background\n");
    
    console_reset_color();
    console_write("Back to default colors\n\n");
    
    // Test formatted output
    console_printf("Integer tests: %d, %d, %d\n", -42, 0, 123);
    console_printf("Hex tests: 0x%x, 0x%x\n", 0xDEADBEEF, 0x12345678);
    console_printf("Unsigned tests: %u, %u\n", 0, 4294967295U);
    console_printf("String test: %s\n", "Hello from printf!");
    console_printf("Char test: %c%c%c\n", 'A', 'B', 'C');
    
    // Test cursor positioning
    console_write("\nCursor positioning test:\n");
    console_set_cursor(10, 10);
    console_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE);
    console_write("X");
    console_reset_color();
    console_set_cursor(20, 0);
    
    // Test special characters
    console_write("Special characters:\n");
    console_write("Tab:\tIndented text\n");
    console_write("Backspace: Hello\b\b\bHi\n");
    console_write("Carriage return: ABC\rXYZ\n");
    
    console_write("\nConsole initialization complete!\n");
    
    // Simple infinite loop
    while (1) {
        __asm__ volatile ("hlt");
    }
}