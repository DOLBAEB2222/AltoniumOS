#include "fat12.h"

#define FAT12_CLUSTER_FREE 0x000
#define FAT12_CLUSTER_EOC  0x0FF8
#define FAT12_DIR_ENTRY_SIZE 32

#define FAT12_MAX_FAT_SECTORS          64
#define FAT12_MAX_ROOT_DIR_SECTORS     64
#define FAT12_MAX_SECTORS_PER_CLUSTER  32
#define FAT12_MAX_PATH_DEPTH           16

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
} fat12_raw_dir_entry_t;

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
} fat12_bpb_t;

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t root_dir_sectors;
    uint32_t total_sectors;
    uint16_t sectors_per_fat;
    uint32_t fat_start_lba;
    uint32_t root_dir_start_lba;
    uint32_t data_start_lba;
    uint32_t total_data_sectors;
    uint32_t total_clusters;
    uint32_t cluster_size_bytes;
    uint32_t base_lba;
    uint32_t fat_size_bytes;
} fat12_fs_t;

static fat12_fs_t g_fs;
static uint8_t g_fat_primary[FAT12_MAX_FAT_SECTORS * SECTOR_SIZE];
static uint8_t g_fat_secondary[FAT12_MAX_FAT_SECTORS * SECTOR_SIZE];
static uint8_t g_root_dir[FAT12_MAX_ROOT_DIR_SECTORS * SECTOR_SIZE];
static uint8_t g_cluster_buffer[FAT12_MAX_SECTORS_PER_CLUSTER * SECTOR_SIZE];

static int g_fs_ready = 0;
static int g_fat_dirty = 0;
static int g_root_dirty = 0;

static uint16_t g_current_dir_cluster = 0;
static uint16_t g_path_stack[FAT12_MAX_PATH_DEPTH];
static char g_path_names[FAT12_MAX_PATH_DEPTH][FAT12_MAX_DISPLAY_NAME];
static int g_path_depth = 0;
static char g_cwd[FAT12_PATH_MAX] = "/";

/* Utility helpers */
static unsigned int fat12_strlen(const char *str) {
    unsigned int len = 0;
    if (!str) {
        return 0;
    }
    while (str[len]) {
        len++;
    }
    return len;
}

static void fat12_memset(void *dest, uint8_t value, unsigned int count) {
    uint8_t *ptr = (uint8_t *)dest;
    while (count--) {
        *ptr++ = value;
    }
}

static void fat12_memcpy(void *dest, const void *src, unsigned int count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (count--) {
        *d++ = *s++;
    }
}

static int fat12_memcmp(const void *a, const void *b, unsigned int count) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (count--) {
        if (*pa != *pb) {
            return *pa - *pb;
        }
        pa++;
        pb++;
    }
    return 0;
}

static char fat12_to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 32);
    }
    return c;
}

static void fat12_rebuild_cwd(void) {
    int pos = 0;
    g_cwd[pos++] = '/';
    if (g_path_depth == 0) {
        g_cwd[pos] = '\0';
        return;
    }

    for (int i = 0; i < g_path_depth; i++) {
        unsigned int name_len = fat12_strlen(g_path_names[i]);
        if (name_len == 0) {
            continue;
        }
        if (pos + (int)name_len + 1 >= FAT12_PATH_MAX) {
            break;
        }
        for (unsigned int j = 0; j < name_len; j++) {
            g_cwd[pos++] = g_path_names[i][j];
        }
        if (i != g_path_depth - 1) {
            g_cwd[pos++] = '/';
        }
    }
    g_cwd[pos] = '\0';
}

static void fat12_dir_name_to_string(const uint8_t raw[11], char *out) {
    int pos = 0;
    int has_ext = 0;
    for (int i = 0; i < 8; i++) {
        if (raw[i] != ' ') {
            out[pos++] = raw[i];
        }
    }
    for (int i = 8; i < 11; i++) {
        if (raw[i] != ' ') {
            has_ext = 1;
            break;
        }
    }
    if (has_ext && pos < FAT12_MAX_DISPLAY_NAME - 1) {
        out[pos++] = '.';
    }
    for (int i = 8; i < 11; i++) {
        if (raw[i] != ' ' && pos < FAT12_MAX_DISPLAY_NAME - 1) {
            out[pos++] = raw[i];
        }
    }
    if (pos == 0) {
        out[pos++] = '?';
    }
    out[pos] = '\0';
}

static int fat12_is_valid_char(char c) {
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c == '_' || c == '-') return 1;
    return 0;
}

