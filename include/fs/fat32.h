#ifndef FAT32_H
#define FAT32_H

#include "../../disk.h"
#include "vfs.h"

vfs_operations_t fat32_get_vfs_ops(void);

#endif /* FAT32_H */
