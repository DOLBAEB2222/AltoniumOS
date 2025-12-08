#include "../include/lib/ext2_format.h"
#include "../include/lib/string.h"
#include "../disk.h"

#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_BLOCK_SIZE 1024
#define EXT2_INODE_SIZE 128

typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint8_t reserved[940];
} __attribute__((packed)) ext2_superblock_t;

typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} __attribute__((packed)) ext2_group_desc_t;

int ext2_format(uint32_t partition_lba, uint32_t partition_size_sectors, const char *volume_label) {
    (void)volume_label;
    
    uint32_t block_count = (partition_size_sectors * 512) / EXT2_BLOCK_SIZE;
    uint32_t inode_count = block_count / 4;
    
    ext2_superblock_t superblock;
    string_memset((uint8_t *)&superblock, 0, sizeof(ext2_superblock_t));
    
    superblock.s_inodes_count = inode_count;
    superblock.s_blocks_count = block_count;
    superblock.s_r_blocks_count = block_count / 20;
    superblock.s_free_blocks_count = block_count - 100;
    superblock.s_free_inodes_count = inode_count - 10;
    superblock.s_first_data_block = 1;
    superblock.s_log_block_size = 0;
    superblock.s_log_frag_size = 0;
    superblock.s_blocks_per_group = 8192;
    superblock.s_frags_per_group = 8192;
    superblock.s_inodes_per_group = inode_count;
    superblock.s_mtime = 0;
    superblock.s_wtime = 0;
    superblock.s_mnt_count = 0;
    superblock.s_max_mnt_count = 20;
    superblock.s_magic = EXT2_SUPER_MAGIC;
    superblock.s_state = 1;
    superblock.s_errors = 1;
    superblock.s_minor_rev_level = 0;
    superblock.s_lastcheck = 0;
    superblock.s_checkinterval = 0;
    superblock.s_creator_os = 0;
    superblock.s_rev_level = 0;
    superblock.s_def_resuid = 0;
    superblock.s_def_resgid = 0;
    
    uint8_t sector[512];
    string_memset(sector, 0, 512);
    
    if (disk_write_sectors(partition_lba, 1, sector) != 0) {
        return -1;
    }
    
    string_memcpy(sector, (uint8_t *)&superblock, sizeof(ext2_superblock_t) > 512 ? 512 : sizeof(ext2_superblock_t));
    if (disk_write_sectors(partition_lba + 2, 1, sector) != 0) {
        return -2;
    }
    
    ext2_group_desc_t group_desc;
    string_memset((uint8_t *)&group_desc, 0, sizeof(ext2_group_desc_t));
    group_desc.bg_block_bitmap = 3;
    group_desc.bg_inode_bitmap = 4;
    group_desc.bg_inode_table = 5;
    group_desc.bg_free_blocks_count = superblock.s_free_blocks_count;
    group_desc.bg_free_inodes_count = superblock.s_free_inodes_count;
    group_desc.bg_used_dirs_count = 1;
    
    string_memset(sector, 0, 512);
    string_memcpy(sector, (uint8_t *)&group_desc, sizeof(ext2_group_desc_t));
    if (disk_write_sectors(partition_lba + 4, 1, sector) != 0) {
        return -3;
    }
    
    return 0;
}
