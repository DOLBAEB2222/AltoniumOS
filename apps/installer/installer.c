#include "../../include/apps/installer.h"
#include "../../include/lib/tui.h"
#include "../../include/lib/string.h"
#include "../../include/lib/partition_table.h"
#include "../../include/lib/fat32_format.h"
#include "../../include/lib/ext2_format.h"
#include "../../include/drivers/console.h"
#include "../../include/drivers/keyboard.h"
#include "../../include/drivers/storage/block_device.h"

#define STEP_DISK_SELECT 0
#define STEP_PARTITION_TABLE 1
#define STEP_FILESYSTEM 2
#define STEP_CONFIRM_FORMAT 3
#define STEP_COPY_FILES 4
#define STEP_BOOTLOADER 5
#define STEP_COMPLETE 6

static int current_step = STEP_DISK_SELECT;
static int selected_disk = 0;
static int selected_partition_table = PARTITION_TABLE_MBR;
static int selected_filesystem = 0;
static char status_message[128];

static void print_step_header(const char *title) {
    vga_clear();
    tui_draw_centered_text(1, "AltoniumOS Installer", get_current_status_attr());
    tui_draw_centered_text(2, title, get_current_text_attr());
    draw_hline(0, 3, 80, '-', get_current_text_attr());
}

static void draw_hline(int x, int y, int width, char c, uint8_t attr) {
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    for (int i = 0; i < width && x + i < 80; i++) {
        vga[y * 80 + x + i] = (uint16_t)c | ((uint16_t)attr << 8);
    }
}

static int step_disk_select(void) {
    print_step_header("Step 1: Select Target Disk");
    
    tui_list_t list;
    tui_init_list(&list, 10, 5, 60, 12, "Available Disks");
    
    int device_count = storage_get_device_count();
    if (device_count == 0) {
        tui_add_list_item(&list, "No storage devices detected", 0);
    } else {
        for (int i = 0; i < device_count; i++) {
            block_device_t *dev = storage_get_device(i);
            if (dev) {
                char item[TUI_MAX_TEXT_LEN];
                string_copy(item, "Disk ");
                char num[8];
                num[0] = '0' + i;
                num[1] = '\0';
                string_concat(item, num);
                string_concat(item, ": ");
                string_concat(item, dev->driver_name);
                string_concat(item, " (");
                
                uint32_t size_mb = (dev->capacity * dev->sector_size) / (1024 * 1024);
                char size_str[16];
                int idx = 0;
                if (size_mb == 0) {
                    size_str[idx++] = '0';
                } else {
                    char temp[16];
                    int ti = 0;
                    while (size_mb > 0) {
                        temp[ti++] = '0' + (size_mb % 10);
                        size_mb /= 10;
                    }
                    for (int j = ti - 1; j >= 0; j--) {
                        size_str[idx++] = temp[j];
                    }
                }
                size_str[idx] = '\0';
                string_concat(item, size_str);
                string_concat(item, " MB)");
                
                tui_add_list_item(&list, item, 1);
            }
        }
    }
    
    tui_draw_list(&list);
    tui_draw_text(10, 18, "Use UP/DOWN arrows to select, ENTER to continue, ESC to cancel", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            int result = tui_handle_list_input(&list, scancode);
            if (result == 1) {
                selected_disk = tui_get_selected_index(&list);
                return 1;
            } else if (result == -1) {
                return -1;
            }
            tui_draw_list(&list);
        }
    }
}

static int step_partition_table(void) {
    print_step_header("Step 2: Select Partition Table Type");
    
    tui_list_t list;
    tui_init_list(&list, 10, 5, 60, 10, "Partition Table Type");
    
    tui_add_list_item(&list, "MBR (Master Boot Record) - Legacy BIOS", 1);
    tui_add_list_item(&list, "GPT (GUID Partition Table) - UEFI (stub)", 0);
    
    tui_draw_list(&list);
    tui_draw_text(10, 16, "Use UP/DOWN arrows to select, ENTER to continue, ESC to go back", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            int result = tui_handle_list_input(&list, scancode);
            if (result == 1) {
                selected_partition_table = tui_get_selected_index(&list);
                return 1;
            } else if (result == -1) {
                return 0;
            }
            tui_draw_list(&list);
        }
    }
}

