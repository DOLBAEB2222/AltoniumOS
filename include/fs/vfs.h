#ifndef VFS_H
#define VFS_H

#include "../lib/string.h"

#define VFS_OK                      0
#define VFS_ERR_IO                 -1
#define VFS_ERR_NOT_FOUND          -2
#define VFS_ERR_NOT_DIRECTORY      -3
#define VFS_ERR_NOT_FILE           -4
#define VFS_ERR_ALREADY_EXISTS     -5
#define VFS_ERR_INVALID_NAME       -6
#define VFS_ERR_NO_SPACE           -7
#define VFS_ERR_DIR_FULL           -8
#define VFS_ERR_BUFFER_SMALL       -9
#define VFS_ERR_NOT_INITIALIZED   -10
#define VFS_ERR_UNSUPPORTED       -11
#define VFS_ERR_BAD_FS            -12

#define VFS_ATTR_READ_ONLY 0x01
#define VFS_ATTR_HIDDEN    0x02
#define VFS_ATTR_SYSTEM    0x04
#define VFS_ATTR_DIRECTORY 0x10
#define VFS_ATTR_ARCHIVE   0x20

#define VFS_MAX_DISPLAY_NAME 256
#define VFS_PATH_MAX        256

typedef enum {
    FS_TYPE_UNKNOWN = 0,
    FS_TYPE_FAT12,
    FS_TYPE_FAT32,
    FS_TYPE_EXT2
} fs_type_t;

typedef struct {
    char name[VFS_MAX_DISPLAY_NAME];
    uint8_t attr;
    uint32_t size;
    uint32_t inode;
} vfs_dir_entry_t;

typedef int (*vfs_dir_iter_cb)(const vfs_dir_entry_t *entry, void *context);

typedef struct {
    fs_type_t type;
    const char *name;
    uint32_t total_size;
    uint32_t free_size;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t total_inodes;
    uint32_t free_inodes;
} vfs_fs_info_t;

typedef struct vfs_operations {
    int (*init)(uint32_t base_lba);
    int (*mount)(uint32_t base_lba);
    int (*unmount)(void);
    int (*read_file)(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size);
    int (*write_file)(const char *name, const uint8_t *data, uint32_t size);
    int (*create_directory)(const char *name);
    int (*delete_file)(const char *name);
    int (*iterate_current_directory)(vfs_dir_iter_cb cb, void *context);
    int (*iterate_path)(const char *path, vfs_dir_iter_cb cb, void *context);
    int (*change_directory)(const char *path);
    const char *(*get_cwd)(void);
    int (*flush)(void);
    int (*get_fs_info)(vfs_fs_info_t *info);
} vfs_operations_t;

int vfs_init(void);
int vfs_mount(uint32_t base_lba);
int vfs_unmount(void);
fs_type_t vfs_detect_fs_type(uint32_t base_lba);
const char *vfs_get_fs_type_name(fs_type_t type);
fs_type_t vfs_get_current_fs_type(void);
int vfs_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size);
int vfs_write_file(const char *name, const uint8_t *data, uint32_t size);
int vfs_create_directory(const char *name);
int vfs_delete_file(const char *name);
int vfs_iterate_current_directory(vfs_dir_iter_cb cb, void *context);
int vfs_iterate_path(const char *path, vfs_dir_iter_cb cb, void *context);
int vfs_change_directory(const char *path);
const char *vfs_get_cwd(void);
int vfs_flush(void);
int vfs_get_fs_info(vfs_fs_info_t *info);

const char *vfs_error_string(int code);

#endif /* VFS_H */
