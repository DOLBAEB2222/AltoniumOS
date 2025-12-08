#include "../../include/drivers/storage/block_device.h"
#include "../../include/drivers/pci.h"
#include "../../include/drivers/console.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define STORAGE_MAX_DEVICES 16

static block_device_t storage_devices[STORAGE_MAX_DEVICES];
static int storage_device_count = 0;
static block_device_t *primary_device = 0;

/* Forward declarations for device drivers */
extern int ata_pio_init(block_device_t *dev);
extern int ahci_init(block_device_t *dev);
extern int nvme_init(block_device_t *dev);

int storage_register_device(block_device_t *dev) {
    if (storage_device_count >= STORAGE_MAX_DEVICES) {
        return -1;
    }
    
    storage_devices[storage_device_count] = *dev;
    storage_device_count++;
    
    return 0;
}

int storage_manager_init(void) {
    storage_device_count = 0;
    primary_device = 0;
    
    /* Initialize PCI enumeration */
    pci_enumerate();
    
    /* First, try to initialize NVMe devices (highest priority) */
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t *dev = pci_get_device(i);
        if (dev && dev->class_code == PCI_CLASS_STORAGE && dev->subclass_code == PCI_SUBCLASS_NVME) {
            block_device_t bd;
            if (nvme_init(&bd) == 0) {
                if (storage_register_device(&bd) == 0 && primary_device == 0) {
                    primary_device = &storage_devices[storage_device_count - 1];
                }
            }
        }
    }
    
    /* Then, try to initialize AHCI devices (middle priority) */
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t *dev = pci_get_device(i);
        if (dev && dev->class_code == PCI_CLASS_STORAGE && dev->subclass_code == PCI_SUBCLASS_SATA) {
            block_device_t bd;
            if (ahci_init(&bd) == 0) {
                if (storage_register_device(&bd) == 0 && primary_device == 0) {
                    primary_device = &storage_devices[storage_device_count - 1];
                }
            }
        }
    }
    
    /* Finally, try to initialize legacy ATA (lowest priority) */
    block_device_t ata_bd;
    if (ata_pio_init(&ata_bd) == 0) {
        if (storage_register_device(&ata_bd) == 0 && primary_device == 0) {
            primary_device = &storage_devices[storage_device_count - 1];
        }
    }
    
    return storage_device_count;
}

block_device_t *storage_get_device(int index) {
    if (index < 0 || index >= storage_device_count) {
        return 0;
    }
    return &storage_devices[index];
}

int storage_get_device_count(void) {
    return storage_device_count;
}

block_device_t *storage_get_primary_device(void) {
    return primary_device;
}