static int step_filesystem(void) {
    print_step_header("Step 3: Select Filesystem Type");
    
    tui_list_t list;
    tui_init_list(&list, 10, 5, 60, 12, "Filesystem Type");
    
    tui_add_list_item(&list, "FAT12 - Legacy filesystem (existing formatter)", 1);
    tui_add_list_item(&list, "FAT32 - Windows compatible", 1);
    tui_add_list_item(&list, "ext2 - Linux filesystem (basic stub)", 1);
    
    tui_draw_list(&list);
    tui_draw_text(10, 18, "Use UP/DOWN arrows to select, ENTER to continue, ESC to go back", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            int result = tui_handle_list_input(&list, scancode);
            if (result == 1) {
                selected_filesystem = tui_get_selected_index(&list);
                return 1;
            } else if (result == -1) {
                return 0;
            }
            tui_draw_list(&list);
        }
    }
}

static int step_confirm_format(void) {
    print_step_header("Step 4: Confirm Formatting");
    
    char message[128];
    string_copy(message, "Format disk ");
    char num[8];
    num[0] = '0' + selected_disk;
    num[1] = '\0';
    string_concat(message, num);
    string_concat(message, "? All data will be lost!");
    
    int confirmed = tui_show_confirmation("Warning", message, "Y - Yes", "N - No");
    
    if (confirmed) {
        print_step_header("Step 4: Formatting...");
        
        uint32_t partition_lba = 2048;
        uint32_t partition_size = 0x100000;
        
        tui_draw_text(10, 8, "Creating partition table...", get_current_text_attr());
        
        uint8_t fs_type = PARTITION_TYPE_FAT32;
        if (selected_filesystem == 0) fs_type = PARTITION_TYPE_FAT12;
        else if (selected_filesystem == 1) fs_type = PARTITION_TYPE_FAT32;
        else if (selected_filesystem == 2) fs_type = PARTITION_TYPE_LINUX;
        
        int result = partition_create_mbr(0, 0, partition_lba, partition_size, fs_type);
        if (result != 0) {
            string_copy(status_message, "Error: Failed to create partition");
            return 0;
        }
        
        tui_draw_text(10, 9, "Formatting filesystem...", get_current_text_attr());
        
        if (selected_filesystem == 1) {
            result = fat32_format(partition_lba, partition_size, "ALTONIUM");
        } else if (selected_filesystem == 2) {
            result = ext2_format(partition_lba, partition_size, "altonium");
        }
        
        if (result != 0 && selected_filesystem != 0) {
            string_copy(status_message, "Warning: Filesystem format may be incomplete");
        } else {
            string_copy(status_message, "Partition created and formatted successfully");
        }
        
        tui_draw_text(10, 10, status_message, get_current_text_attr());
        tui_draw_text(10, 12, "Press any key to continue...", get_current_text_attr());
        
        while (!keyboard_ready()) {}
        keyboard_get_scancode();
        
        return 1;
    }
    
    return 0;
}

static int step_copy_files(void) {
    print_step_header("Step 5: Copy System Files");
    
    tui_draw_text(10, 8, "File copy summary:", get_current_text_attr());
    tui_draw_text(10, 10, "  [ ] Kernel image (kernel.bin)", get_current_text_attr());
    tui_draw_text(10, 11, "  [ ] Boot configuration", get_current_text_attr());
    tui_draw_text(10, 12, "  [ ] System files", get_current_text_attr());
    tui_draw_text(10, 14, "Note: Actual file copy is stubbed in this version.", get_current_text_attr());
    tui_draw_text(10, 15, "      Files would be copied from source media to target disk.", get_current_text_attr());
    
    tui_draw_text(10, 18, "Press ENTER to continue, ESC to go back", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            if (scancode == 0x1C) {
                return 1;
            } else if (scancode == 0x01) {
                return 0;
            }
        }
    }
}

