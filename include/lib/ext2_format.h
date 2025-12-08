#ifndef LIB_EXT2_FORMAT_H
#define LIB_EXT2_FORMAT_H

#include <stdint.h>

int ext2_format(uint32_t partition_lba, uint32_t partition_size_sectors, const char *volume_label);

#endif
