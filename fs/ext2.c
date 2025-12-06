#include "../include/fs/ext2.h"
#include "../include/fs/vfs.h"

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_MAGIC 0xEF53

typedef struct __attribute__((packed)) {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
} ext2_superblock_t;

static int g_fs_ready = 0;
static ext2_superblock_t g_superblock;
static uint32_t g_base_lba = 0;
static uint32_t g_block_size = 0;
static char g_cwd[VFS_PATH_MAX] = "/";

static int ext2_init(uint32_t base_lba) {
    g_fs_ready = 0;
    g_base_lba = base_lba;
    uint8_t buffer[1024];
    if (disk_read_sectors(base_lba + 2, buffer, 2) != 0) {
        return VFS_ERR_IO;
    }
    ext2_superblock_t *sb = (ext2_superblock_t *)buffer;
    if (sb->magic != EXT2_MAGIC) {
        return VFS_ERR_BAD_FS;
    }
    g_superblock = *sb;
    g_block_size = 1024 << g_superblock.log_block_size;
    g_cwd[0] = '/';
    g_cwd[1] = '\0';
    g_fs_ready = 1;
    return VFS_OK;
}

static int ext2_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_write_file(const char *name, const uint8_t *data, uint32_t size) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_create_directory(const char *name) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_delete_file(const char *name) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_iterate_current_directory(vfs_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_iterate_path(const char *path, vfs_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int ext2_change_directory(const char *path) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static const char *ext2_get_cwd(void) {
    return g_cwd;
}

static int ext2_flush(void) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_OK;
}

static int ext2_get_fs_info(vfs_fs_info_t *info) {
    if (!info || !g_fs_ready) return VFS_ERR_IO;
    info->type = FS_TYPE_EXT2;
    info->name = "ext2";
    info->total_size = g_superblock.blocks_count * g_block_size;
    info->free_size = g_superblock.free_blocks_count * g_block_size;
    info->block_size = g_block_size;
    info->total_blocks = g_superblock.blocks_count;
    info->free_blocks = g_superblock.free_blocks_count;
    info->total_inodes = g_superblock.inodes_count;
    info->free_inodes = g_superblock.free_inodes_count;
    return VFS_OK;
}

vfs_operations_t ext2_get_vfs_ops(void) {
    vfs_operations_t ops;
    ops.init = ext2_init;
    ops.mount = ext2_init;
    ops.unmount = 0;
    ops.read_file = ext2_read_file;
    ops.write_file = ext2_write_file;
    ops.create_directory = ext2_create_directory;
    ops.delete_file = ext2_delete_file;
    ops.iterate_current_directory = ext2_iterate_current_directory;
    ops.iterate_path = ext2_iterate_path;
    ops.change_directory = ext2_change_directory;
    ops.get_cwd = ext2_get_cwd;
    ops.flush = ext2_flush;
    ops.get_fs_info = ext2_get_fs_info;
    return ops;
}
