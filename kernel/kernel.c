#include <stdint.h>

// Kernel main entry point
// eax = magic number (0x2BADB002 for multiboot)
// ebx = address of multiboot information structure
void kernel_main(uint32_t magic, uint32_t mboot_ptr) {
    // Simple kernel stub - just halt
    while (1) {
        asm("hlt");
    }
}
