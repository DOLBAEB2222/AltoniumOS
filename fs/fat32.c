#include "../include/fs/fat32.h"
#include "../include/fs/vfs.h"

#define FAT32_CLUSTER_FREE 0x00000000
#define FAT32_CLUSTER_EOC  0x0FFFFFF8
#define FAT32_DIR_ENTRY_SIZE 32

#define FAT32_MAX_FAT_SECTORS          512
#define FAT32_MAX_ROOT_DIR_SECTORS     64
#define FAT32_MAX_SECTORS_PER_CLUSTER  32
#define FAT32_MAX_PATH_DEPTH           16

typedef struct __attribute__((packed)) {
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} fat32_raw_dir_entry_t;

typedef struct __attribute__((packed)) {
    uint8_t jump[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} fat32_bpb_t;

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint32_t root_cluster;
    uint32_t total_sectors;
    uint32_t sectors_per_fat;
    uint32_t fat_start_lba;
    uint32_t data_start_lba;
    uint32_t total_data_sectors;
    uint32_t total_clusters;
    uint32_t cluster_size_bytes;
    uint32_t base_lba;
    uint32_t fat_size_bytes;
} fat32_fs_t;

static fat32_fs_t g_fs;
static uint8_t g_fat_cache[8 * SECTOR_SIZE];
static uint32_t g_fat_cache_sector = 0xFFFFFFFF;
static uint8_t g_cluster_buffer[FAT32_MAX_SECTORS_PER_CLUSTER * SECTOR_SIZE];
static int g_fs_ready = 0;
static int g_fat_dirty = 0;
static uint32_t g_current_dir_cluster = 0;
static char g_cwd[VFS_PATH_MAX] = "/";

static unsigned int fat32_strlen(const char *str) {
    unsigned int len = 0;
    if (!str) return 0;
    while (str[len]) len++;
    return len;
}

static void fat32_memset(void *dest, uint8_t value, unsigned int count) {
    uint8_t *ptr = (uint8_t *)dest;
    while (count--) *ptr++ = value;
}

static void fat32_memcpy(void *dest, const void *src, unsigned int count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (count--) *d++ = *s++;
}

static int fat32_memcmp(const void *a, const void *b, unsigned int count) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (count--) {
        if (*pa != *pb) return *pa - *pb;
        pa++;
        pb++;
    }
    return 0;
}

static char fat32_to_upper(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - 32);
    return c;
}

static void fat32_dir_name_to_string(const uint8_t raw[11], char *out) {
    int pos = 0;
    int has_ext = 0;
    for (int i = 0; i < 8; i++) {
        if (raw[i] != ' ') out[pos++] = raw[i];
    }
    for (int i = 8; i < 11; i++) {
        if (raw[i] != ' ') {
            has_ext = 1;
            break;
        }
    }
    if (has_ext && pos < VFS_MAX_DISPLAY_NAME - 1) out[pos++] = '.';
    for (int i = 8; i < 11; i++) {
        if (raw[i] != ' ' && pos < VFS_MAX_DISPLAY_NAME - 1) out[pos++] = raw[i];
    }
    if (pos == 0) out[pos++] = '?';
    out[pos] = '\0';
}

static int fat32_is_valid_char(char c) {
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c == '_' || c == '-') return 1;
    return 0;
}

static int fat32_make_short_name(const char *input, uint8_t out[11]) {
    for (int i = 0; i < 11; i++) out[i] = ' ';
    if (!input || *input == '\0') return VFS_ERR_INVALID_NAME;
    char temp[32];
    int len = 0;
    while (*input && len < (int)(sizeof(temp) - 1)) {
        temp[len++] = fat32_to_upper(*input++);
    }
    temp[len] = '\0';
    int dot_index = -1;
    for (int i = 0; i < len; i++) {
        if (temp[i] == '.') {
            if (dot_index != -1) return VFS_ERR_INVALID_NAME;
            dot_index = i;
        } else if (temp[i] == ' ') {
            return VFS_ERR_INVALID_NAME;
        }
    }
    int base_len = (dot_index == -1) ? len : dot_index;
    int ext_len = (dot_index == -1) ? 0 : (len - dot_index - 1);
    if (base_len <= 0 || base_len > 8) return VFS_ERR_INVALID_NAME;
    if (ext_len < 0 || ext_len > 3) return VFS_ERR_INVALID_NAME;
    for (int i = 0; i < base_len; i++) {
        char c = temp[i];
        if (!fat32_is_valid_char(c)) return VFS_ERR_INVALID_NAME;
        out[i] = c;
    }
    for (int i = 0; i < ext_len; i++) {
        char c = temp[dot_index + 1 + i];
        if (!fat32_is_valid_char(c)) return VFS_ERR_INVALID_NAME;
        out[8 + i] = c;
    }
    return VFS_OK;
}

static uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    if (cluster < 2) return g_fs.data_start_lba;
    uint32_t rel_cluster = cluster - 2;
    return g_fs.data_start_lba + (rel_cluster * g_fs.sectors_per_cluster);
}

static int fat32_read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t lba = fat32_cluster_to_lba(cluster);
    return disk_read_sectors(g_fs.base_lba + lba, buffer, g_fs.sectors_per_cluster);
}

static int fat32_write_cluster(uint32_t cluster, const uint8_t *buffer) {
    uint32_t lba = fat32_cluster_to_lba(cluster);
    return disk_write_sectors(g_fs.base_lba + lba, buffer, g_fs.sectors_per_cluster);
}

static int fat32_flush_fat_cache(void) {
    if (!g_fat_dirty || g_fat_cache_sector == 0xFFFFFFFF) return VFS_OK;
    for (uint8_t fat_index = 0; fat_index < g_fs.num_fats; fat_index++) {
        uint32_t start_lba = g_fs.base_lba + g_fs.fat_start_lba + (fat_index * g_fs.sectors_per_fat) + g_fat_cache_sector;
        for (int i = 0; i < 8 && (g_fat_cache_sector + i) < g_fs.sectors_per_fat; i++) {
            int res = disk_write_sector(start_lba + i, g_fat_cache + i * SECTOR_SIZE);
            if (res != 0) return VFS_ERR_IO;
        }
    }
    g_fat_dirty = 0;
    return VFS_OK;
}

static uint32_t fat32_get_fat_entry(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_offset / SECTOR_SIZE;
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;
    if (fat_sector != g_fat_cache_sector) {
        fat32_flush_fat_cache();
        uint32_t sectors_to_read = 8;
        if (fat_sector + sectors_to_read > g_fs.sectors_per_fat) {
            sectors_to_read = g_fs.sectors_per_fat - fat_sector;
        }
        if (disk_read_sectors(g_fs.base_lba + g_fs.fat_start_lba + fat_sector, g_fat_cache, sectors_to_read) != 0) {
            return FAT32_CLUSTER_EOC;
        }
        g_fat_cache_sector = fat_sector;
    }
    uint32_t value = *((uint32_t *)(g_fat_cache + entry_offset));
    return value & 0x0FFFFFFF;
}

static void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_offset / SECTOR_SIZE;
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;
    if (fat_sector != g_fat_cache_sector) {
        fat32_flush_fat_cache();
        uint32_t sectors_to_read = 8;
        if (fat_sector + sectors_to_read > g_fs.sectors_per_fat) {
            sectors_to_read = g_fs.sectors_per_fat - fat_sector;
        }
        if (disk_read_sectors(g_fs.base_lba + g_fs.fat_start_lba + fat_sector, g_fat_cache, sectors_to_read) != 0) {
            return;
        }
        g_fat_cache_sector = fat_sector;
    }
    value &= 0x0FFFFFFF;
    *((uint32_t *)(g_fat_cache + entry_offset)) = value;
    g_fat_dirty = 1;
}

static uint32_t fat32_allocate_cluster(void) {
    for (uint32_t cluster = 2; cluster < g_fs.total_clusters + 2; cluster++) {
        if (fat32_get_fat_entry(cluster) == FAT32_CLUSTER_FREE) {
            fat32_set_fat_entry(cluster, FAT32_CLUSTER_EOC);
            fat32_memset(g_cluster_buffer, 0, g_fs.cluster_size_bytes);
            if (fat32_write_cluster(cluster, g_cluster_buffer) != 0) {
                fat32_set_fat_entry(cluster, FAT32_CLUSTER_FREE);
                return 0;
            }
            return cluster;
        }
    }
    return 0;
}

static void fat32_free_chain(uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC) {
        uint32_t next = fat32_get_fat_entry(cluster);
        fat32_set_fat_entry(cluster, FAT32_CLUSTER_FREE);
        cluster = next;
    }
}

static int fat32_is_dot_entry(const fat32_raw_dir_entry_t *entry) {
    if ((entry->attr & VFS_ATTR_DIRECTORY) == 0) return 0;
    if (entry->name[0] != '.') return 0;
    if (entry->name[1] == ' ' && entry->name[2] == ' ') return 1;
    if (entry->name[1] == '.' && entry->name[2] == ' ') return 1;
    return 0;
}

