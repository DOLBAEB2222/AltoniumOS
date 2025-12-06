#ifndef KERNEL_MAIN_H
#define KERNEL_MAIN_H

#include "../lib/string.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

enum {
    BOOT_MODE_UNKNOWN = 0,
    BOOT_MODE_BIOS = 1,
    BOOT_MODE_UEFI = 2
};

void kernel_main(void);
void detect_boot_mode(void);
const char *get_boot_mode_name(void);
int get_current_boot_mode(void);

#endif