static int fat12_make_short_name(const char *input, uint8_t out[11]) {
    for (int i = 0; i < 11; i++) {
        out[i] = ' ';
    }

    if (!input || *input == '\0') {
        return FAT12_ERR_INVALID_NAME;
    }

    char temp[32];
    int len = 0;
    while (*input && len < (int)(sizeof(temp) - 1)) {
        temp[len++] = fat12_to_upper(*input++);
    }
    temp[len] = '\0';

    int dot_index = -1;
    for (int i = 0; i < len; i++) {
        if (temp[i] == '.') {
            if (dot_index != -1) {
                return FAT12_ERR_INVALID_NAME;
            }
            dot_index = i;
        } else if (temp[i] == ' ') {
            return FAT12_ERR_INVALID_NAME;
        }
    }

    int base_len = (dot_index == -1) ? len : dot_index;
    int ext_len = (dot_index == -1) ? 0 : (len - dot_index - 1);

    if (base_len <= 0 || base_len > 8) {
        return FAT12_ERR_INVALID_NAME;
    }
    if (ext_len < 0 || ext_len > 3) {
        return FAT12_ERR_INVALID_NAME;
    }

    for (int i = 0; i < base_len; i++) {
        char c = temp[i];
        if (!fat12_is_valid_char(c)) {
            return FAT12_ERR_INVALID_NAME;
        }
        out[i] = c;
    }

    for (int i = 0; i < ext_len; i++) {
        char c = temp[dot_index + 1 + i];
        if (!fat12_is_valid_char(c)) {
            return FAT12_ERR_INVALID_NAME;
        }
        out[8 + i] = c;
    }

    return FAT12_OK;
}


static uint32_t fat12_cluster_to_lba(uint16_t cluster) {
    if (cluster < 2) {
        return g_fs.data_start_lba;
    }
    uint32_t rel_cluster = (uint32_t)(cluster - 2);
    return g_fs.data_start_lba + (rel_cluster * g_fs.sectors_per_cluster);
}

static int fat12_read_cluster(uint16_t cluster, uint8_t *buffer) {
    uint32_t lba = fat12_cluster_to_lba(cluster);
    return disk_read_sectors(g_fs.base_lba + lba, buffer, g_fs.sectors_per_cluster);
}

static int fat12_write_cluster(uint16_t cluster, const uint8_t *buffer) {
    uint32_t lba = fat12_cluster_to_lba(cluster);
    return disk_write_sectors(g_fs.base_lba + lba, buffer, g_fs.sectors_per_cluster);
}

static uint16_t fat12_get_fat_entry(uint16_t cluster) {
    uint32_t index = (uint32_t)cluster + (cluster / 2);
    if (index + 1 >= g_fs.fat_size_bytes) {
        return FAT12_CLUSTER_EOC;
    }
    uint16_t value;
    if ((cluster & 1) == 0) {
        value = (uint16_t)(g_fat_primary[index] | ((g_fat_primary[index + 1] & 0x0F) << 8));
    } else {
        value = (uint16_t)(((g_fat_primary[index] & 0xF0) >> 4) | (g_fat_primary[index + 1] << 4));
    }
    return (uint16_t)(value & 0x0FFF);
}

static void fat12_set_fat_entry(uint16_t cluster, uint16_t value) {
    uint32_t index = (uint32_t)cluster + (cluster / 2);
    value &= 0x0FFF;
    if ((cluster & 1) == 0) {
        g_fat_primary[index] = (uint8_t)(value & 0xFF);
        g_fat_primary[index + 1] = (uint8_t)((g_fat_primary[index + 1] & 0xF0) | ((value >> 8) & 0x0F));
    } else {
        g_fat_primary[index] = (uint8_t)((g_fat_primary[index] & 0x0F) | ((value & 0x0F) << 4));
        g_fat_primary[index + 1] = (uint8_t)((value >> 4) & 0xFF);
    }
    g_fat_dirty = 1;
}

static uint16_t fat12_allocate_cluster(void) {
    for (uint16_t cluster = 2; cluster < g_fs.total_clusters + 2; cluster++) {
        if (fat12_get_fat_entry(cluster) == FAT12_CLUSTER_FREE) {
            fat12_set_fat_entry(cluster, FAT12_CLUSTER_EOC);
            fat12_memset(g_cluster_buffer, 0, g_fs.cluster_size_bytes);
            if (fat12_write_cluster(cluster, g_cluster_buffer) != 0) {
                fat12_set_fat_entry(cluster, FAT12_CLUSTER_FREE);
                return 0;
            }
            return cluster;
        }
    }
    return 0;
}

static void fat12_free_chain(uint16_t start_cluster) {
    uint16_t cluster = start_cluster;
    while (cluster >= 2 && cluster < FAT12_CLUSTER_EOC) {
        uint16_t next = fat12_get_fat_entry(cluster);
        fat12_set_fat_entry(cluster, FAT12_CLUSTER_FREE);
        cluster = next;
    }
}

static int fat12_flush_root(void) {
    if (!g_root_dirty) {
        return FAT12_OK;
    }
    for (uint16_t i = 0; i < g_fs.root_dir_sectors; i++) {
        int res = disk_write_sector(g_fs.base_lba + g_fs.root_dir_start_lba + i, g_root_dir + i * SECTOR_SIZE);
        if (res != 0) {
            return FAT12_ERR_IO;
        }
    }
    g_root_dirty = 0;
    return FAT12_OK;
}

static int fat12_flush_fats(void) {
    if (!g_fat_dirty) {
        return FAT12_OK;
    }
    for (uint8_t fat_index = 0; fat_index < g_fs.num_fats; fat_index++) {
        uint32_t start_lba = g_fs.base_lba + g_fs.fat_start_lba + (fat_index * g_fs.sectors_per_fat);
        for (uint16_t sector = 0; sector < g_fs.sectors_per_fat; sector++) {
            int res = disk_write_sector(start_lba + sector, g_fat_primary + sector * SECTOR_SIZE);
            if (res != 0) {
                return FAT12_ERR_IO;
            }
        }
    }
    g_fat_dirty = 0;
    return FAT12_OK;
}