static int fat32_iterate_directory(uint32_t dir_cluster, vfs_dir_iter_cb cb, void *context) {
    if (dir_cluster < 2) dir_cluster = g_fs.root_cluster;
    uint32_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC) {
        if (fat32_read_cluster(cluster, g_cluster_buffer) != 0) {
            return VFS_ERR_IO;
        }
        for (uint32_t offset = 0; offset < g_fs.cluster_size_bytes; offset += FAT32_DIR_ENTRY_SIZE) {
            fat32_raw_dir_entry_t *entry = (fat32_raw_dir_entry_t *)(g_cluster_buffer + offset);
            if (entry->name[0] == 0x00) return VFS_OK;
            if (entry->name[0] == 0xE5) continue;
            if (entry->attr == 0x0F) continue;
            if (fat32_is_dot_entry(entry)) continue;
            vfs_dir_entry_t info;
            fat32_dir_name_to_string(entry->name, info.name);
            info.attr = entry->attr;
            info.size = entry->file_size;
            info.inode = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
            if (cb && cb(&info, context)) return VFS_OK;
        }
        cluster = fat32_get_fat_entry(cluster);
    }
    return VFS_OK;
}

static int fat32_find_entry(uint32_t dir_cluster, const uint8_t short_name[11], fat32_raw_dir_entry_t *out_entry, uint32_t *out_owner_cluster, uint32_t *out_entry_index) {
    if (dir_cluster < 2) dir_cluster = g_fs.root_cluster;
    uint32_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC) {
        if (fat32_read_cluster(cluster, g_cluster_buffer) != 0) {
            return VFS_ERR_IO;
        }
        for (uint32_t idx = 0; idx < g_fs.cluster_size_bytes / FAT32_DIR_ENTRY_SIZE; idx++) {
            fat32_raw_dir_entry_t *entry = (fat32_raw_dir_entry_t *)(g_cluster_buffer + idx * FAT32_DIR_ENTRY_SIZE);
            if (entry->name[0] == 0x00) return VFS_ERR_NOT_FOUND;
            if (entry->name[0] == 0xE5) continue;
            if (fat32_memcmp(entry->name, short_name, 11) == 0) {
                if (out_entry) fat32_memcpy(out_entry, entry, sizeof(fat32_raw_dir_entry_t));
                if (out_owner_cluster) *out_owner_cluster = cluster;
                if (out_entry_index) *out_entry_index = idx;
                return VFS_OK;
            }
        }
        cluster = fat32_get_fat_entry(cluster);
    }
    return VFS_ERR_NOT_FOUND;
}

static int fat32_init(uint32_t base_lba) {
    g_fs_ready = 0;
    g_current_dir_cluster = 0;
    g_fat_dirty = 0;
    g_fat_cache_sector = 0xFFFFFFFF;
    g_cwd[0] = '/';
    g_cwd[1] = '\0';
    uint8_t boot_sector[SECTOR_SIZE];
    if (disk_read_sector(base_lba, boot_sector) != 0) {
        return VFS_ERR_IO;
    }
    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        return VFS_ERR_BAD_FS;
    }
    fat32_bpb_t *bpb = (fat32_bpb_t *)boot_sector;
    g_fs.bytes_per_sector = bpb->bytes_per_sector;
    g_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    g_fs.reserved_sectors = bpb->reserved_sectors;
    g_fs.num_fats = bpb->num_fats;
    g_fs.root_cluster = bpb->root_cluster;
    g_fs.total_sectors = bpb->total_sectors_32 ? bpb->total_sectors_32 : bpb->total_sectors_16;
    g_fs.sectors_per_fat = bpb->sectors_per_fat_32 ? bpb->sectors_per_fat_32 : bpb->sectors_per_fat_16;
    if (g_fs.bytes_per_sector != 512) return VFS_ERR_BAD_FS;
    if (g_fs.sectors_per_cluster == 0 || g_fs.sectors_per_cluster > FAT32_MAX_SECTORS_PER_CLUSTER) return VFS_ERR_BAD_FS;
    g_fs.cluster_size_bytes = g_fs.bytes_per_sector * g_fs.sectors_per_cluster;
    g_fs.fat_start_lba = g_fs.reserved_sectors;
    g_fs.data_start_lba = g_fs.reserved_sectors + g_fs.num_fats * g_fs.sectors_per_fat;
    g_fs.total_data_sectors = g_fs.total_sectors - g_fs.data_start_lba;
    g_fs.total_clusters = g_fs.total_data_sectors / g_fs.sectors_per_cluster;
    g_fs.base_lba = base_lba;
    g_fs.fat_size_bytes = g_fs.sectors_per_fat * g_fs.bytes_per_sector;
    g_current_dir_cluster = g_fs.root_cluster;
    g_fs_ready = 1;
    return VFS_OK;
}

