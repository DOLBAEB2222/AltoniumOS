#include "../include/fs/vfs.h"
#include "../include/fs/fat12.h"
#include "../include/fs/fat32.h"
#include "../include/fs/ext2.h"
#include "../disk.h"

static vfs_operations_t g_vfs_ops;
static fs_type_t g_current_fs_type = FS_TYPE_UNKNOWN;
static uint32_t g_current_base_lba = 0;

int vfs_init(void) {
    g_current_fs_type = FS_TYPE_UNKNOWN;
    g_current_base_lba = 0;
    g_vfs_ops.init = 0;
    g_vfs_ops.mount = 0;
    g_vfs_ops.unmount = 0;
    g_vfs_ops.read_file = 0;
    g_vfs_ops.write_file = 0;
    g_vfs_ops.create_directory = 0;
    g_vfs_ops.delete_file = 0;
    g_vfs_ops.iterate_current_directory = 0;
    g_vfs_ops.iterate_path = 0;
    g_vfs_ops.change_directory = 0;
    g_vfs_ops.get_cwd = 0;
    g_vfs_ops.flush = 0;
    g_vfs_ops.get_fs_info = 0;
    return VFS_OK;
}

int vfs_unmount(void) {
    if (g_vfs_ops.flush) {
        g_vfs_ops.flush();
    }
    if (g_vfs_ops.unmount) {
        g_vfs_ops.unmount();
    }
    vfs_init();
    return VFS_OK;
}

fs_type_t vfs_get_current_fs_type(void) {
    return g_current_fs_type;
}

static int vfs_set_ops(const vfs_operations_t *ops) {
    if (!ops) {
        return VFS_ERR_UNSUPPORTED;
    }
    g_vfs_ops = *ops;
    return VFS_OK;
}

const char *vfs_get_fs_type_name(fs_type_t type) {
    switch (type) {
        case FS_TYPE_FAT12: return "FAT12";
        case FS_TYPE_FAT32: return "FAT32";
        case FS_TYPE_EXT2: return "ext2";
        default: return "Unknown";
    }
}

static fs_type_t detect_fat(uint32_t base_lba) {
    uint8_t sector[512];
    if (disk_read_sector(base_lba, sector) != 0) {
        return FS_TYPE_UNKNOWN;
    }
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        return FS_TYPE_UNKNOWN;
    }
    uint16_t bytes_per_sector = sector[11] | (sector[12] << 8);
    uint8_t sectors_per_cluster = sector[13];
    uint16_t reserved_sectors = sector[14] | (sector[15] << 8);
    uint8_t num_fats = sector[16];
    uint16_t root_entry_count = sector[17] | (sector[18] << 8);
    uint16_t total_sectors_16 = sector[19] | (sector[20] << 8);
    uint32_t total_sectors_32 = sector[32] | (sector[33] << 8) | (sector[34] << 16) | (sector[35] << 24);
    if (bytes_per_sector != 512 || sectors_per_cluster == 0 || num_fats == 0) {
        return FS_TYPE_UNKNOWN;
    }
    uint32_t total_sectors = total_sectors_16 ? total_sectors_16 : total_sectors_32;
    if (total_sectors == 0) {
        return FS_TYPE_UNKNOWN;
    }
    uint32_t root_dir_sectors = ((root_entry_count * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    uint32_t fat_size_16 = sector[22] | (sector[23] << 8);
    uint32_t fat_size = fat_size_16;
    if (fat_size == 0) {
        fat_size = sector[36] | (sector[37] << 8) | (sector[38] << 16) | (sector[39] << 24);
    }
    uint32_t data_sectors = total_sectors - (reserved_sectors + num_fats * fat_size + root_dir_sectors);
    uint32_t total_clusters = data_sectors / sectors_per_cluster;
    if (total_clusters < 4085) {
        return FS_TYPE_FAT12;
    } else if (total_clusters < 65525) {
        return FS_TYPE_FAT32; /* treat FAT16 as FAT32 handler for now */
    } else {
        return FS_TYPE_FAT32;
    }
}

static fs_type_t detect_ext2(uint32_t base_lba) {
    uint8_t sector[1024];
    if (disk_read_sectors(base_lba + 2, sector, 2) != 0) {
        return FS_TYPE_UNKNOWN;
    }
    uint16_t magic = sector[56] | (sector[57] << 8);
    if (magic == 0xEF53) {
        return FS_TYPE_EXT2;
    }
    return FS_TYPE_UNKNOWN;
}

fs_type_t vfs_detect_fs_type(uint32_t base_lba) {
    fs_type_t type = detect_ext2(base_lba);
    if (type != FS_TYPE_UNKNOWN) {
        return type;
    }
    type = detect_fat(base_lba);
    if (type != FS_TYPE_UNKNOWN) {
        return type;
    }
    return FS_TYPE_UNKNOWN;
}

int vfs_mount(uint32_t base_lba) {
    fs_type_t type = vfs_detect_fs_type(base_lba);
    g_current_base_lba = base_lba;
    vfs_operations_t ops;
    switch (type) {
        case FS_TYPE_FAT12:
            ops = fat12_get_vfs_ops();
            break;
        case FS_TYPE_FAT32:
            ops = fat32_get_vfs_ops();
            break;
        case FS_TYPE_EXT2:
            ops = ext2_get_vfs_ops();
            break;
        default:
            return VFS_ERR_BAD_FS;
    }
    int status = vfs_set_ops(&ops);
    if (status != VFS_OK) {
        return status;
    }
    g_current_fs_type = type;
    if (g_vfs_ops.mount) {
        return g_vfs_ops.mount(base_lba);
    }
    return VFS_ERR_UNSUPPORTED;
}

int vfs_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size) {
    if (!g_vfs_ops.read_file) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.read_file(path, buffer, max_size, out_size);
}