static int fat12_is_free_entry(const fat12_raw_dir_entry_t *entry) {
    return (entry->name[0] == 0x00 || entry->name[0] == 0xE5);
}

static int fat12_is_dot_entry(const fat12_raw_dir_entry_t *entry) {
    if ((entry->attr & FAT12_ATTR_DIRECTORY) == 0) {
        return 0;
    }
    if (entry->name[0] != '.') {
        return 0;
    }
    if (entry->name[1] == ' ' && entry->name[2] == ' ') {
        return 1;
    }
    if (entry->name[1] == '.' && entry->name[2] == ' ') {
        return 1;
    }
    return 0;
}


static void fat12_fill_dir_entry(fat12_raw_dir_entry_t *dest, const uint8_t short_name[11], uint8_t attr, uint16_t first_cluster, uint32_t size) {
    fat12_memcpy(dest->name, short_name, 11);
    dest->attr = attr;
    dest->nt_reserved = 0;
    dest->creation_time_tenths = 0;
    dest->creation_time = 0;
    dest->creation_date = 0;
    dest->last_access_date = 0;
    dest->first_cluster_high = 0;
    dest->write_time = 0;
    dest->write_date = 0;
    dest->first_cluster_low = first_cluster;
    dest->file_size = size;
}

static void fat12_copy_entry_info(const fat12_raw_dir_entry_t *entry, fat12_dir_entry_info_t *info) {
    fat12_dir_name_to_string(entry->name, info->name);
    info->attr = entry->attr;
    info->size = entry->file_size;
    info->first_cluster = entry->first_cluster_low;
}

static int fat12_iterate_directory_internal(uint16_t dir_cluster, fat12_dir_iter_cb cb, void *context) {
    if (dir_cluster == 0) {
        /* Root directory */
        for (uint32_t i = 0; i < g_fs.root_entry_count; i++) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_root_dir + i * FAT12_DIR_ENTRY_SIZE);
            if (entry->name[0] == 0x00) {
                break;
            }
            if (entry->name[0] == 0xE5) {
                continue;
            }
            if (entry->attr == FAT12_ATTR_VOLUME_ID) {
                continue;
            }
            if (fat12_is_dot_entry(entry)) {
                continue;
            }
            fat12_dir_entry_info_t info;
            fat12_copy_entry_info(entry, &info);
            if (cb && cb(&info, context)) {
                return FAT12_OK;
            }
        }
        return FAT12_OK;
    }

    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < FAT12_CLUSTER_EOC) {
        if (fat12_read_cluster(cluster, g_cluster_buffer) != 0) {
            return FAT12_ERR_IO;
        }
        for (uint32_t offset = 0; offset < g_fs.cluster_size_bytes; offset += FAT12_DIR_ENTRY_SIZE) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_cluster_buffer + offset);
            if (entry->name[0] == 0x00) {
                return FAT12_OK;
            }
            if (entry->name[0] == 0xE5) {
                continue;
            }
            if (entry->attr == FAT12_ATTR_VOLUME_ID) {
                continue;
            }
            if (fat12_is_dot_entry(entry)) {
                continue;
            }
            fat12_dir_entry_info_t info;
            fat12_copy_entry_info(entry, &info);
            if (cb && cb(&info, context)) {
                return FAT12_OK;
            }
        }
        cluster = fat12_get_fat_entry(cluster);
    }

    return FAT12_OK;
}

static int fat12_find_entry(uint16_t dir_cluster, const uint8_t short_name[11], fat12_raw_dir_entry_t *out_entry, uint16_t *out_owner_cluster, uint16_t *out_entry_index) {
    if (dir_cluster == 0) {
        for (uint32_t i = 0; i < g_fs.root_entry_count; i++) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_root_dir + i * FAT12_DIR_ENTRY_SIZE);
            if (entry->name[0] == 0x00) {
                return FAT12_ERR_NOT_FOUND;
            }
            if (entry->name[0] == 0xE5) {
                continue;
            }
            if (fat12_memcmp(entry->name, short_name, 11) == 0) {
                if (out_entry) {
                    fat12_memcpy(out_entry, entry, sizeof(fat12_raw_dir_entry_t));
                }
                if (out_owner_cluster) {
                    *out_owner_cluster = 0;
                }
                if (out_entry_index) {
                    *out_entry_index = (uint16_t)i;
                }
                return FAT12_OK;
            }
        }
        return FAT12_ERR_NOT_FOUND;
    }

    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < FAT12_CLUSTER_EOC) {
        if (fat12_read_cluster(cluster, g_cluster_buffer) != 0) {
            return FAT12_ERR_IO;
        }
        for (uint32_t idx = 0; idx < g_fs.cluster_size_bytes / FAT12_DIR_ENTRY_SIZE; idx++) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_cluster_buffer + idx * FAT12_DIR_ENTRY_SIZE);
            if (entry->name[0] == 0x00) {
                return FAT12_ERR_NOT_FOUND;
            }
            if (entry->name[0] == 0xE5) {
                continue;
            }
            if (fat12_memcmp(entry->name, short_name, 11) == 0) {
                if (out_entry) {
                    fat12_memcpy(out_entry, entry, sizeof(fat12_raw_dir_entry_t));
                }
                if (out_owner_cluster) {
                    *out_owner_cluster = cluster;
                }
                if (out_entry_index) {
                    *out_entry_index = (uint16_t)idx;
                }
                return FAT12_OK;
            }
        }
        cluster = fat12_get_fat_entry(cluster);
    }

    return FAT12_ERR_NOT_FOUND;
}

