#ifndef LIB_FAT32_FORMAT_H
#define LIB_FAT32_FORMAT_H

#include <stdint.h>

int fat32_format(uint32_t partition_lba, uint32_t partition_size_sectors, const char *volume_label);

#endif