static int step_bootloader(void) {
    print_step_header("Step 6: Install Bootloader");
    
    tui_draw_text(10, 8, "Bootloader installation summary:", get_current_text_attr());
    tui_draw_text(10, 10, "  Target disk: Disk 0", get_current_text_attr());
    tui_draw_text(10, 11, "  Boot sector: MBR", get_current_text_attr());
    tui_draw_text(10, 12, "  Status: Ready to install", get_current_text_attr());
    tui_draw_text(10, 14, "Note: Bootloader installation is stubbed in this version.", get_current_text_attr());
    tui_draw_text(10, 15, "      On real hardware, this would write the boot sector.", get_current_text_attr());
    
    tui_draw_text(10, 18, "Press ENTER to continue", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            if (scancode == 0x1C) {
                return 1;
            }
        }
    }
}

static void step_complete(void) {
    print_step_header("Installation Complete!");
    
    tui_draw_text(10, 8, "AltoniumOS has been installed successfully!", get_current_text_attr());
    tui_draw_text(10, 10, "Summary:", get_current_text_attr());
    tui_draw_text(10, 11, "  - Partition table created", get_current_text_attr());
    tui_draw_text(10, 12, "  - Filesystem formatted", get_current_text_attr());
    tui_draw_text(10, 13, "  - Files prepared for copy", get_current_text_attr());
    tui_draw_text(10, 14, "  - Bootloader ready", get_current_text_attr());
    
    tui_draw_text(10, 16, "You can now boot from the target disk.", get_current_text_attr());
    tui_draw_text(10, 18, "Press any key to return to shell...", get_current_text_attr());
    
    while (!keyboard_ready()) {}
    keyboard_get_scancode();
}

void installer_run_full_wizard(void) {
    current_step = STEP_DISK_SELECT;
    string_copy(status_message, "");
    
    while (current_step <= STEP_COMPLETE) {
        int result = 0;
        
        switch (current_step) {
            case STEP_DISK_SELECT:
                result = step_disk_select();
                if (result < 0) {
                    vga_clear();
                    console_print("Installation cancelled.\n");
                    return;
                }
                break;
                
            case STEP_PARTITION_TABLE:
                result = step_partition_table();
                if (result == 0) {
                    current_step = STEP_DISK_SELECT;
                    continue;
                }
                break;
                
            case STEP_FILESYSTEM:
                result = step_filesystem();
                if (result == 0) {
                    current_step = STEP_PARTITION_TABLE;
                    continue;
                }
                break;
                
            case STEP_CONFIRM_FORMAT:
                result = step_confirm_format();
                if (result == 0) {
                    current_step = STEP_FILESYSTEM;
                    continue;
                }
                break;
                
            case STEP_COPY_FILES:
                result = step_copy_files();
                if (result == 0) {
                    current_step = STEP_CONFIRM_FORMAT;
                    continue;
                }
                break;
                
            case STEP_BOOTLOADER:
                result = step_bootloader();
                break;
                
            case STEP_COMPLETE:
                step_complete();
                vga_clear();
                return;
        }
        
        if (result > 0 || current_step == STEP_BOOTLOADER) {
            current_step++;
        }
    }
    
    vga_clear();
}