static int fat12_write_entry(uint16_t owner_cluster, uint16_t entry_index, const fat12_raw_dir_entry_t *entry) {
    if (owner_cluster == 0) {
        if (entry_index >= g_fs.root_entry_count) {
            return FAT12_ERR_OUT_OF_RANGE;
        }
        fat12_memcpy(g_root_dir + entry_index * FAT12_DIR_ENTRY_SIZE, entry, sizeof(fat12_raw_dir_entry_t));
        g_root_dirty = 1;
        return FAT12_OK;
    }

    if (fat12_read_cluster(owner_cluster, g_cluster_buffer) != 0) {
        return FAT12_ERR_IO;
    }
    uint32_t max_entries = g_fs.cluster_size_bytes / FAT12_DIR_ENTRY_SIZE;
    if (entry_index >= max_entries) {
        return FAT12_ERR_OUT_OF_RANGE;
    }
    fat12_memcpy(g_cluster_buffer + entry_index * FAT12_DIR_ENTRY_SIZE, entry, sizeof(fat12_raw_dir_entry_t));
    if (fat12_write_cluster(owner_cluster, g_cluster_buffer) != 0) {
        return FAT12_ERR_IO;
    }
    return FAT12_OK;
}

static int fat12_find_free_entry(uint16_t dir_cluster, uint16_t *out_owner_cluster, uint16_t *out_entry_index) {
    if (dir_cluster == 0) {
        for (uint32_t i = 0; i < g_fs.root_entry_count; i++) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_root_dir + i * FAT12_DIR_ENTRY_SIZE);
            if (fat12_is_free_entry(entry)) {
                if (out_owner_cluster) {
                    *out_owner_cluster = 0;
                }
                if (out_entry_index) {
                    *out_entry_index = (uint16_t)i;
                }
                return FAT12_OK;
            }
        }
        return FAT12_ERR_DIR_FULL;
    }

    uint16_t cluster = dir_cluster;
    uint16_t previous_cluster = 0;
    while (cluster >= 2 && cluster < FAT12_CLUSTER_EOC) {
        if (fat12_read_cluster(cluster, g_cluster_buffer) != 0) {
            return FAT12_ERR_IO;
        }
        for (uint32_t idx = 0; idx < g_fs.cluster_size_bytes / FAT12_DIR_ENTRY_SIZE; idx++) {
            fat12_raw_dir_entry_t *entry = (fat12_raw_dir_entry_t *)(g_cluster_buffer + idx * FAT12_DIR_ENTRY_SIZE);
            if (fat12_is_free_entry(entry)) {
                if (out_owner_cluster) {
                    *out_owner_cluster = cluster;
                }
                if (out_entry_index) {
                    *out_entry_index = (uint16_t)idx;
                }
                return FAT12_OK;
            }
        }
        previous_cluster = cluster;
        cluster = fat12_get_fat_entry(cluster);
    }

    uint16_t new_cluster = fat12_allocate_cluster();
    if (!new_cluster) {
        return FAT12_ERR_NO_FREE_CLUSTER;
    }
    if (previous_cluster) {
        fat12_set_fat_entry(previous_cluster, new_cluster);
        fat12_set_fat_entry(new_cluster, FAT12_CLUSTER_EOC);
    }
    if (out_owner_cluster) {
        *out_owner_cluster = new_cluster;
    }
    if (out_entry_index) {
        *out_entry_index = 0;
    }
    return FAT12_OK;
}

static int fat12_path_next(const char **path_ptr, char *component, int max_len) {
    const char *ptr = *path_ptr;
    while (*ptr == '/' || *ptr == '\\') {
        ptr++;
    }
    if (*ptr == '\0') {
        *path_ptr = ptr;
        component[0] = '\0';
        return 0;
    }
    int len = 0;
    while (*ptr && *ptr != '/' && *ptr != '\\') {
        if (len < max_len - 1) {
            component[len++] = *ptr;
        }
        ptr++;
    }
    component[len] = '\0';
    *path_ptr = ptr;
    return len;
}



