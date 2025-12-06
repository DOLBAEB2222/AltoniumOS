#include "../include/kernel/main.h"
#include "../include/kernel/bootlog.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"
#include "../include/shell/prompt.h"
#include "../include/shell/commands.h"
#include "../disk.h"
#include "../fat12.h"

extern uint32_t multiboot_magic_storage;
extern uint32_t multiboot_info_ptr_storage;

static int boot_mode = BOOT_MODE_UNKNOWN;

void detect_boot_mode(void) {
    boot_mode = BOOT_MODE_BIOS;
    if (multiboot_magic_storage != MULTIBOOT_BOOTLOADER_MAGIC) {
        boot_mode = BOOT_MODE_UNKNOWN;
        return;
    }
    multiboot_info_t *info = (multiboot_info_t *)(uintptr_t)multiboot_info_ptr_storage;
    if (!info) {
        return;
    }
    if ((info->flags & (1u << 2)) != 0 && info->cmdline != 0) {
        const char *cmdline = (const char *)(uintptr_t)info->cmdline;
        if (string_contains(cmdline, "bootmode=uefi")) {
            boot_mode = BOOT_MODE_UEFI;
        }
    }
}

const char *get_boot_mode_name(void) {
    switch (boot_mode) {
        case BOOT_MODE_UEFI: return "UEFI";
        case BOOT_MODE_BIOS: return "BIOS";
        default: return "Unknown";
    }
}

void kernel_main(void) {
    detect_boot_mode();
    bootlog_init();
    vga_clear();
    console_print("Welcome to AltoniumOS 1.0.0\n");

    console_print("Boot mode: ");
    console_print(get_boot_mode_name());
    console_print("\n");
    
    console_print("Initializing disk driver... ");
    int disk_result = disk_init();
    if (disk_result != 0) {
        console_print("FAILED (error ");
        if (disk_result < 0) {
            console_print("-");
            disk_result = -disk_result;
        }
        char error_str[16];
        int pos = 0;
        if (disk_result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (disk_result > 0) {
                temp[temp_pos++] = '0' + (disk_result % 10);
                disk_result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
    } else {
        console_print("OK\n");
        
        console_print("Running disk self-test... ");
        int test_result = disk_self_test();
        if (test_result != 0) {
            console_print("FAILED (error ");
            if (test_result < 0) {
                console_print("-");
                test_result = -test_result;
            }
            char error_str[16];
            int pos = 0;
            if (test_result == 0) {
                error_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (test_result > 0) {
                    temp[temp_pos++] = '0' + (test_result % 10);
                    test_result /= 10;
                }
                for (int i = temp_pos - 1; i >= 0; i--) {
                    error_str[pos++] = temp[i];
                }
            }
            error_str[pos] = '\0';
            console_print(error_str);
            console_print(")\n");
        } else {
            console_print("OK\n");
        }
    }
    
    if (disk_result == 0) {
        console_print("Initializing FAT12 filesystem... ");
        int fat_result = fat12_init(0);
        if (fat_result != FAT12_OK) {
            console_print("FAILED (");
            console_print(fat12_error_string(fat_result));
            console_print(" code ");
            print_decimal(fat_result);
            console_print(")\n");
        } else {
            console_print("OK\n");
            commands_set_fat_ready(1);
            console_print("Mounted volume at ");
            console_print(fat12_get_cwd());
            console_print("\n");
        }
    }
    
    console_print("Type 'help' for available commands\n\n");
    
    while (1) {
        prompt_reset();
        render_prompt_line();
        
        while (1) {
            if (keyboard_ready()) {
                handle_keyboard_input();
                if (prompt_command_executed()) {
                    prompt_clear_executed_flag();
                    break;
                }
            }
        }
    }
}
