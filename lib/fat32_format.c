#include "../include/lib/fat32_format.h"
#include "../include/lib/string.h"
#include "../disk.h"

typedef struct {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

int fat32_format(uint32_t partition_lba, uint32_t partition_size_sectors, const char *volume_label) {
    fat32_boot_sector_t boot_sector;
    string_memset((uint8_t *)&boot_sector, 0, sizeof(fat32_boot_sector_t));
    
    boot_sector.jump[0] = 0xEB;
    boot_sector.jump[1] = 0x58;
    boot_sector.jump[2] = 0x90;
    
    string_copy(boot_sector.oem_name, "ALTONIUM");
    
    boot_sector.bytes_per_sector = 512;
    boot_sector.sectors_per_cluster = 8;
    boot_sector.reserved_sectors = 32;
    boot_sector.num_fats = 2;
    boot_sector.root_entries = 0;
    boot_sector.total_sectors_16 = 0;
    boot_sector.media_descriptor = 0xF8;
    boot_sector.sectors_per_fat_16 = 0;
    boot_sector.sectors_per_track = 63;
    boot_sector.num_heads = 16;
    boot_sector.hidden_sectors = 0;
    boot_sector.total_sectors_32 = partition_size_sectors;
    
    uint32_t clusters = (partition_size_sectors - 32) / 8;
    uint32_t fat_size_sectors = (clusters * 4 + 511) / 512;
    
    boot_sector.sectors_per_fat_32 = fat_size_sectors;
    boot_sector.ext_flags = 0;
    boot_sector.fs_version = 0;
    boot_sector.root_cluster = 2;
    boot_sector.fs_info_sector = 1;
    boot_sector.backup_boot_sector = 6;
    
    boot_sector.drive_number = 0x80;
    boot_sector.boot_signature = 0x29;
    boot_sector.volume_id = 0x12345678;
    
    string_memset((uint8_t *)boot_sector.volume_label, ' ', 11);
    if (volume_label) {
        int len = string_length(volume_label);
        if (len > 11) len = 11;
        string_memcpy((uint8_t *)boot_sector.volume_label, (const uint8_t *)volume_label, len);
    }
    
    string_copy(boot_sector.fs_type, "FAT32   ");
    
    uint8_t sector[512];
    string_memset(sector, 0, 512);
    string_memcpy(sector, (uint8_t *)&boot_sector, sizeof(fat32_boot_sector_t));
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    if (disk_write_sectors(partition_lba, 1, sector) != 0) {
        return -1;
    }
    
    uint8_t fat_sector[512];
    string_memset(fat_sector, 0, 512);
    fat_sector[0] = 0xF8;
    fat_sector[1] = 0xFF;
    fat_sector[2] = 0xFF;
    fat_sector[3] = 0x0F;
    fat_sector[4] = 0xFF;
    fat_sector[5] = 0xFF;
    fat_sector[6] = 0xFF;
    fat_sector[7] = 0x0F;
    fat_sector[8] = 0xFF;
    fat_sector[9] = 0xFF;
    fat_sector[10] = 0xFF;
    fat_sector[11] = 0x0F;
    
    uint32_t fat1_start = partition_lba + 32;
    if (disk_write_sectors(fat1_start, 1, fat_sector) != 0) {
        return -2;
    }
    
    uint32_t fat2_start = fat1_start + fat_size_sectors;
    if (disk_write_sectors(fat2_start, 1, fat_sector) != 0) {
        return -3;
    }
    
    uint8_t empty_sector[512];
    string_memset(empty_sector, 0, 512);
    uint32_t root_dir_start = fat2_start + fat_size_sectors;
    for (int i = 0; i < 8; i++) {
        if (disk_write_sectors(root_dir_start + i, 1, empty_sector) != 0) {
            return -4;
        }
    }
    
    return 0;
}