int vfs_write_file(const char *name, const uint8_t *data, uint32_t size) {
    if (!g_vfs_ops.write_file) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.write_file(name, data, size);
}

int vfs_create_directory(const char *name) {
    if (!g_vfs_ops.create_directory) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.create_directory(name);
}

int vfs_delete_file(const char *name) {
    if (!g_vfs_ops.delete_file) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.delete_file(name);
}

int vfs_iterate_current_directory(vfs_dir_iter_cb cb, void *context) {
    if (!g_vfs_ops.iterate_current_directory) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.iterate_current_directory(cb, context);
}

int vfs_iterate_path(const char *path, vfs_dir_iter_cb cb, void *context) {
    if (!g_vfs_ops.iterate_path) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.iterate_path(path, cb, context);
}

int vfs_change_directory(const char *path) {
    if (!g_vfs_ops.change_directory) {
        return VFS_ERR_NOT_INITIALIZED;
    }
    return g_vfs_ops.change_directory(path);
}

const char *vfs_get_cwd(void) {
    if (!g_vfs_ops.get_cwd) {
        return "/";
    }
    return g_vfs_ops.get_cwd();
}

int vfs_flush(void) {
    if (!g_vfs_ops.flush) {
        return VFS_OK;
    }
    return g_vfs_ops.flush();
}

int vfs_get_fs_info(vfs_fs_info_t *info) {
    if (!g_vfs_ops.get_fs_info) {
        return VFS_ERR_UNSUPPORTED;
    }
    return g_vfs_ops.get_fs_info(info);
}

const char *vfs_error_string(int code) {
    switch (code) {
        case VFS_OK: return "ok";
        case VFS_ERR_IO: return "io";
        case VFS_ERR_NOT_FOUND: return "not found";
        case VFS_ERR_NOT_DIRECTORY: return "not dir";
        case VFS_ERR_NOT_FILE: return "not file";
        case VFS_ERR_ALREADY_EXISTS: return "exists";
        case VFS_ERR_INVALID_NAME: return "name";
        case VFS_ERR_NO_SPACE: return "disk full";
        case VFS_ERR_DIR_FULL: return "dir full";
        case VFS_ERR_BUFFER_SMALL: return "buffer";
        case VFS_ERR_NOT_INITIALIZED: return "fs offline";
        case VFS_ERR_UNSUPPORTED: return "unsupported";
        case VFS_ERR_BAD_FS: return "bad fs";
        default: return "unknown";
    }
}
