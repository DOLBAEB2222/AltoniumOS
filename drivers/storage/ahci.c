#include "../../include/drivers/storage/block_device.h"
#include "../../include/drivers/pci.h"
#include "../../include/drivers/console.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

#define AHCI_CAP            0x00
#define AHCI_GHC            0x04
#define AHCI_PI             0x0C
#define AHCI_VS             0x10
#define AHCI_CCC_CTL        0x14
#define AHCI_CCC_PORTS      0x18
#define AHCI_EM_LOC         0x1C
#define AHCI_EM_CTL         0x20
#define AHCI_CAP2           0x24
#define AHCI_BOHC           0x28

#define AHCI_PORT_BASE      0x100
#define AHCI_PORT_SIZE      0x80

#define AHCI_PORT_CLB       0x00
#define AHCI_PORT_CLBU      0x04
#define AHCI_PORT_FB        0x08
#define AHCI_PORT_FBU       0x0C
#define AHCI_PORT_IS        0x10
#define AHCI_PORT_IE        0x14
#define AHCI_PORT_CMD       0x18
#define AHCI_PORT_RESERVED  0x1C
#define AHCI_PORT_TFD       0x20
#define AHCI_PORT_SIG       0x24
#define AHCI_PORT_SSTS      0x28
#define AHCI_PORT_SCTL      0x2C
#define AHCI_PORT_SERR      0x30
#define AHCI_PORT_SACT      0x34
#define AHCI_PORT_CI        0x38

#define AHCI_GHC_AE         0x80000000
#define AHCI_GHC_RESET      0x00000001

#define AHCI_PORT_CMD_ST    0x00000001
#define AHCI_PORT_CMD_FRE   0x00000010

/* AHCI device private data */
typedef struct {
    pci_device_t *pci_dev;
    uint32_t *hba_mem;
    uint32_t sector_size;
    uint32_t capacity_sectors;
    int port_index;
} ahci_private_t;

static ahci_private_t *current_ahci_device = 0;

/* Minimal AHCI read implementation (PIO fallback) */
static int ahci_read(block_device_t *dev, uint32_t lba, uint8_t *buffer, uint16_t num_sectors) {
    /* For now, return -1 to indicate not yet implemented */
    return -1;
}

/* Minimal AHCI write implementation */
static int ahci_write(block_device_t *dev, uint32_t lba, const uint8_t *buffer, uint16_t num_sectors) {
    /* For now, return -1 to indicate read-only or not implemented */
    return -1;
}

int ahci_init(block_device_t *dev) {
    pci_device_t *pci_dev = 0;
    
    /* Find first AHCI controller */
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t *d = pci_get_device(i);
        if (d && d->class_code == PCI_CLASS_STORAGE && d->subclass_code == PCI_SUBCLASS_SATA) {
            pci_dev = d;
            break;
        }
    }
    
    if (!pci_dev) {
        return -1;
    }
    
    /* Enable memory space and bus master */
    pci_enable_memory_space(pci_dev);
    
    /* Get ABAR (BAR5 for AHCI) */
    uint32_t abar = pci_dev->bar[5];
    if (abar == 0) {
        abar = pci_dev->bar[0];
    }
    
    if (abar == 0 || abar == 0xFFFFFFFF) {
        return -2;
    }
    
    /* Map ABAR to memory */
    uint32_t *hba_mem = (uint32_t *)abar;
    
    /* Initialize AHCI host controller */
    uint32_t ghc = *(volatile uint32_t *)(hba_mem + (AHCI_GHC / 4));
    
    if ((ghc & AHCI_GHC_AE) == 0) {
        /* Enable AHCI */
        *(volatile uint32_t *)(hba_mem + (AHCI_GHC / 4)) = ghc | AHCI_GHC_AE;
    }
    
    /* Allocate and set up device private data */
    ahci_private_t *priv = (ahci_private_t *)0;
    
    dev->type = BLOCK_DEVICE_AHCI;
    dev->sector_size = 512;
    dev->capacity_sectors = 0;
    dev->driver_name = "AHCI";
    dev->queue_depth = 32;
    dev->ops.read = ahci_read;
    dev->ops.write = ahci_write;
    dev->private_data = priv;
    
    return 0;
}
