#ifndef LIB_PARTITION_TABLE_H
#define LIB_PARTITION_TABLE_H

#include <stdint.h>

#define PARTITION_TYPE_EMPTY     0x00
#define PARTITION_TYPE_FAT12     0x01
#define PARTITION_TYPE_FAT16     0x04
#define PARTITION_TYPE_FAT32     0x0B
#define PARTITION_TYPE_FAT32_LBA 0x0C
#define PARTITION_TYPE_LINUX     0x83
#define PARTITION_TYPE_EXTENDED  0x05
#define PARTITION_TYPE_GPT       0xEE

#define PARTITION_TABLE_MBR 0
#define PARTITION_TABLE_GPT 1

#define MAX_PARTITIONS 16

typedef struct {
    uint8_t status;
    uint8_t first_chs[3];
    uint8_t type;
    uint8_t last_chs[3];
    uint32_t first_lba;
    uint32_t sector_count;
} __attribute__((packed)) mbr_partition_entry_t;

typedef struct {
    uint8_t bootloader[446];
    mbr_partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_t;

typedef struct {
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_entries_crc32;
} __attribute__((packed)) gpt_header_t;

typedef struct {
    uint8_t type;
    uint32_t first_lba;
    uint32_t sector_count;
    int active;
} partition_info_t;

int partition_read_mbr(uint32_t disk_lba, mbr_t *mbr);
int partition_write_mbr(uint32_t disk_lba, const mbr_t *mbr);
int partition_list(uint32_t disk_lba, partition_info_t *partitions, int max_count, int *out_count);
int partition_create_mbr(uint32_t disk_lba, int partition_index, uint32_t start_lba, uint32_t size_sectors, uint8_t type);
int partition_delete_mbr(uint32_t disk_lba, int partition_index);
int partition_get_free_space(uint32_t disk_lba, uint32_t *out_start_lba, uint32_t *out_size_sectors);
int partition_init_gpt_stub(uint32_t disk_lba, uint64_t disk_size_sectors);
const char *partition_type_name(uint8_t type);

#endif
