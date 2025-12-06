#include "../include/shell/commands.h"
#include "../include/shell/nano.h"
#include "../include/kernel/bootlog.h"
#include "../include/drivers/console.h"
#include "../disk.h"
#include "../include/fs/vfs.h"

extern void halt_cpu(void);
extern const char *get_boot_mode_name(void);
extern void bootlog_print(void);

static int fs_ready = 0;
static uint8_t fs_io_buffer[FS_IO_BUFFER_SIZE];

static const char *os_name = "AltoniumOS";
static const char *os_version = "1.0.0";
static const char *os_arch = "x86";
static const char *build_date = __DATE__;
static const char *build_time = __TIME__;

void commands_init(void) {
    fs_ready = 0;
}

int commands_is_fs_ready(void) {
    return fs_ready;
}

void commands_set_fs_ready(int ready) {
    fs_ready = ready;
}

uint8_t *commands_get_io_buffer(void) {
    return fs_io_buffer;
}

void print_fs_error(int code) {
    console_print(" (");
    console_print(vfs_error_string(code));
    console_print(" code ");
    print_decimal(code);
    console_print(")");
}

typedef struct {
    int count;
} ls_context_t;

static int ls_callback(const vfs_dir_entry_t *entry, void *context) {
    ls_context_t *ctx = (ls_context_t *)context;
    ctx->count++;
    if (entry->attr & VFS_ATTR_DIRECTORY) {
        console_print("[DIR] ");
    } else {
        console_print("      ");
    }
    console_print(entry->name);
    if ((entry->attr & VFS_ATTR_DIRECTORY) == 0) {
        console_print(" (");
        print_unsigned(entry->size);
        console_print(" bytes)");
    }
    console_print("\n");
    return 0;
}

void handle_clear(void) {
    vga_clear();
}

void handle_echo(const char *args) {
    if (args && *args) {
        console_print(args);
    }
    console_print("\n");
}

void handle_fetch(void) {
    console_print("OS: ");
    console_print(os_name);
    console_print("\n");
    
    console_print("Version: ");
    console_print(os_version);
    console_print("\n");
    
    console_print("Architecture: ");
    console_print(os_arch);
    console_print("\n");
    
    console_print("Build Date: ");
    console_print(build_date);
    console_print("\n");
    
    console_print("Build Time: ");
    console_print(build_time);
    console_print("\n");

    console_print("Boot Mode: ");
    console_print(get_boot_mode_name());
    console_print("\n");
}

void handle_help(void) {
    console_print("Available commands:\n");
    console_print("  clear          - Clear the screen\n");
    console_print("  echo TEXT      - Print text to the screen\n");
    console_print("  fetch          - Print OS and system information\n");
    console_print("  disk           - Test disk I/O and show disk information\n");
    console_print("  ls [PATH]      - List files in the current or given directory\n");
    console_print("  dir [PATH]     - Alias for ls\n");
    console_print("  pwd            - Show current directory\n");
    console_print("  cd PATH        - Change directory\n");
    console_print("  cat FILE       - Print file contents\n");
    console_print("  touch FILE     - Create a zero-length file\n");
    console_print("  write FILE TXT - Create/overwrite a text file\n");
    console_print("  mkdir NAME     - Create a directory\n");
    console_print("  rm FILE        - Delete a file\n");
    console_print("  nano FILE      - Text editor (Ctrl+S/Ctrl+X/Ctrl+T/Ctrl+H)\n");
    console_print("  theme [OPTION] - Switch theme (normal/blue/green) or 'list'\n");
    console_print("  fsstat         - Show filesystem/disk statistics\n");
    console_print("  bootlog        - Show BIOS boot diagnostics\n");
    console_print("  shutdown       - Shut down the system\n");
    console_print("  help           - Display this help message\n");
}

void handle_shutdown(void) {
    console_print("Attempting system shutdown...\n");
    if (fs_ready) {
        vfs_flush();
    }
    console_print("Halting CPU...\n");
    halt_cpu();
}

