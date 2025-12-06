#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H

#include "../lib/string.h"

#define FS_IO_BUFFER_SIZE 16384

void commands_init(void);
int commands_is_fat_ready(void);
void commands_set_fat_ready(int ready);
void execute_command(const char *cmd_line);

void handle_clear(void);
void handle_echo(const char *args);
void handle_fetch(void);
void handle_help(void);
void handle_shutdown(void);
void handle_disk(void);
void handle_ls(const char *args);
void handle_pwd(void);
void handle_cd(const char *args);
void handle_cat(const char *args);
void handle_touch(const char *args);
void handle_write_command(const char *args);
void handle_mkdir_command(const char *args);
void handle_rm_command(const char *args);
void handle_nano_command(const char *args);
void handle_theme_command(const char *args);
void handle_fsstat_command(void);
void handle_bootlog_command(void);
void handle_hwinfo_command(void);

const char *fat12_error_string(int code);
void print_fs_error(int code);
uint8_t *commands_get_io_buffer(void);

#endif
