#include "../include/kernel/main.h"
#include "../include/kernel/bootlog.h"
#include "../include/kernel/log.h"
#include "../include/init/manager.h"
#include "../include/drivers/console.h"
#include "../include/drivers/keyboard.h"
#include "../include/shell/prompt.h"
#include "../include/shell/commands.h"
#include "../disk.h"
#include "../fat12.h"

extern uint32_t multiboot_magic_storage;
extern uint32_t multiboot_info_ptr_storage;

static int boot_mode = BOOT_MODE_UNKNOWN;
static init_manager_t init_manager;

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

static int service_console_start(service_descriptor_t *svc) {
    (void)svc;
    console_init(console_get_state());
    vga_clear();
    KLOG_INFO("console", "VGA console initialized");
    return 0;
}

static int service_keyboard_start(service_descriptor_t *svc) {
    (void)svc;
    keyboard_init(keyboard_get_state());
    KLOG_INFO("keyboard", "PS/2 keyboard driver initialized");
    return 0;
}

static int service_bootlog_start(service_descriptor_t *svc) {
    (void)svc;
    bootlog_init();
    KLOG_INFO("bootlog", "Boot diagnostics initialized");
    return 0;
}

static int service_disk_start(service_descriptor_t *svc) {
    (void)svc;
    int result = disk_init();
    if (result != 0) {
        KLOG_ERROR("disk", "Disk initialization failed");
        return result;
    }
    
    KLOG_INFO("disk", "Disk driver initialized");
    
    result = disk_self_test();
    if (result != 0) {
        KLOG_WARN("disk", "Disk self-test failed");
        return result;
    }
    
    KLOG_INFO("disk", "Disk self-test passed");
    return 0;
}

static int service_filesystem_start(service_descriptor_t *svc) {
    (void)svc;
    int result = fat12_init(0);
    if (result != FAT12_OK) {
        KLOG_ERROR("filesystem", "FAT12 initialization failed");
        return result;
    }
    
    commands_set_fat_ready(1);
    KLOG_INFO("filesystem", "FAT12 filesystem mounted");
    
    klog_set_filesystem_ready(1);
    
    return 0;
}

static int service_shell_start(service_descriptor_t *svc) {
    (void)svc;
    commands_init();
    KLOG_INFO("shell", "Shell initialized");
    return 0;
}

static void register_core_services(void) {
    const char *no_deps[] = { 0 };
    const char *console_deps[] = { "console", 0 };
    const char *keyboard_deps[] = { "console", 0 };
    const char *disk_deps[] = { "console", "bootlog", 0 };
    const char *fs_deps[] = { "disk", 0 };
    const char *shell_deps[] = { "console", "keyboard", "filesystem", 0 };
    
    init_manager_register_service(&init_manager, "console", service_console_start, 
                                  no_deps, 0, FAILURE_POLICY_HALT);
    
    init_manager_register_service(&init_manager, "keyboard", service_keyboard_start,
                                  keyboard_deps, 1, FAILURE_POLICY_WARN);
    
    init_manager_register_service(&init_manager, "bootlog", service_bootlog_start,
                                  console_deps, 1, FAILURE_POLICY_WARN);
    
    init_manager_register_service(&init_manager, "disk", service_disk_start,
                                  disk_deps, 2, FAILURE_POLICY_WARN);
    
    init_manager_register_service(&init_manager, "filesystem", service_filesystem_start,
                                  fs_deps, 1, FAILURE_POLICY_WARN);
    
    init_manager_register_service(&init_manager, "shell", service_shell_start,
                                  shell_deps, 3, FAILURE_POLICY_HALT);
}

void kernel_main(void) {
    detect_boot_mode();
    
    klog_init();
    KLOG_INFO("kernel", "AltoniumOS 1.0.0 starting");
    
    init_manager_init(&init_manager);
    register_core_services();
    
    KLOG_INFO("kernel", "Starting init system");
    int result = init_manager_start_all(&init_manager);
    
    console_print("Welcome to AltoniumOS 1.0.0\n");
    console_print("Boot mode: ");
    console_print(get_boot_mode_name());
    console_print("\n\n");
    
    if (result != 0) {
        console_print("Init system encountered errors\n\n");
        KLOG_ERROR("kernel", "Init system failed");
    }
    
    console_print("Mounted volume at ");
    console_print(fat12_get_cwd());
    console_print("\n");
    console_print("Type 'help' for available commands\n\n");
    
    KLOG_INFO("kernel", "Entering main loop");
    
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