int fat12_init(uint32_t base_lba) {
    uint8_t sector[SECTOR_SIZE];
    if (disk_read_sector(base_lba, sector) != 0) {
        return FAT12_ERR_IO;
    }

    fat12_bpb_t *bpb = (fat12_bpb_t *)sector;
    if (bpb->bytes_per_sector != SECTOR_SIZE) {
        return FAT12_ERR_BAD_BPB;
    }

    g_fs.bytes_per_sector = bpb->bytes_per_sector;
    g_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    g_fs.reserved_sectors = bpb->reserved_sectors;
    g_fs.num_fats = bpb->num_fats;
    g_fs.root_entry_count = bpb->root_entry_count;
    g_fs.sectors_per_fat = bpb->sectors_per_fat_16;
    g_fs.total_sectors = (bpb->total_sectors_16 != 0) ? bpb->total_sectors_16 : bpb->total_sectors_32;
    g_fs.base_lba = base_lba;

    if (g_fs.sectors_per_cluster == 0 || g_fs.sectors_per_cluster > FAT12_MAX_SECTORS_PER_CLUSTER) {
        return FAT12_ERR_NOT_FAT12;
    }
    if (g_fs.sectors_per_fat == 0 || g_fs.sectors_per_fat > FAT12_MAX_FAT_SECTORS) {
        return FAT12_ERR_NOT_FAT12;
    }

    g_fs.fat_size_bytes = g_fs.sectors_per_fat * SECTOR_SIZE;
    g_fs.root_dir_sectors = (uint16_t)(((g_fs.root_entry_count * FAT12_DIR_ENTRY_SIZE) + (SECTOR_SIZE - 1)) / SECTOR_SIZE);
    if (g_fs.root_dir_sectors > FAT12_MAX_ROOT_DIR_SECTORS) {
        return FAT12_ERR_NOT_FAT12;
    }

    g_fs.fat_start_lba = g_fs.reserved_sectors;
    g_fs.root_dir_start_lba = g_fs.fat_start_lba + (g_fs.num_fats * g_fs.sectors_per_fat);
    g_fs.data_start_lba = g_fs.root_dir_start_lba + g_fs.root_dir_sectors;
    g_fs.total_data_sectors = g_fs.total_sectors - g_fs.reserved_sectors - (g_fs.num_fats * g_fs.sectors_per_fat) - g_fs.root_dir_sectors;
    g_fs.total_clusters = g_fs.total_data_sectors / g_fs.sectors_per_cluster;
    g_fs.cluster_size_bytes = g_fs.sectors_per_cluster * SECTOR_SIZE;

    if (g_fs.total_clusters < 1 || g_fs.total_clusters > 4084) {
        return FAT12_ERR_NOT_FAT12;
    }

    for (uint16_t sector_index = 0; sector_index < g_fs.sectors_per_fat; sector_index++) {
        uint32_t lba = base_lba + g_fs.fat_start_lba + sector_index;
        if (disk_read_sector(lba, g_fat_primary + sector_index * SECTOR_SIZE) != 0) {
            return FAT12_ERR_IO;
        }
    }

    if (g_fs.num_fats > 1) {
        for (uint16_t sector_index = 0; sector_index < g_fs.sectors_per_fat; sector_index++) {
            uint32_t lba = base_lba + g_fs.fat_start_lba + g_fs.sectors_per_fat + sector_index;
            if (disk_read_sector(lba, g_fat_secondary + sector_index * SECTOR_SIZE) != 0) {
                return FAT12_ERR_IO;
            }
        }
    }

    for (uint16_t sector_index = 0; sector_index < g_fs.root_dir_sectors; sector_index++) {
        uint32_t lba = base_lba + g_fs.root_dir_start_lba + sector_index;
        if (disk_read_sector(lba, g_root_dir + sector_index * SECTOR_SIZE) != 0) {
            return FAT12_ERR_IO;
        }
    }

    g_current_dir_cluster = 0;
    g_path_depth = 0;
    g_cwd[0] = '/';
    g_cwd[1] = '\0';

    g_fs_ready = 1;
    g_fat_dirty = 0;
    g_root_dirty = 0;
    return FAT12_OK;
}

int fat12_iterate_current_directory(fat12_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    return fat12_iterate_directory_internal(g_current_dir_cluster, cb, context);
}

static int fat12_locate_directory(const char *path, uint16_t *out_cluster) {
    if (!path || !*path) {
        if (out_cluster) {
            *out_cluster = g_current_dir_cluster;
        }
        return FAT12_OK;
    }

    uint16_t cluster;
    uint16_t temp_stack[FAT12_MAX_PATH_DEPTH];
    int temp_depth;

    if (path[0] == '/' || path[0] == '\\') {
        cluster = 0;
        temp_depth = 0;
    } else {
        cluster = g_current_dir_cluster;
        temp_depth = g_path_depth;
        for (int i = 0; i < temp_depth; i++) {
            temp_stack[i] = g_path_stack[i];
        }
    }

    const char *cursor = path;
    char component[32];
    while (fat12_path_next(&cursor, component, sizeof(component)) > 0) {
        while (*cursor == '/' || *cursor == '\\') {
            cursor++;
        }
        if (component[0] == '.' && component[1] == '\0') {
            continue;
        }
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0') {
            if (temp_depth > 0) {
                temp_depth--;
                cluster = (temp_depth == 0) ? 0 : temp_stack[temp_depth - 1];
            } else {
                cluster = 0;
            }
            continue;
        }
        uint8_t short_name[11];
        int res = fat12_make_short_name(component, short_name);
        if (res != FAT12_OK) {
            return res;
        }
        fat12_raw_dir_entry_t entry;
        res = fat12_find_entry(cluster, short_name, &entry, 0, 0);
        if (res != FAT12_OK) {
            return res;
        }
        if ((entry.attr & FAT12_ATTR_DIRECTORY) == 0) {
            return FAT12_ERR_NOT_DIRECTORY;
        }
        cluster = entry.first_cluster_low;
        if (temp_depth < FAT12_MAX_PATH_DEPTH) {
            temp_stack[temp_depth++] = cluster;
        }
    }

    if (out_cluster) {
        *out_cluster = cluster;
    }
    return FAT12_OK;
}

