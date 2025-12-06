#include "disk.h"
#include "include/kernel/hw_detect.h"
#include "include/drivers/console.h"

/* Performance counters */
static uint32_t g_disk_reads = 0;
static uint32_t g_disk_writes = 0;
static uint32_t g_disk_multi_reads = 0;
static uint32_t g_disk_multi_writes = 0;
static uint32_t g_disk_read_sectors = 0;
static uint32_t g_disk_write_sectors = 0;

/* I/O Port Functions */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* Wait for drive to be ready (not busy) */
static int wait_for_bsy(void) {
    int timeout = 10000;  /* Timeout counter */
    
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS_REG);
        if ((status & ATA_STATUS_BSY) == 0) {
            return 0;  /* Success */
        }
    }
    
    return -1;  /* Timeout */
}

/* Wait for DRQ (Data Request) or error */
static int wait_for_drq(void) {
    int timeout = 10000;  /* Timeout counter */
    
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS_REG);
        if (status & ATA_STATUS_ERR) {
            return -1;  /* Error */
        }
        if (status & ATA_STATUS_DRQ) {
            return 0;  /* Data ready */
        }
    }
    
    return -1;  /* Timeout */
}

/* Select drive */
static void select_drive(void) {
    outb(ATA_DRIVE_REG, ATA_DRIVE_MASTER | ATA_DRIVE_LBA);
}

/* Initialize the ATA driver */
int disk_init(void) {
    /* Check if storage controller is available */
    if (!hw_has_storage_controller()) {
        console_print("Warning: No storage controller detected, skipping disk init\n");
        return -10;  /* Hardware not available */
    }
    
    /* Select master drive in LBA mode */
    select_drive();
    
    /* Wait for drive to be ready */
    if (wait_for_bsy() != 0) {
        return -1;  /* Drive is stuck in busy state */
    }
    
    /* Check if drive is ready */
    uint8_t status = inb(ATA_STATUS_REG);
    if ((status & ATA_STATUS_DRDY) == 0) {
        return -2;  /* Drive not ready */
    }
    
    /* Try to identify the device */
    outb(ATA_SECCOUNT0_REG, 0);
    outb(ATA_LBA0_REG, 0);
    outb(ATA_LBA1_REG, 0);
    outb(ATA_LBA2_REG, 0);
    outb(ATA_COMMAND_REG, ATA_CMD_IDENTIFY_DEVICE);
    
    /* Check if device exists */
    status = inb(ATA_STATUS_REG);
    if (status == 0) {
        return -3;  /* No device present */
    }
    
    /* Wait for identify command to complete */
    if (wait_for_drq() != 0) {
        return -4;  /* Identify command failed */
    }
    
    /* Read and discard identify data (256 words) */
    for (int i = 0; i < 256; i++) {
        inw(ATA_DATA_REG);
    }
    
    return 0;  /* Success */
}

/* Read a single sector */
int disk_read_sector(uint32_t lba, uint8_t *buffer) {
    if (!buffer) {
        return -1;
    }
    
    /* Select drive */
    select_drive();
    
    /* Wait for drive to be ready */
    if (wait_for_bsy() != 0) {
        return -2;
    }
    
    /* Set up LBA address and sector count */
    outb(ATA_SECCOUNT0_REG, 1);
    outb(ATA_LBA0_REG, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1_REG, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2_REG, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_DRIVE_REG, ATA_DRIVE_MASTER | ATA_DRIVE_LBA | ((lba >> 24) & 0x0F));
    
    /* Send read command */
    outb(ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);
    
    /* Wait for data request */
    if (wait_for_drq() != 0) {
        return -3;
    }
    
    /* Read sector data (256 words = 512 bytes) */
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_DATA_REG);
        buffer[i * 2] = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
    }
    
    g_disk_reads++;
    g_disk_read_sectors++;
    return 0;  /* Success */
}

/* Write a single sector */
int disk_write_sector(uint32_t lba, const uint8_t *buffer) {
    if (!buffer) {
        return -1;
    }
    
    /* Select drive */
    select_drive();
    
    /* Wait for drive to be ready */
    if (wait_for_bsy() != 0) {
        return -2;
    }
    
    /* Set up LBA address and sector count */
    outb(ATA_SECCOUNT0_REG, 1);
    outb(ATA_LBA0_REG, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1_REG, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2_REG, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_DRIVE_REG, ATA_DRIVE_MASTER | ATA_DRIVE_LBA | ((lba >> 24) & 0x0F));
    
    /* Send write command */
    outb(ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);
    
    /* Wait for data request */
    if (wait_for_drq() != 0) {
        return -3;
    }
    
    /* Write sector data (256 words = 512 bytes) */
    for (int i = 0; i < 256; i++) {
        uint16_t data = (uint16_t)buffer[i * 2] | ((uint16_t)buffer[i * 2 + 1] << 8);
        outw(ATA_DATA_REG, data);
    }
    
    /* Wait for write to complete */
    if (wait_for_bsy() != 0) {
        return -4;
    }
    
    /* Check for errors */
    uint8_t status = inb(ATA_STATUS_REG);
    if (status & ATA_STATUS_ERR) {
        return -5;
    }
    
    g_disk_writes++;
    g_disk_write_sectors++;
    return 0;  /* Success */
}