void handle_disk(void) {
    console_print("Disk Information:\n");
    
    int result = disk_init();
    if (result != 0) {
        console_print("  Disk initialization FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
        return;
    }
    
    console_print("  Disk initialization: OK\n");
    
    uint8_t buffer[512];
    result = disk_read_sector(0, buffer);
    if (result != 0) {
        console_print("  Sector 0 read: FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                error_str[pos++] = temp[i];
            }
        }
        error_str[pos] = '\0';
        console_print(error_str);
        console_print(")\n");
        return;
    }
    
    console_print("  Sector 0 read: OK\n");
    
    console_print("  First 64 bytes of sector 0:\n");
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) {
            console_print("    ");
        }
        
        uint8_t byte = buffer[i];
        char hex_chars[] = "0123456789ABCDEF";
        char hex_byte[3];
        hex_byte[0] = hex_chars[(byte >> 4) & 0xF];
        hex_byte[1] = hex_chars[byte & 0xF];
        hex_byte[2] = '\0';
        console_print(" ");
        console_print(hex_byte);
        
        if (i % 16 == 15) {
            console_print("\n");
        }
    }
    if (64 % 16 != 0) {
        console_print("\n");
    }
    
    console_print("  Disk self-test: ");
    result = disk_self_test();
    if (result != 0) {
        console_print("FAILED (error ");
        if (result < 0) {
            console_print("-");
            result = -result;
        }
        char error_str[16];
        int pos = 0;
        if (result == 0) {
            error_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (result > 0) {
                temp[temp_pos++] = '0' + (result % 10);
                result /= 10;
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

void handle_ls(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[VFS_PATH_MAX];
    int path_len = 0;
    if (path && *path) {
        path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
        if (path_len < 0) {
            console_print("ls failed (path too long)\n");
            return;
        }
    }
    ls_context_t ctx;
    ctx.count = 0;
    int result;
    if (path_len > 0) {
        result = vfs_iterate_path(path_buf, ls_callback, &ctx);
    } else {
        result = vfs_iterate_current_directory(ls_callback, &ctx);
    }
    if (result != VFS_OK) {
        console_print("ls failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    if (ctx.count == 0) {
        console_print("(empty)\n");
    }
}

void handle_pwd(void) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    console_print(vfs_get_cwd());
    console_print("\n");
}

void handle_cd(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[VFS_PATH_MAX];
    int path_len = 0;
    if (path && *path) {
        path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
        if (path_len < 0) {
            console_print("cd failed (path too long)\n");
            return;
        }
    }
    if (path_len == 0) {
        path_buf[0] = '/';
        path_buf[1] = '\0';
    }
    int result = vfs_change_directory(path_buf);
    if (result != VFS_OK) {
        console_print("cd failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print(vfs_get_cwd());
    console_print("\n");
}

void handle_cat(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *path = skip_whitespace(args);
    char path_buf[VFS_PATH_MAX];
    int path_len = copy_path_argument(path, path_buf, sizeof(path_buf));
    if (path_len < 0) {
        console_print("cat failed (path too long)\n");
        return;
    }
    if (path_len == 0) {
        console_print("Usage: cat FILE\n");
        return;
    }
    uint32_t size = 0;
    int result = vfs_read_file(path_buf, fs_io_buffer, FS_IO_BUFFER_SIZE - 1, &size);
    if (result != VFS_OK) {
        console_print("cat failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    for (uint32_t i = 0; i < size; i++) {
        console_putchar((char)fs_io_buffer[i]);
    }
    if (size == 0 || fs_io_buffer[size - 1] != '\n') {
        console_print("\n");
    }
}

void handle_touch(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[VFS_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: touch NAME\n");
        return;
    }
    int result = vfs_write_file(name_buf, 0, 0);
    if (result != VFS_OK) {
        console_print("touch failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Created empty file: ");
    console_print(name_buf);
    console_print("\n");
}

void handle_write_command(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[VFS_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: write NAME TEXT\n");
        return;
    }
    const char *payload = skip_whitespace(cursor);
    uint32_t length = 0;
    if (payload) {
        while (payload[length] != '\0') {
            if (length >= FS_IO_BUFFER_SIZE) {
                console_print("write failed (data too large)\n");
                return;
            }
            fs_io_buffer[length] = (uint8_t)payload[length];
            length++;
        }
    }
    int result = vfs_write_file(name_buf, fs_io_buffer, length);
    if (result != VFS_OK) {
        console_print("write failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Wrote ");
    print_unsigned(length);
    console_print(" bytes\n");
}

void handle_mkdir_command(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[VFS_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: mkdir NAME\n");
        return;
    }
    int result = vfs_create_directory(name_buf);
    if (result != VFS_OK) {
        console_print("mkdir failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("Directory created\n");
}

void handle_rm_command(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    const char *cursor = args;
    char name_buf[VFS_PATH_MAX];
    if (read_token(&cursor, name_buf, sizeof(name_buf)) == 0) {
        console_print("Usage: rm NAME\n");
        return;
    }
    int result = vfs_delete_file(name_buf);
    if (result != VFS_OK) {
        console_print("rm failed");
        print_fs_error(result);
        console_print("\n");
        return;
    }
    console_print("File deleted\n");
}

void handle_nano_command(const char *args) {
    if (!fs_ready) {
        console_print("Filesystem not initialized\n");
        return;
    }
    
    const char *cursor = args;
    char filename_buf[VFS_PATH_MAX];
    if (read_token(&cursor, filename_buf, sizeof(filename_buf)) == 0) {
        console_print("Usage: nano FILENAME\n");
        return;
    }
    
    nano_init_editor(filename_buf);
}

void handle_fsstat_command(void) {
    if (fs_ready) {
        vfs_fs_info_t fs_info;
        if (vfs_get_fs_info(&fs_info) == VFS_OK) {
            console_print("Filesystem Information:\n");
            console_print("  Type:               ");
            console_print(fs_info.name);
            console_print("\n");
            console_print("  Total size:         ");
            print_unsigned(fs_info.total_size);
            console_print(" bytes\n");
            console_print("  Free size:          ");
            print_unsigned(fs_info.free_size);
            console_print(" bytes\n");
            console_print("  Block size:         ");
            print_unsigned(fs_info.block_size);
            console_print(" bytes\n");
            console_print("  Total blocks:       ");
            print_unsigned(fs_info.total_blocks);
            console_print("\n");
            console_print("  Free blocks:        ");
            print_unsigned(fs_info.free_blocks);
            console_print("\n");
            if (fs_info.total_inodes > 0) {
                console_print("  Total inodes:       ");
                print_unsigned(fs_info.total_inodes);
                console_print("\n");
                console_print("  Free inodes:        ");
                print_unsigned(fs_info.free_inodes);
                console_print("\n");
            }
            console_print("\n");
        }
    }
    
    disk_stats_t disk_stats;
    disk_get_stats(&disk_stats);
    
    console_print("Disk I/O Statistics:\n");
    console_print("  Read operations:    ");
    print_unsigned(disk_stats.read_ops);
    console_print(" (");
    print_unsigned(disk_stats.read_sectors);
    console_print(" sectors)\n");
    
    console_print("  Write operations:   ");
    print_unsigned(disk_stats.write_ops);
    console_print(" (");
    print_unsigned(disk_stats.write_sectors);
    console_print(" sectors)\n");
    
    console_print("  Multi-read ops:     ");
    print_unsigned(disk_stats.read_multi_ops);
    console_print("\n");
    
    console_print("  Multi-write ops:    ");
    print_unsigned(disk_stats.write_multi_ops);
    console_print("\n");
    
    uint32_t total_ops = disk_stats.read_ops + disk_stats.write_ops;
    uint32_t multi_ops = disk_stats.read_multi_ops + disk_stats.write_multi_ops;
    console_print("  Total operations:   ");
    print_unsigned(total_ops);
    console_print("\n");
    
    if (total_ops > 0) {
        uint32_t multi_pct = (multi_ops * 100) / total_ops;
        console_print("  Multi-op ratio:     ");
        print_unsigned(multi_pct);
        console_print("%\n");
    }
}

void handle_theme_command(const char *args) {
    const char *cursor = args;
    char option_buf[32];
    const theme_t *themes = console_get_themes();
    
    if (read_token(&cursor, option_buf, sizeof(option_buf)) == 0) {
        console_print("Current theme: ");
        console_print(themes[console_get_theme()].name);
        console_print("\nUsage: theme [normal|blue|green|list]\n");
        return;
    }
    
    if (strcmp_impl(option_buf, "list") == 0) {
        console_print("Available themes:\n");
        for (int i = 0; i < THEME_COUNT; i++) {
            console_print("  ");
            console_print(themes[i].name);
            if (i == console_get_theme()) {
                console_print(" (current)");
            }
            console_print("\n");
        }
        return;
    }
    
    int found = -1;
    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcmp_impl(option_buf, themes[i].name) == 0) {
            found = i;
            break;
        }
    }
    
    if (found == -1) {
        console_print("Unknown theme: ");
        console_print(option_buf);
        console_print("\nAvailable: normal, blue, green\n");
        return;
    }
    
    console_set_theme(found);
    console_print("Theme changed to: ");
    console_print(themes[console_get_theme()].name);
    console_print("\n");
    
    vga_clear();
    console_print("Theme applied: ");
    console_print(themes[console_get_theme()].name);
    console_print("\n\n");
}

void handle_bootlog_command(void) {
    bootlog_print();
}

void execute_command(const char *cmd_line) {
    if (!cmd_line || *cmd_line == '\0') {
        return;
    }

    while (*cmd_line && (*cmd_line == ' ' || *cmd_line == '\t')) {
        cmd_line++;
    }

    if (strncmp_impl(cmd_line, "clear", 5) == 0 && 
        (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        handle_clear();
    } else if (strncmp_impl(cmd_line, "echo ", 5) == 0) {
        const char *args = cmd_line + 5;
        handle_echo(args);
    } else if (strncmp_impl(cmd_line, "fetch", 5) == 0 && 
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        handle_fetch();
    } else if (strncmp_impl(cmd_line, "disk", 4) == 0 && 
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        handle_disk();
    } else if (strncmp_impl(cmd_line, "ls", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_ls(args);
    } else if (strncmp_impl(cmd_line, "dir", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        const char *args = cmd_line + 3;
        handle_ls(args);
    } else if (strncmp_impl(cmd_line, "pwd", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        handle_pwd();
    } else if (strncmp_impl(cmd_line, "cd", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_cd(args);
    } else if (strncmp_impl(cmd_line, "cat", 3) == 0 &&
               (cmd_line[3] == '\0' || cmd_line[3] == ' ' || cmd_line[3] == '\n')) {
        const char *args = cmd_line + 3;
        handle_cat(args);
    } else if (strncmp_impl(cmd_line, "touch", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_touch(args);
    } else if (strncmp_impl(cmd_line, "write", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_write_command(args);
    } else if (strncmp_impl(cmd_line, "mkdir", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_mkdir_command(args);
    } else if (strncmp_impl(cmd_line, "rm", 2) == 0 &&
               (cmd_line[2] == '\0' || cmd_line[2] == ' ' || cmd_line[2] == '\n')) {
        const char *args = cmd_line + 2;
        handle_rm_command(args);
    } else if (strncmp_impl(cmd_line, "nano", 4) == 0 &&
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        const char *args = cmd_line + 4;
        handle_nano_command(args);
    } else if (strncmp_impl(cmd_line, "theme", 5) == 0 &&
               (cmd_line[5] == '\0' || cmd_line[5] == ' ' || cmd_line[5] == '\n')) {
        const char *args = cmd_line + 5;
        handle_theme_command(args);
    } else if (strncmp_impl(cmd_line, "fsstat", 6) == 0 &&
               (cmd_line[6] == '\0' || cmd_line[6] == ' ' || cmd_line[6] == '\n')) {
        handle_fsstat_command();
    } else if (strncmp_impl(cmd_line, "shutdown", 8) == 0 && 
               (cmd_line[8] == '\0' || cmd_line[8] == ' ' || cmd_line[8] == '\n')) {
        handle_shutdown();
    } else if (strncmp_impl(cmd_line, "bootlog", 7) == 0 && 
               (cmd_line[7] == '\0' || cmd_line[7] == ' ' || cmd_line[7] == '\n')) {
        handle_bootlog_command();
    } else if (strncmp_impl(cmd_line, "help", 4) == 0 && 
               (cmd_line[4] == '\0' || cmd_line[4] == ' ' || cmd_line[4] == '\n')) {
        handle_help();
    } else {
        console_print("Unknown command: ");
        console_print(cmd_line);
        console_print("\n");
    }
}
