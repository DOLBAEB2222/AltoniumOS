#include "../../include/drivers/pci.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

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

static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static pci_device_t pci_devices[PCI_MAX_DEVICES];
static int pci_device_count = 0;

/* Read a PCI configuration register using mechanism 1 */
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset) {
    uint32_t addr = 0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)fn << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset, uint32_t value) {
    uint32_t addr = 0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)fn << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    outl(PCI_CONFIG_DATA, value);
}

/* Get a value from a specific byte offset in config space */
static uint8_t pci_read_byte(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset) {
    uint32_t data = pci_read_config(bus, dev, fn, offset);
    return (data >> ((offset & 3) * 8)) & 0xFF;
}

static uint16_t pci_read_word(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t offset) {
    uint32_t data = pci_read_config(bus, dev, fn, offset);
    return (data >> ((offset & 2) * 8)) & 0xFFFF;
}

/* Get BAR with proper size detection */
uint32_t pci_get_bar(pci_device_t *dev, int bar_index) {
    if (bar_index < 0 || bar_index >= 6) {
        return 0;
    }
    return dev->bar[bar_index];
}

int pci_enable_memory_space(pci_device_t *dev) {
    uint16_t cmd = pci_read_word(dev->bus, dev->dev, dev->fn, PCI_COMMAND);
    cmd |= PCI_CMD_MEMORY_SPACE | PCI_CMD_BUS_MASTER;
    pci_write_config(dev->bus, dev->dev, dev->fn, PCI_COMMAND, cmd);
    return 0;
}

/* Enumerate PCI devices via configuration mechanism 1 */
int pci_enumerate(void) {
    pci_device_count = 0;
    
    for (int bus = 0; bus < 256 && pci_device_count < PCI_MAX_DEVICES; bus++) {
        for (int dev = 0; dev < 32 && pci_device_count < PCI_MAX_DEVICES; dev++) {
            for (int fn = 0; fn < 8 && pci_device_count < PCI_MAX_DEVICES; fn++) {
                uint16_t vendor_id = pci_read_word(bus, dev, fn, PCI_VENDOR_ID);
                
                if (vendor_id == 0xFFFF) {
                    continue;
                }
                
                uint16_t device_id = pci_read_word(bus, dev, fn, PCI_DEVICE_ID);
                uint8_t class_code = pci_read_byte(bus, dev, fn, PCI_CLASS_CODE);
                uint8_t subclass_code = pci_read_byte(bus, dev, fn, PCI_SUBCLASS_CODE);
                uint8_t prog_if = pci_read_byte(bus, dev, fn, PCI_PROG_IF);
                
                pci_device_t *dev_entry = &pci_devices[pci_device_count];
                dev_entry->bus = bus;
                dev_entry->dev = dev;
                dev_entry->fn = fn;
                dev_entry->vendor_id = vendor_id;
                dev_entry->device_id = device_id;
                dev_entry->class_code = class_code;
                dev_entry->subclass_code = subclass_code;
                dev_entry->prog_if = prog_if;
                
                for (int bar_idx = 0; bar_idx < 6; bar_idx++) {
                    uint32_t bar_offset = PCI_BAR0 + (bar_idx * 4);
                    dev_entry->bar[bar_idx] = pci_read_config(bus, dev, fn, bar_offset) & 0xFFFFFFF0;
                }
                
                pci_device_count++;
            }
        }
    }
    
    return pci_device_count;
}

int pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t *pci_get_device(int index) {
    if (index < 0 || index >= pci_device_count) {
        return 0;
    }
    return &pci_devices[index];
}
