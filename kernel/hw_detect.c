#include <stdint.h>
#include "../include/kernel/hw_detect.h"
#include "../include/kernel/main.h"
#include "../include/kernel/bootlog.h"
#include "../include/drivers/console.h"

/* Global hardware capabilities */
static hw_capabilities_t g_hw_caps = {0};

/* External multiboot information */
extern uint32_t multiboot_magic_storage;
extern uint32_t multiboot_info_ptr_storage;

/* Memory map entry structure from Multiboot specification */
typedef struct {
    uint32_t size;
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

/* PCI configuration space access */
static inline uint32_t pci_read_config(uint8_t bus, uint8_t device, 
                                        uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)((1 << 31) | (bus << 16) | 
                                   (device << 11) | (function << 8) | (offset & 0xFC));
    outl(0xCF8, address);
    return inl(0xCFC);
}

/* Use I/O functions from string.h - no duplicates needed */

/* CPU detection functions */
static void detect_cpu_vendor(void) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Check if CPUID is supported */
    if (!g_hw_caps.cpu_signature) {
        /* Try to toggle ID bit in EFLAGS */
        uint32_t flags;
        __asm__ volatile("pushf\n\t"
                         "pop %0\n\t"
                         "mov %0, %1\n\t"
                         "xor $0x200000, %0\n\t"
                         "push %0\n\t"
                         "popf\n\t"
                         "pushf\n\t"
                         "pop %0\n\t"
                         : "=r"(flags), "=r"(eax)
                         :
                         : "cc");
        
        if ((flags ^ eax) & 0x200000) {
            /* CPUID is supported */
            cpuid(0, &eax, &ebx, &ecx, &edx);
            g_hw_caps.cpu_signature = eax;
            
            /* Store vendor string */
            *((uint32_t*)&g_hw_caps.cpu_vendor[0]) = ebx;
            *((uint32_t*)&g_hw_caps.cpu_vendor[4]) = edx;
            *((uint32_t*)&g_hw_caps.cpu_vendor[8]) = ecx;
            g_hw_caps.cpu_vendor[12] = '\0';
        }
    }
}

static void detect_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;
    
    if (!g_hw_caps.cpu_signature) {
        return;
    }
    
    /* Get standard features */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* Extract CPU family/model/stepping */
    g_hw_caps.cpu_signature = eax;
    g_hw_caps.cpu_family = (eax >> 8) & 0xF;
    g_hw_caps.cpu_model_num = (eax >> 4) & 0xF;
    g_hw_caps.cpu_stepping = eax & 0xF;
    
    /* Handle extended family/model */
    if (g_hw_caps.cpu_family == 0xF) {
        g_hw_caps.cpu_family += (eax >> 20) & 0xFF;
    }
    if (g_hw_caps.cpu_family == 0x6 || g_hw_caps.cpu_family == 0xF) {
        g_hw_caps.cpu_model_num += ((eax >> 16) & 0xF) << 4;
    }
    
    /* Feature flags */
    g_hw_caps.cpu_features.pae = (edx >> 6) & 1;
    g_hw_caps.cpu_features.apic = (edx >> 9) & 1;
    g_hw_caps.cpu_features.sse2 = (edx >> 26) & 1;
    g_hw_caps.cpu_features.x2apic = (ecx >> 21) & 1;
    
    /* Check for long mode */
    if (g_hw_caps.cpu_signature >= 0x80000001) {
        cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
        g_hw_caps.cpu_features.long_mode = (edx >> 29) & 1;
    }
    
    /* Get CPU model string */
    if (g_hw_caps.cpu_signature >= 0x80000004) {
        char *model_ptr = g_hw_caps.cpu_model;
        for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
            cpuid(i, &eax, &ebx, &ecx, &edx);
            
            *((uint32_t*)model_ptr) = eax; model_ptr += 4;
            *((uint32_t*)model_ptr) = ebx; model_ptr += 4;
            *((uint32_t*)model_ptr) = ecx; model_ptr += 4;
            *((uint32_t*)model_ptr) = edx; model_ptr += 4;
        }
        g_hw_caps.cpu_model[48] = '\0';
        
        /* Remove leading spaces */
        char *start = g_hw_caps.cpu_model;
        while (*start == ' ') start++;
        if (start != g_hw_caps.cpu_model) {
            char *dst = g_hw_caps.cpu_model;
            while (*start) *dst++ = *start++;
            *dst = '\0';
        }
    }
}

