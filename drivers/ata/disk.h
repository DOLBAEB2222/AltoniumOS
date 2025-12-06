#ifndef DISK_H
#define DISK_H

#include <kernel/types.h>

/* ATA PIO Driver for Primary IDE Channel */

/* ATA Register Definitions (Primary IDE Channel) */
#define ATA_DATA_REG           0x1F0
#define ATA_ERROR_REG          0x1F1
#define ATA_FEATURES_REG       0x1F1
#define ATA_SECCOUNT0_REG      0x1F2
#define ATA_LBA0_REG           0x1F3
#define ATA_LBA1_REG           0x1F4
#define ATA_LBA2_REG           0x1F5
#define ATA_DRIVE_REG          0x1F6
#define ATA_COMMAND_REG        0x1F7
#define ATA_STATUS_REG         0x1F7
#define ATA_ALTSTATUS_REG      0x3F6
#define ATA_CONTROL_REG        0x3F6

/* ATA Commands */
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_IDENTIFY_DEVICE     0xEC

/* ATA Status Bits */
#define ATA_STATUS_BSY      0x80    /* Busy */
#define ATA_STATUS_DRDY     0x40    /* Drive Ready */
#define ATA_STATUS_DF       0x20    /* Device Fault */
#define ATA_STATUS_DRQ      0x08    /* Data Request */
#define ATA_STATUS_ERR      0x01    /* Error */

/* ATA Control Bits */
#define ATA_CONTROL_SRST    0x04    /* Software Reset */
#define ATA_CONTROL_NIEN    0x02    /* Interrupt Disable */

/* Drive/Head Register Bits */
#define ATA_DRIVE_LBA       0x40    /* LBA addressing mode */
#define ATA_DRIVE_MASTER    0xA0    /* Master drive */

/* Sector size */
#define SECTOR_SIZE 512

/* Function Prototypes */
int disk_init(void);
int disk_read_sector(uint32_t lba, uint8_t *buffer);
int disk_write_sector(uint32_t lba, const uint8_t *buffer);
int disk_read_sectors(uint32_t lba, uint8_t *buffer, uint16_t num_sectors);
int disk_write_sectors(uint32_t lba, const uint8_t *buffer, uint16_t num_sectors);
int disk_self_test(void);

#endif /* DISK_H */