int fat12_iterate_path(const char *path, fat12_dir_iter_cb cb, void *context) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    if (!path || !*path) {
        return fat12_iterate_current_directory(cb, context);
    }

    uint16_t cluster;
    int res = fat12_locate_directory(path, &cluster);
    if (res != FAT12_OK) {
        return res;
    }
    return fat12_iterate_directory_internal(cluster, cb, context);
}

int fat12_change_directory(const char *path) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    if (!path || !*path) {
        return FAT12_OK;
    }

    uint16_t new_cluster = (path[0] == '/' || path[0] == '\\') ? 0 : g_current_dir_cluster;
    uint16_t temp_stack[FAT12_MAX_PATH_DEPTH];
    char temp_names[FAT12_MAX_PATH_DEPTH][FAT12_MAX_DISPLAY_NAME];
    int temp_depth = (path[0] == '/' || path[0] == '\\') ? 0 : g_path_depth;
    for (int i = 0; i < temp_depth; i++) {
        temp_stack[i] = g_path_stack[i];
        fat12_memcpy(temp_names[i], g_path_names[i], FAT12_MAX_DISPLAY_NAME);
    }

    const char *cursor = path;
    char component[32];
    while (fat12_path_next(&cursor, component, sizeof(component)) > 0) {
        while (*cursor == '/' || *cursor == '\\') {
            cursor++;
        }

        if (component[0] == '.' && component[1] == '\0') {
            continue;
        }
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0') {
            if (temp_depth > 0) {
                temp_depth--;
                new_cluster = (temp_depth == 0) ? 0 : temp_stack[temp_depth - 1];
            } else {
                new_cluster = 0;
            }
            continue;
        }

        uint8_t short_name[11];
        int res = fat12_make_short_name(component, short_name);
        if (res != FAT12_OK) {
            return res;
        }
        fat12_raw_dir_entry_t entry;
        res = fat12_find_entry(new_cluster, short_name, &entry, 0, 0);
        if (res != FAT12_OK) {
            return res;
        }
        if ((entry.attr & FAT12_ATTR_DIRECTORY) == 0) {
            return FAT12_ERR_NOT_DIRECTORY;
        }
        new_cluster = entry.first_cluster_low;
        if (temp_depth < FAT12_MAX_PATH_DEPTH) {
            temp_stack[temp_depth] = new_cluster;
            fat12_dir_name_to_string(entry.name, temp_names[temp_depth]);
            temp_depth++;
        }
    }

    g_current_dir_cluster = new_cluster;
    g_path_depth = temp_depth;
    for (int i = 0; i < temp_depth; i++) {
        g_path_stack[i] = temp_stack[i];
        fat12_memcpy(g_path_names[i], temp_names[i], FAT12_MAX_DISPLAY_NAME);
    }
    fat12_rebuild_cwd();
    return FAT12_OK;
}

const char *fat12_get_cwd(void) {
    return g_cwd;
}

static int fat12_resolve_parent_and_name(const char *path, uint16_t *out_dir_cluster, uint8_t short_name[11]) {
    if (!path || !*path) {
        return FAT12_ERR_INVALID_NAME;
    }

    uint16_t cluster;
    uint16_t temp_stack[FAT12_MAX_PATH_DEPTH];
    int temp_depth;

    if (path[0] == '/' || path[0] == '\\') {
        cluster = 0;
        temp_depth = 0;
    } else {
        cluster = g_current_dir_cluster;
        temp_depth = g_path_depth;
        for (int i = 0; i < temp_depth; i++) {
            temp_stack[i] = g_path_stack[i];
        }
    }

    const char *cursor = path;
    char component[32];
    char last_component[32];
    last_component[0] = '\0';

    while (fat12_path_next(&cursor, component, sizeof(component)) > 0) {
        while (*cursor == '/' || *cursor == '\\') {
            cursor++;
        }
        if (*cursor == '\0') {
            fat12_memcpy(last_component, component, sizeof(last_component));
            break;
        }

        if (component[0] == '.' && component[1] == '\0') {
            continue;
        }
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0') {
            if (temp_depth > 0) {
                temp_depth--;
                cluster = (temp_depth == 0) ? 0 : temp_stack[temp_depth - 1];
            } else {
                cluster = 0;
            }
            continue;
        }

        uint8_t temp_short[11];
        int res = fat12_make_short_name(component, temp_short);
        if (res != FAT12_OK) {
            return res;
        }
        fat12_raw_dir_entry_t entry;
        res = fat12_find_entry(cluster, temp_short, &entry, 0, 0);
        if (res != FAT12_OK) {
            return res;
        }
        if ((entry.attr & FAT12_ATTR_DIRECTORY) == 0) {
            return FAT12_ERR_NOT_DIRECTORY;
        }
        cluster = entry.first_cluster_low;
        if (temp_depth < FAT12_MAX_PATH_DEPTH) {
            temp_stack[temp_depth++] = cluster;
        }
    }

    if (last_component[0] == '\0') {
        return FAT12_ERR_INVALID_NAME;
    }
    if ((last_component[0] == '.' && last_component[1] == '\0') ||
        (last_component[0] == '.' && last_component[1] == '.' && last_component[2] == '\0')) {
        return FAT12_ERR_INVALID_NAME;
    }

    int res = fat12_make_short_name(last_component, short_name);
    if (res != FAT12_OK) {
        return res;
    }
    if (out_dir_cluster) {
        *out_dir_cluster = cluster;
    }
    return FAT12_OK;
}