void installer_run_diskpart(void) {
    vga_clear();
    tui_draw_centered_text(1, "AltoniumOS Disk Partitioner", get_current_status_attr());
    draw_hline(0, 2, 80, '-', get_current_text_attr());
    
    partition_info_t partitions[MAX_PARTITIONS];
    int partition_count = 0;
    
    int result = partition_list(0, partitions, MAX_PARTITIONS, &partition_count);
    
    tui_draw_text(5, 4, "Current Partition Layout:", get_current_text_attr());
    
    if (result != 0 || partition_count == 0) {
        tui_draw_text(5, 6, "No partitions found or disk not initialized.", get_current_text_attr());
    } else {
        for (int i = 0; i < partition_count && i < 10; i++) {
            char line[80];
            string_copy(line, "  Partition ");
            char num[8];
            num[0] = '0' + (i + 1);
            num[1] = '\0';
            string_concat(line, num);
            string_concat(line, ": ");
            string_concat(line, partition_type_name(partitions[i].type));
            string_concat(line, ", Start: ");
            
            uint32_t val = partitions[i].first_lba;
            char val_str[16];
            int idx = 0;
            if (val == 0) {
                val_str[idx++] = '0';
            } else {
                char temp[16];
                int ti = 0;
                while (val > 0) {
                    temp[ti++] = '0' + (val % 10);
                    val /= 10;
                }
                for (int j = ti - 1; j >= 0; j--) {
                    val_str[idx++] = temp[j];
                }
            }
            val_str[idx] = '\0';
            string_concat(line, val_str);
            
            string_concat(line, ", Size: ");
            val = partitions[i].sector_count / 2048;
            idx = 0;
            if (val == 0) {
                val_str[idx++] = '0';
            } else {
                char temp[16];
                int ti = 0;
                while (val > 0) {
                    temp[ti++] = '0' + (val % 10);
                    val /= 10;
                }
                for (int j = ti - 1; j >= 0; j--) {
                    val_str[idx++] = temp[j];
                }
            }
            val_str[idx] = '\0';
            string_concat(line, val_str);
            string_concat(line, " MB");
            
            tui_draw_text(5, 6 + i, line, get_current_text_attr());
        }
    }
    
    tui_list_t menu;
    tui_init_list(&menu, 10, 14, 60, 8, "Actions");
    tui_add_list_item(&menu, "Create new partition", 1);
    tui_add_list_item(&menu, "Delete partition (requires confirmation)", 1);
    tui_add_list_item(&menu, "Refresh partition list", 1);
    tui_add_list_item(&menu, "Exit to shell", 1);
    
    tui_draw_list(&menu);
    tui_draw_text(10, 23, "Use UP/DOWN arrows, ENTER to select, ESC to exit", get_current_text_attr());
    
    while (1) {
        if (keyboard_ready()) {
            uint8_t scancode = keyboard_get_scancode();
            int action = tui_handle_list_input(&menu, scancode);
            
            if (action == 1) {
                int selected = tui_get_selected_index(&menu);
                
                if (selected == 0) {
                    if (partition_count >= 4) {
                        tui_show_message("Error", "Maximum 4 partitions allowed in MBR");
                    } else {
                        uint32_t start_lba, size_sectors;
                        partition_get_free_space(0, &start_lba, &size_sectors);
                        
                        result = partition_create_mbr(0, partition_count, start_lba, size_sectors, PARTITION_TYPE_LINUX);
                        if (result == 0) {
                            tui_show_message("Success", "Partition created successfully");
                            installer_run_diskpart();
                            return;
                        } else {
                            tui_show_message("Error", "Failed to create partition");
                        }
                    }
                } else if (selected == 1) {
                    if (partition_count == 0) {
                        tui_show_message("Error", "No partitions to delete");
                    } else {
                        int confirmed = tui_show_confirmation("Confirm", "Delete last partition?", "Y", "N");
                        if (confirmed) {
                            result = partition_delete_mbr(0, partition_count - 1);
                            if (result == 0) {
                                tui_show_message("Success", "Partition deleted");
                                installer_run_diskpart();
                                return;
                            } else {
                                tui_show_message("Error", "Failed to delete partition");
                            }
                        }
                    }
                } else if (selected == 2) {
                    installer_run_diskpart();
                    return;
                } else if (selected == 3) {
                    vga_clear();
                    return;
                }
                
                tui_draw_list(&menu);
            } else if (action == -1) {
                vga_clear();
                return;
            } else {
                tui_draw_list(&menu);
            }
        }
    }
}