/* Memory map detection */
static void detect_memory_map(void) {
    g_hw_caps.total_memory_kb = 0;
    g_hw_caps.usable_memory_kb = 0;
    g_hw_caps.memory_region_count = 0;
    
    if (multiboot_magic_storage != MULTIBOOT_BOOTLOADER_MAGIC) {
        return;
    }
    
    multiboot_info_t *info = (multiboot_info_t *)(uintptr_t)multiboot_info_ptr_storage;
    if (!info || !(info->flags & (1 << 6))) {
        return;
    }
    
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(uintptr_t)info->mmap_addr;
    uint32_t mmap_end = info->mmap_addr + info->mmap_length;
    
    while ((uint32_t)mmap < mmap_end && g_hw_caps.memory_region_count < MAX_MEMORY_REGIONS) {
        memory_region_t *region = &g_hw_caps.memory_regions[g_hw_caps.memory_region_count];
        
        region->base = mmap->base;
        region->length = mmap->length;
        region->type = mmap->type;
        region->reserved = 0;
        
        /* Convert to KB for total count */
        uint64_t region_kb = mmap->length / 1024;
        g_hw_caps.total_memory_kb += region_kb;
        
        if (mmap->type == MEMORY_TYPE_AVAILABLE) {
            g_hw_caps.usable_memory_kb += region_kb;
        }
        
        g_hw_caps.memory_region_count++;
        
        /* Move to next entry */
        mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
}

/* ACPI RSDP detection */
static void detect_acpi(void) {
    g_hw_caps.rsdp_address = 0;
    g_hw_caps.acpi_info.present = 0;
    
    /* Search EBDA (Extended BIOS Data Area) */
    uint16_t ebda_segment = *((uint16_t*)0x40E);
    if (ebda_segment) {
        uint8_t *ebda = (uint8_t*)(ebda_segment * 16);
        for (uint8_t *ptr = ebda; ptr < ebda + 1024; ptr += 16) {
            if (ptr[0] == 'R' && ptr[1] == 'S' && ptr[2] == 'D' && ptr[3] == 'P' &&
                ptr[4] == ' ' && ptr[5] == 'P' && ptr[6] == 'T' && ptr[7] == 'R') {
                /* Check checksum */
                uint8_t sum = 0;
                for (int i = 0; i < 20; i++) sum += ptr[i];
                if (sum == 0) {
                    g_hw_caps.rsdp_address = (uint64_t)(uintptr_t)ptr;
                    g_hw_caps.rsdp_revision = ptr[8];
                    for (int i = 0; i < 6; i++) g_hw_caps.rsdp_oem_id[i] = ptr[9 + i];
                    g_hw_caps.acpi_info.present = 1;
                    return;
                }
            }
        }
    }
    
    /* Search main BIOS area (0xE0000 - 0xFFFFF) */
    for (uint8_t *ptr = (uint8_t*)0xE0000; ptr < (uint8_t*)0x100000; ptr += 16) {
        if (ptr[0] == 'R' && ptr[1] == 'S' && ptr[2] == 'D' && ptr[3] == 'P' &&
            ptr[4] == ' ' && ptr[5] == 'P' && ptr[6] == 'T' && ptr[7] == 'R') {
            /* Check checksum */
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) sum += ptr[i];
            if (sum == 0) {
                g_hw_caps.rsdp_address = (uint64_t)(uintptr_t)ptr;
                g_hw_caps.rsdp_revision = ptr[8];
                for (int i = 0; i < 6; i++) g_hw_caps.rsdp_oem_id[i] = ptr[9 + i];
                g_hw_caps.acpi_info.present = 1;
                return;
            }
        }
    }
}

