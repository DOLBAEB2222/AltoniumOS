#include "../include/lib/partition_table.h"
#include "../include/lib/string.h"
#include "../disk.h"

static uint8_t sector_buffer[512];

int partition_read_mbr(uint32_t disk_lba, mbr_t *mbr) {
    if (disk_read_sectors(disk_lba, 1, (uint8_t *)mbr) != 0) {
        return -1;
    }
    
    if (mbr->signature != 0xAA55) {
        return -2;
    }
    
    return 0;
}

int partition_write_mbr(uint32_t disk_lba, const mbr_t *mbr) {
    if (disk_write_sectors(disk_lba, 1, (const uint8_t *)mbr) != 0) {
        return -1;
    }
    return 0;
}

int partition_list(uint32_t disk_lba, partition_info_t *partitions, int max_count, int *out_count) {
    mbr_t mbr;
    if (partition_read_mbr(disk_lba, &mbr) != 0) {
        return -1;
    }
    
    int count = 0;
    for (int i = 0; i < 4 && count < max_count; i++) {
        if (mbr.partitions[i].type != PARTITION_TYPE_EMPTY) {
            partitions[count].type = mbr.partitions[i].type;
            partitions[count].first_lba = mbr.partitions[i].first_lba;
            partitions[count].sector_count = mbr.partitions[i].sector_count;
            partitions[count].active = (mbr.partitions[i].status == 0x80) ? 1 : 0;
            count++;
        }
    }
    
    *out_count = count;
    return 0;
}

int partition_create_mbr(uint32_t disk_lba, int partition_index, uint32_t start_lba, uint32_t size_sectors, uint8_t type) {
    if (partition_index < 0 || partition_index >= 4) {
        return -1;
    }
    
    mbr_t mbr;
    if (partition_read_mbr(disk_lba, &mbr) != 0) {
        string_memset((uint8_t *)&mbr, 0, sizeof(mbr_t));
        mbr.signature = 0xAA55;
    }
    
    mbr.partitions[partition_index].status = (partition_index == 0) ? 0x80 : 0x00;
    mbr.partitions[partition_index].type = type;
    mbr.partitions[partition_index].first_lba = start_lba;
    mbr.partitions[partition_index].sector_count = size_sectors;
    
    mbr.partitions[partition_index].first_chs[0] = 0x00;
    mbr.partitions[partition_index].first_chs[1] = 0x02;
    mbr.partitions[partition_index].first_chs[2] = 0x00;
    mbr.partitions[partition_index].last_chs[0] = 0xFF;
    mbr.partitions[partition_index].last_chs[1] = 0xFF;
    mbr.partitions[partition_index].last_chs[2] = 0xFF;
    
    return partition_write_mbr(disk_lba, &mbr);
}

int partition_delete_mbr(uint32_t disk_lba, int partition_index) {
    if (partition_index < 0 || partition_index >= 4) {
        return -1;
    }
    
    mbr_t mbr;
    if (partition_read_mbr(disk_lba, &mbr) != 0) {
        return -2;
    }
    
    string_memset((uint8_t *)&mbr.partitions[partition_index], 0, sizeof(mbr_partition_entry_t));
    
    return partition_write_mbr(disk_lba, &mbr);
}

int partition_get_free_space(uint32_t disk_lba, uint32_t *out_start_lba, uint32_t *out_size_sectors) {
    mbr_t mbr;
    if (partition_read_mbr(disk_lba, &mbr) != 0) {
        *out_start_lba = 2048;
        *out_size_sectors = 0x100000;
        return 0;
    }
    
    uint32_t next_free = 2048;
    
    for (int i = 0; i < 4; i++) {
        if (mbr.partitions[i].type != PARTITION_TYPE_EMPTY) {
            uint32_t part_end = mbr.partitions[i].first_lba + mbr.partitions[i].sector_count;
            if (part_end > next_free) {
                next_free = part_end;
            }
        }
    }
    
    *out_start_lba = next_free;
    *out_size_sectors = 0x100000;
    
    return 0;
}

int partition_init_gpt_stub(uint32_t disk_lba, uint64_t disk_size_sectors) {
    (void)disk_lba;
    (void)disk_size_sectors;
    return -1;
}

const char *partition_type_name(uint8_t type) {
    switch (type) {
        case PARTITION_TYPE_EMPTY: return "Empty";
        case PARTITION_TYPE_FAT12: return "FAT12";
        case PARTITION_TYPE_FAT16: return "FAT16";
        case PARTITION_TYPE_FAT32: return "FAT32";
        case PARTITION_TYPE_FAT32_LBA: return "FAT32 LBA";
        case PARTITION_TYPE_LINUX: return "Linux";
        case PARTITION_TYPE_EXTENDED: return "Extended";
        case PARTITION_TYPE_GPT: return "GPT Protective";
        default: return "Unknown";
    }
}