/* Read multiple sectors using multi-sector PIO (optimized) */
static int disk_read_sectors_multi_pio(uint32_t lba, uint8_t *buffer, uint16_t num_sectors) {
    if (!buffer || num_sectors == 0 || num_sectors > 256) {
        return -1;
    }
    
    select_drive();
    
    if (wait_for_bsy() != 0) {
        return -2;
    }
    
    outb(ATA_SECCOUNT0_REG, (uint8_t)num_sectors);
    outb(ATA_LBA0_REG, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1_REG, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2_REG, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_DRIVE_REG, ATA_DRIVE_MASTER | ATA_DRIVE_LBA | ((lba >> 24) & 0x0F));
    
    outb(ATA_COMMAND_REG, ATA_CMD_READ_SECTORS_MULTI);
    
    for (uint16_t sector = 0; sector < num_sectors; sector++) {
        if (wait_for_drq() != 0) {
            return -3;
        }
        
        uint8_t *sector_buffer = buffer + (sector * SECTOR_SIZE);
        for (int i = 0; i < 256; i++) {
            uint16_t data = inw(ATA_DATA_REG);
            sector_buffer[i * 2] = (uint8_t)(data & 0xFF);
            sector_buffer[i * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
        }
    }
    
    g_disk_multi_reads++;
    g_disk_reads++;
    g_disk_read_sectors += num_sectors;
    return 0;
}

/* Read multiple sectors */
int disk_read_sectors(uint32_t lba, uint8_t *buffer, uint16_t num_sectors) {
    if (!buffer || num_sectors == 0) {
        return -1;
    }
    
    if (num_sectors > 1 && num_sectors <= 256) {
        int result = disk_read_sectors_multi_pio(lba, buffer, num_sectors);
        if (result == 0) {
            return 0;
        }
    }
    
    for (uint16_t i = 0; i < num_sectors; i++) {
        int result = disk_read_sector(lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            return result;
        }
    }
    
    return 0;
}

/* Write multiple sectors using multi-sector PIO (optimized) */
static int disk_write_sectors_multi_pio(uint32_t lba, const uint8_t *buffer, uint16_t num_sectors) {
    if (!buffer || num_sectors == 0 || num_sectors > 256) {
        return -1;
    }
    
    select_drive();
    
    if (wait_for_bsy() != 0) {
        return -2;
    }
    
    outb(ATA_SECCOUNT0_REG, (uint8_t)num_sectors);
    outb(ATA_LBA0_REG, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1_REG, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2_REG, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_DRIVE_REG, ATA_DRIVE_MASTER | ATA_DRIVE_LBA | ((lba >> 24) & 0x0F));
    
    outb(ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS_MULTI);
    
    for (uint16_t sector = 0; sector < num_sectors; sector++) {
        if (wait_for_drq() != 0) {
            return -3;
        }
        
        const uint8_t *sector_buffer = buffer + (sector * SECTOR_SIZE);
        for (int i = 0; i < 256; i++) {
            uint16_t data = (uint16_t)sector_buffer[i * 2] | ((uint16_t)sector_buffer[i * 2 + 1] << 8);
            outw(ATA_DATA_REG, data);
        }
    }
    
    if (wait_for_bsy() != 0) {
        return -4;
    }
    
    uint8_t status = inb(ATA_STATUS_REG);
    if (status & ATA_STATUS_ERR) {
        return -5;
    }
    
    g_disk_multi_writes++;
    g_disk_writes++;
    g_disk_write_sectors += num_sectors;
    return 0;
}

/* Write multiple sectors */
int disk_write_sectors(uint32_t lba, const uint8_t *buffer, uint16_t num_sectors) {
    if (!buffer || num_sectors == 0) {
        return -1;
    }
    
    if (num_sectors > 1 && num_sectors <= 256) {
        int result = disk_write_sectors_multi_pio(lba, buffer, num_sectors);
        if (result == 0) {
            return 0;
        }
    }
    
    for (uint16_t i = 0; i < num_sectors; i++) {
        int result = disk_write_sector(lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            return result;
        }
    }
    
    return 0;
}

/* Simple self-test: read LBA 0 and validate it looks like a boot sector */
int disk_self_test(void) {
    uint8_t buffer[SECTOR_SIZE];
    
    /* Read the first sector (MBR/boot sector) */
    int result = disk_read_sector(0, buffer);
    if (result != 0) {
        return result;
    }
    
    /* Check for boot signature (0x55AA at offset 510) */
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
        return 0;  /* Success - valid boot sector */
    }
    
    /* If no boot signature, check if it's at least not all zeros */
    int has_data = 0;
    for (int i = 0; i < SECTOR_SIZE; i++) {
        if (buffer[i] != 0) {
            has_data = 1;
            break;
        }
    }
    
    if (has_data) {
        return 0;  /* Success - has data but no boot signature */
    }
    
    return -1;  /* Failure - sector appears empty */
}

/* Get disk I/O statistics */
void disk_get_stats(disk_stats_t *stats) {
    if (stats) {
        stats->read_ops = g_disk_reads;
        stats->write_ops = g_disk_writes;
        stats->read_multi_ops = g_disk_multi_reads;
        stats->write_multi_ops = g_disk_multi_writes;
        stats->read_sectors = g_disk_read_sectors;
        stats->write_sectors = g_disk_write_sectors;
    }
}

/* Reset disk I/O statistics */
void disk_reset_stats(void) {
    g_disk_reads = 0;
    g_disk_writes = 0;
    g_disk_multi_reads = 0;
    g_disk_multi_writes = 0;
    g_disk_read_sectors = 0;
    g_disk_write_sectors = 0;
}