int fat12_read_file(const char *path, uint8_t *buffer, uint32_t max_size, uint32_t *out_size) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    if (!buffer) {
        return FAT12_ERR_INVALID_NAME;
    }

    uint16_t dir_cluster;
    uint8_t short_name[11];
    int res = fat12_resolve_parent_and_name(path, &dir_cluster, short_name);
    if (res != FAT12_OK) {
        return res;
    }

    fat12_raw_dir_entry_t entry;
    res = fat12_find_entry(dir_cluster, short_name, &entry, 0, 0);
    if (res != FAT12_OK) {
        return res;
    }
    if (entry.attr & FAT12_ATTR_DIRECTORY) {
        return FAT12_ERR_NOT_FILE;
    }

    if (entry.file_size > max_size) {
        return FAT12_ERR_BUFFER_SMALL;
    }

    uint32_t bytes_remaining = entry.file_size;
    uint32_t cursor = 0;
    uint16_t cluster = entry.first_cluster_low;

    while (bytes_remaining > 0 && cluster >= 2 && cluster < FAT12_CLUSTER_EOC) {
        if (fat12_read_cluster(cluster, g_cluster_buffer) != 0) {
            return FAT12_ERR_IO;
        }
        uint32_t to_copy = g_fs.cluster_size_bytes;
        if (to_copy > bytes_remaining) {
            to_copy = bytes_remaining;
        }
        fat12_memcpy(buffer + cursor, g_cluster_buffer, to_copy);
        cursor += to_copy;
        bytes_remaining -= to_copy;
        cluster = fat12_get_fat_entry(cluster);
    }

    if (out_size) {
        *out_size = entry.file_size;
    }
    return FAT12_OK;
}

static int fat12_write_directory_entry(uint16_t dir_cluster, const uint8_t short_name[11], const fat12_raw_dir_entry_t *entry_template) {
    fat12_raw_dir_entry_t entry;
    uint16_t owner_cluster;
    uint16_t entry_index;
    int res = fat12_find_entry(dir_cluster, short_name, &entry, &owner_cluster, &entry_index);
    if (res == FAT12_OK) {
        return fat12_write_entry(owner_cluster, entry_index, entry_template);
    }
    if (res != FAT12_ERR_NOT_FOUND) {
        return res;
    }
    res = fat12_find_free_entry(dir_cluster, &owner_cluster, &entry_index);
    if (res != FAT12_OK) {
        return res;
    }
    return fat12_write_entry(owner_cluster, entry_index, entry_template);
}

static int fat12_mark_entry_deleted(uint16_t dir_cluster, const uint8_t short_name[11]) {
    fat12_raw_dir_entry_t entry;
    uint16_t owner_cluster;
    uint16_t entry_index;
    int res = fat12_find_entry(dir_cluster, short_name, &entry, &owner_cluster, &entry_index);
    if (res != FAT12_OK) {
        return res;
    }
    entry.name[0] = 0xE5;
    entry.file_size = 0;
    entry.first_cluster_low = 0;
    return fat12_write_entry(owner_cluster, entry_index, &entry);
}

