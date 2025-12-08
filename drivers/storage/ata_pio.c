#include "../../include/drivers/storage/block_device.h"
#include "../../disk.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* ATA PIO device private data */
typedef struct {
    uint32_t total_sectors;
} ata_pio_private_t;

/* ATA PIO read callback */
static int ata_pio_read(block_device_t *dev, uint32_t lba, uint8_t *buffer, uint16_t num_sectors) {
    if (num_sectors == 0) {
        return 0;
    }
    
    if (num_sectors == 1) {
        return disk_read_sector(lba, buffer);
    } else {
        return disk_read_sectors(lba, buffer, num_sectors);
    }
}

/* ATA PIO write callback */
static int ata_pio_write(block_device_t *dev, uint32_t lba, const uint8_t *buffer, uint16_t num_sectors) {
    if (num_sectors == 0) {
        return 0;
    }
    
    if (num_sectors == 1) {
        return disk_write_sector(lba, buffer);
    } else {
        return disk_write_sectors(lba, buffer, num_sectors);
    }
}

int ata_pio_init(block_device_t *dev) {
    /* Initialize the legacy disk driver */
    if (disk_init() != 0) {
        return -1;
    }
    
    /* Set up the block device structure */
    dev->type = BLOCK_DEVICE_ATA;
    dev->sector_size = SECTOR_SIZE;
    dev->capacity_sectors = 0;
    dev->driver_name = "ATA PIO";
    dev->queue_depth = 1;
    dev->ops.read = ata_pio_read;
    dev->ops.write = ata_pio_write;
    dev->private_data = 0;
    
    return 0;
}