/* PCI device detection */
static void detect_pci_devices(void) {
    g_hw_caps.pci_devices.pci_bus_present = 0;
    g_hw_caps.pci_devices.ps2_controller_present = 0;
    g_hw_caps.pci_devices.usb_controller_present = 0;
    g_hw_caps.pci_devices.storage_controller_present = 0;
    
    /* Check if PCI exists by trying to read a PCI configuration register */
    uint32_t config = pci_read_config(0, 0, 0, 0);
    if (config == 0xFFFFFFFF || config == 0x00000000) {
        return;
    }
    
    g_hw_caps.pci_devices.pci_bus_present = 1;
    
    /* Scan for common devices */
    for (int bus = 0; bus < 256; bus++) {
        for (int device = 0; device < 32; device++) {
            uint32_t vendor_device = pci_read_config(bus, device, 0, 0);
            uint16_t vendor_id = vendor_device & 0xFFFF;
            uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
            
            if (vendor_id == 0xFFFF) continue;
            
            uint32_t class_rev = pci_read_config(bus, device, 0, 8);
            uint8_t class_code = (class_rev >> 24) & 0xFF;
            uint8_t subclass = (class_rev >> 16) & 0xFF;
            
            /* Check for various device types */
            switch (class_code) {
                case 0x01: /* Mass Storage Controller */
                    g_hw_caps.pci_devices.storage_controller_present = 1;
                    break;
                case 0x0C: /* Serial Bus Controller */
                    if (subclass == 0x03) { /* USB Controller */
                        g_hw_caps.pci_devices.usb_controller_present = 1;
                    }
                    break;
            }
        }
    }
    
    /* PS/2 controller is typically not on PCI, check legacy ports */
    /* Test PS/2 controller status */
    outb(0x64, 0xAB); /* Self-test command */
    uint8_t response = inb(0x64);
    if ((response & 0x20) == 0) { /* Test passed */
        g_hw_caps.pci_devices.ps2_controller_present = 1;
    }
}

/* Boot mode detection */
static void detect_boot_mode(void) {
    /* This will be called after detect_boot_mode() in main.c */
    extern int get_current_boot_mode(void);
    int mode = get_current_boot_mode();
    
    g_hw_caps.boot_info.boot_mode_bios = (mode == 1);
    g_hw_caps.boot_info.boot_mode_uefi = (mode == 2);
    g_hw_caps.boot_info.pae_enabled = 0; /* Will be set by memory manager */
}

/* Public API functions */
void hw_detect_init(void) {
    /* Note: bootlog_print() takes no parameters in current implementation */
    bootlog_print();
    
    /* Clear structure */
    for (size_t i = 0; i < sizeof(g_hw_caps); i++) {
        ((char*)&g_hw_caps)[i] = 0;
    }
    
    detect_cpu_vendor();
    detect_cpu_features();
    detect_memory_map();
    detect_acpi();
    detect_pci_devices();
    detect_boot_mode();
    
    bootlog_print();
}

const hw_capabilities_t *hw_get_capabilities(void) {
    return &g_hw_caps;
}

int hw_has_pae(void) {
    return g_hw_caps.cpu_features.pae;
}

int hw_has_apic(void) {
    return g_hw_caps.cpu_features.apic;
}

int hw_has_ps2_controller(void) {
    return g_hw_caps.pci_devices.ps2_controller_present;
}

int hw_has_storage_controller(void) {
    return g_hw_caps.pci_devices.storage_controller_present;
}

int hw_is_pae_enabled(void) {
    return g_hw_caps.boot_info.pae_enabled;
}

uint64_t hw_get_total_memory_kb(void) {
    return g_hw_caps.total_memory_kb;
}

uint64_t hw_get_usable_memory_kb(void) {
    return g_hw_caps.usable_memory_kb;
}