int fat12_write_file(const char *name, const uint8_t *data, uint32_t size) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    if (!data && size > 0) {
        return FAT12_ERR_INVALID_NAME;
    }

    uint16_t dir_cluster;
    uint8_t short_name[11];
    int res = fat12_resolve_parent_and_name(name, &dir_cluster, short_name);
    if (res != FAT12_OK) {
        return res;
    }

    fat12_raw_dir_entry_t existing;
    uint16_t old_cluster = 0;
    int has_existing = 0;
    res = fat12_find_entry(dir_cluster, short_name, &existing, 0, 0);
    if (res == FAT12_OK) {
        if (existing.attr & FAT12_ATTR_DIRECTORY) {
            return FAT12_ERR_ALREADY_EXISTS;
        }
        has_existing = 1;
        old_cluster = existing.first_cluster_low;
    } else if (res != FAT12_ERR_NOT_FOUND) {
        return res;
    }

    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;
    uint32_t bytes_written = 0;

    if (size > 0) {
        while (bytes_written < size) {
            uint16_t new_cluster = fat12_allocate_cluster();
            if (!new_cluster) {
                if (first_cluster >= 2) {
                    fat12_free_chain(first_cluster);
                }
                return FAT12_ERR_NO_FREE_CLUSTER;
            }
            if (first_cluster == 0) {
                first_cluster = new_cluster;
            }
            if (prev_cluster >= 2) {
                fat12_set_fat_entry(prev_cluster, new_cluster);
            }
            fat12_set_fat_entry(new_cluster, FAT12_CLUSTER_EOC);

            uint32_t to_copy = g_fs.cluster_size_bytes;
            if (to_copy > (size - bytes_written)) {
                to_copy = size - bytes_written;
            }
            fat12_memset(g_cluster_buffer, 0, g_fs.cluster_size_bytes);
            fat12_memcpy(g_cluster_buffer, data + bytes_written, to_copy);
            if (fat12_write_cluster(new_cluster, g_cluster_buffer) != 0) {
                fat12_free_chain(first_cluster);
                return FAT12_ERR_IO;
            }
            bytes_written += to_copy;
            prev_cluster = new_cluster;
        }
    }

    fat12_raw_dir_entry_t new_entry;
    fat12_fill_dir_entry(&new_entry, short_name, FAT12_ATTR_ARCHIVE, first_cluster, size);
    res = fat12_write_directory_entry(dir_cluster, short_name, &new_entry);
    if (res != FAT12_OK) {
        if (first_cluster >= 2) {
            fat12_free_chain(first_cluster);
        }
        return res;
    }

    if (has_existing && old_cluster >= 2 && old_cluster != first_cluster) {
        fat12_free_chain(old_cluster);
    }

    fat12_flush_root();
    fat12_flush_fats();
    return FAT12_OK;
}

int fat12_create_directory(const char *name) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }

    uint16_t dir_cluster;
    uint8_t short_name[11];
    int res = fat12_resolve_parent_and_name(name, &dir_cluster, short_name);
    if (res != FAT12_OK) {
        return res;
    }

    fat12_raw_dir_entry_t existing;
    res = fat12_find_entry(dir_cluster, short_name, &existing, 0, 0);
    if (res == FAT12_OK) {
        return FAT12_ERR_ALREADY_EXISTS;
    }

    uint16_t new_cluster = fat12_allocate_cluster();
    if (!new_cluster) {
        return FAT12_ERR_NO_FREE_CLUSTER;
    }

    fat12_memset(g_cluster_buffer, 0, g_fs.cluster_size_bytes);
    fat12_raw_dir_entry_t *dot = (fat12_raw_dir_entry_t *)g_cluster_buffer;
    fat12_raw_dir_entry_t *dotdot = (fat12_raw_dir_entry_t *)(g_cluster_buffer + FAT12_DIR_ENTRY_SIZE);

    uint8_t dot_name[11] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    dot_name[0] = '.';
    fat12_fill_dir_entry(dot, dot_name, FAT12_ATTR_DIRECTORY, new_cluster, 0);

    uint8_t dotdot_name[11] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    dotdot_name[0] = '.';
    dotdot_name[1] = '.';
    uint16_t parent_cluster = (dir_cluster == 0) ? 0 : dir_cluster;
    fat12_fill_dir_entry(dotdot, dotdot_name, FAT12_ATTR_DIRECTORY, parent_cluster, 0);

    if (fat12_write_cluster(new_cluster, g_cluster_buffer) != 0) {
        fat12_free_chain(new_cluster);
        return FAT12_ERR_IO;
    }

    fat12_raw_dir_entry_t new_entry;
    fat12_fill_dir_entry(&new_entry, short_name, FAT12_ATTR_DIRECTORY, new_cluster, 0);
    res = fat12_write_directory_entry(dir_cluster, short_name, &new_entry);
    if (res != FAT12_OK) {
        fat12_free_chain(new_cluster);
        return res;
    }

    fat12_flush_root();
    fat12_flush_fats();
    return FAT12_OK;
}

int fat12_delete_file(const char *name) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }

    uint16_t dir_cluster;
    uint8_t short_name[11];
    int res = fat12_resolve_parent_and_name(name, &dir_cluster, short_name);
    if (res != FAT12_OK) {
        return res;
    }

    fat12_raw_dir_entry_t entry;
    res = fat12_find_entry(dir_cluster, short_name, &entry, 0, 0);
    if (res != FAT12_OK) {
        return res;
    }
    if (entry.attr & FAT12_ATTR_DIRECTORY) {
        return FAT12_ERR_NOT_FILE;
    }

    if (entry.first_cluster_low >= 2) {
        fat12_free_chain(entry.first_cluster_low);
    }

    res = fat12_mark_entry_deleted(dir_cluster, short_name);
    if (res != FAT12_OK) {
        return res;
    }
    fat12_flush_root();
    fat12_flush_fats();
    return FAT12_OK;
}

int fat12_flush(void) {
    if (!g_fs_ready) {
        return FAT12_ERR_NOT_INITIALIZED;
    }
    int res = fat12_flush_root();
    if (res != FAT12_OK) {
        return res;
    }
    return fat12_flush_fats();
}
