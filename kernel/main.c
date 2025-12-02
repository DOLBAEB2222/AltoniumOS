#include <stdint.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info)
{
    (void)multiboot_magic;
    (void)multiboot_info;
    
    const char* message = "Kernel booted successfully!";
    uint8_t color = 0x0F;
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (color << 8) | ' ';
    }
    
    for (int i = 0; message[i] != '\0'; i++) {
        vga_buffer[i] = (color << 8) | message[i];
    }
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}
