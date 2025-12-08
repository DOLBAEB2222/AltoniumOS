#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* Block device type enumeration */
typedef enum {
    BLOCK_DEVICE_UNKNOWN = 0,
    BLOCK_DEVICE_ATA,
    BLOCK_DEVICE_AHCI,
    BLOCK_DEVICE_NVME,
} block_device_type_t;

/* Forward declaration */
typedef struct block_device block_device_t;

/* Block device operations */
typedef struct {
    int (*read)(block_device_t *dev, uint32_t lba, uint8_t *buffer, uint16_t num_sectors);
    int (*write)(block_device_t *dev, uint32_t lba, const uint8_t *buffer, uint16_t num_sectors);
} block_device_ops_t;

/* Block device structure */
typedef struct block_device {
    block_device_type_t type;
    uint32_t sector_size;
    uint32_t capacity_sectors;
    const char *driver_name;
    uint32_t queue_depth;
    block_device_ops_t ops;
    void *private_data;
} block_device_t;

/* Storage manager API */
int storage_manager_init(void);
block_device_t *storage_get_device(int index);
int storage_get_device_count(void);
block_device_t *storage_get_primary_device(void);

#endif /* BLOCK_DEVICE_H */