static int fat32_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    uint8_t short_name[11];
    if (fat32_make_short_name(path, short_name) != VFS_OK) return VFS_ERR_INVALID_NAME;
    fat32_raw_dir_entry_t entry;
    if (fat32_find_entry(g_current_dir_cluster, short_name, &entry, 0, 0) != VFS_OK) {
        return VFS_ERR_NOT_FOUND;
    }
    if (entry.attr & VFS_ATTR_DIRECTORY) return VFS_ERR_NOT_FILE;
    uint32_t file_size = entry.file_size;
    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32_t bytes_read = 0;
    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC && bytes_read < file_size) {
        if (fat32_read_cluster(cluster, g_cluster_buffer) != 0) return VFS_ERR_IO;
        uint32_t bytes_in_cluster = (file_size - bytes_read > g_fs.cluster_size_bytes) ? 
                                    g_fs.cluster_size_bytes : (file_size - bytes_read);
        if (bytes_read + bytes_in_cluster > max_size) {
            bytes_in_cluster = max_size - bytes_read;
        }
        fat32_memcpy(buffer + bytes_read, g_cluster_buffer, bytes_in_cluster);
        bytes_read += bytes_in_cluster;
        if (bytes_read >= max_size) break;
        cluster = fat32_get_fat_entry(cluster);
    }
    if (out_size) *out_size = bytes_read;
    return VFS_OK;
}

static int fat32_write_file(const char *name, const uint8_t *data, uint32_t size) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int fat32_create_directory(const char *name) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int fat32_delete_file(const char *name) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int fat32_iterate_current_directory(vfs_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return fat32_iterate_directory(g_current_dir_cluster, cb, context);
}

static int fat32_iterate_path(const char *path, vfs_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return VFS_ERR_UNSUPPORTED;
}

static int fat32_change_directory(const char *path) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    if (!path) return VFS_ERR_INVALID_NAME;
    if (path[0] == '/' && path[1] == '\0') {
        g_current_dir_cluster = g_fs.root_cluster;
        g_cwd[0] = '/';
        g_cwd[1] = '\0';
        return VFS_OK;
    }
    uint8_t short_name[11];
    if (fat32_make_short_name(path, short_name) != VFS_OK) return VFS_ERR_INVALID_NAME;
    fat32_raw_dir_entry_t entry;
    if (fat32_find_entry(g_current_dir_cluster, short_name, &entry, 0, 0) != VFS_OK) {
        return VFS_ERR_NOT_FOUND;
    }
    if ((entry.attr & VFS_ATTR_DIRECTORY) == 0) return VFS_ERR_NOT_DIRECTORY;
    g_current_dir_cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    return VFS_OK;
}

static const char *fat32_get_cwd(void) {
    return g_cwd;
}

static int fat32_flush(void) {
    if (!g_fs_ready) return VFS_ERR_NOT_INITIALIZED;
    return fat32_flush_fat_cache();
}

static int fat32_get_fs_info(vfs_fs_info_t *info) {
    if (!info || !g_fs_ready) return VFS_ERR_IO;
    info->type = FS_TYPE_FAT32;
    info->name = "FAT32";
    info->total_size = g_fs.total_sectors * g_fs.bytes_per_sector;
    info->block_size = g_fs.cluster_size_bytes;
    info->total_blocks = g_fs.total_clusters;
    uint32_t free_clusters = 0;
    for (uint32_t cluster = 2; cluster < g_fs.total_clusters + 2; cluster++) {
        if (fat32_get_fat_entry(cluster) == FAT32_CLUSTER_FREE) {
            free_clusters++;
        }
    }
    info->free_blocks = free_clusters;
    info->free_size = free_clusters * g_fs.cluster_size_bytes;
    info->total_inodes = 0;
    info->free_inodes = 0;
    return VFS_OK;
}

vfs_operations_t fat32_get_vfs_ops(void) {
    vfs_operations_t ops;
    ops.init = fat32_init;
    ops.mount = fat32_init;
    ops.unmount = 0;
    ops.read_file = fat32_read_file;
    ops.write_file = fat32_write_file;
    ops.create_directory = fat32_create_directory;
    ops.delete_file = fat32_delete_file;
    ops.iterate_current_directory = fat32_iterate_current_directory;
    ops.iterate_path = fat32_iterate_path;
    ops.change_directory = fat32_change_directory;
    ops.get_cwd = fat32_get_cwd;
    ops.flush = fat32_flush;
    ops.get_fs_info = fat32_get_fs_info;
    return ops;
}
