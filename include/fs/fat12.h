#ifndef INCLUDE_FS_FAT12_H
#define INCLUDE_FS_FAT12_H

#include <kernel/types.h>
#include <drivers/disk.h>

#define FAT12_ATTR_READ_ONLY 0x01
#define FAT12_ATTR_HIDDEN    0x02
#define FAT12_ATTR_SYSTEM    0x04
#define FAT12_ATTR_VOLUME_ID 0x08
#define FAT12_ATTR_DIRECTORY 0x10
#define FAT12_ATTR_ARCHIVE   0x20

#define FAT12_OK                      0
#define FAT12_ERR_IO                 -1
#define FAT12_ERR_BAD_BPB            -2
#define FAT12_ERR_NOT_FAT12          -3
#define FAT12_ERR_OUT_OF_RANGE       -4
#define FAT12_ERR_NO_FREE_CLUSTER    -5
#define FAT12_ERR_INVALID_NAME       -6
#define FAT12_ERR_NOT_FOUND          -7
#define FAT12_ERR_NOT_DIRECTORY      -8
#define FAT12_ERR_ALREADY_EXISTS     -9
#define FAT12_ERR_DIR_FULL          -10
#define FAT12_ERR_BUFFER_SMALL      -11
#define FAT12_ERR_NOT_FILE          -12
#define FAT12_ERR_NOT_INITIALIZED   -13

#define FAT12_MAX_DISPLAY_NAME 13
#define FAT12_PATH_MAX        128

typedef struct {
    char name[FAT12_MAX_DISPLAY_NAME];
    uint8_t attr;
    uint32_t size;
    uint16_t first_cluster;
} fat12_dir_entry_info_t;

typedef int (*fat12_dir_iter_cb)(const fat12_dir_entry_info_t *entry, void *context);

int fat12_init(uint32_t base_lba);
int fat12_iterate_current_directory(fat12_dir_iter_cb cb, void *context);
int fat12_iterate_path(const char *path, fat12_dir_iter_cb cb, void *context);
int fat12_change_directory(const char *path);
const char *fat12_get_cwd(void);
int fat12_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size);
int fat12_write_file(const char *name, const uint8_t *data, uint32_t size);
int fat12_create_directory(const char *name);
int fat12_delete_file(const char *name);
int fat12_flush(void);

#endif /* INCLUDE_FS_FAT12_H */
