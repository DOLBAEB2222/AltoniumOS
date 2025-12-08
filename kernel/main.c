#include "../include/kernel/main.h"
#include "../include/kernel/bootlog.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"
#include "../include/drivers/storage/block_device.h"
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
    
    console_print("Initializing storage manager... ");
    int storage_devices = storage_manager_init();
    console_print("OK (");
    print_decimal(storage_devices);
    console_print(" device(s) detected)\n");
    
    if (boot_mode == BOOT_MODE_BIOS && bootlog_data->boot_method == 2) {
        console_print("\nFATAL: Disk read error during boot (status 0x");
        char hex[3];
        hex[0] = "0123456789ABCDEF"[bootlog_data->int13_status >> 4];
        hex[1] = "0123456789ABCDEF"[bootlog_data->int13_status & 0x0F];
        hex[2] = '\0';
        console_print(hex);
        console_print(")\n");
        console_print("Cannot continue. Check BIOS settings or use Safe BIOS mode.\n");
        console_print("Press Ctrl+Alt+Del to restart.\n");
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    
    if (boot_mode == BOOT_MODE_BIOS && bootlog_data->memory_mb > 0) {
        console_print("Detected memory: ");
        unsigned int mem = bootlog_data->memory_mb;
        char buf[16];
        int i = 0;
        if (mem == 0) {
            buf[i++] = '0';
        } else {
            char temp[16];
            int ti = 0;
            while (mem > 0) {
                temp[ti++] = '0' + (mem % 10);
                mem /= 10;
            }
            for (int j = ti - 1; j >= 0; j--) {
                buf[i++] = temp[j];
            }
        }
        buf[i] = '\0';
        console_print(buf);
        console_print(" MB\n");
    }
    
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
