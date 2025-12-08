#ifndef PCI_H
#define PCI_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define PCI_MAX_DEVICES 256

/* PCI Configuration Space Offsets */
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02
#define PCI_COMMAND             0x04
#define PCI_STATUS              0x06
#define PCI_REVISION_ID         0x08
#define PCI_CLASS_CODE          0x09
#define PCI_SUBCLASS_CODE       0x0A
#define PCI_PROG_IF             0x09
#define PCI_CACHE_LINE_SIZE     0x0C
#define PCI_LATENCY_TIMER       0x0D
#define PCI_HEADER_TYPE         0x0E
#define PCI_BIST                0x0F
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1C
#define PCI_BAR4                0x20
#define PCI_BAR5                0x24

/* PCI Device Classes */
#define PCI_CLASS_STORAGE       0x01
#define PCI_SUBCLASS_SATA       0x06
#define PCI_SUBCLASS_NVME       0x08
#define PCI_SUBCLASS_ATA        0x01

/* PCI Command Register Bits */
#define PCI_CMD_IO_SPACE        0x0001
#define PCI_CMD_MEMORY_SPACE    0x0002
#define PCI_CMD_BUS_MASTER      0x0004

/* PCI Device Structure */
typedef struct {
    uint8_t bus;
    uint8_t dev;
    uint8_t fn;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t prog_if;
    uint32_t bar[6];
    uint32_t bar_size[6];
} pci_device_t;

/* PCI enumeration functions */
int pci_enumerate(void);
int pci_get_device_count(void);
pci_device_t *pci_get_device(int index);
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset, uint32_t value);
uint32_t pci_get_bar(pci_device_t *dev, int bar_index);
int pci_enable_memory_space(pci_device_t *dev);

#endif /* PCI_H */
