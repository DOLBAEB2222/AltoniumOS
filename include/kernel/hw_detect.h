#ifndef KERNEL_HW_DETECT_H
#define KERNEL_HW_DETECT_H

#include <stdint.h>
/* Avoid conflicts with string.h */
#ifndef UINTPTR_T_DEFINED
#define UINTPTR_T_DEFINED
typedef uintptr_t hw_uintptr_t;
#endif

/* CPU feature flags */
typedef struct {
    uint32_t pae : 1;          /* Physical Address Extension */
    uint32_t apic : 1;         /* APIC present */
    uint32_t x2apic : 1;       /* x2APIC present */
    uint32_t sse2 : 1;         /* SSE2 present */
    uint32_t long_mode : 1;    /* x86-64 long mode available */
    uint32_t reserved_cpu : 27;
} cpu_features_t;

/* PCI device class codes */
typedef struct {
    uint32_t ps2_controller_present : 1;    /* PS/2 controller */
    uint32_t usb_controller_present : 1;    /* USB controller */
    uint32_t storage_controller_present : 1; /* Storage controller */
    uint32_t pci_bus_present : 1;           /* PCI bus functional */
    uint32_t reserved_pci : 4;
} pci_devices_t;

/* ACPI information */
typedef struct {
    uint32_t present : 1;       /* ACPI RSDP found */
    uint32_t reserved_hw : 31;
} acpi_info_t;

/* Boot mode information */
typedef struct {
    uint32_t pae_enabled : 1;   /* PAE actually enabled */
    uint32_t boot_mode_bios : 1;
    uint32_t boot_mode_uefi : 1;
    uint32_t reserved_boot : 29;
} boot_info_t;

/* Memory region types */
#define MEMORY_TYPE_AVAILABLE      1
#define MEMORY_TYPE_RESERVED       2
#define MEMORY_TYPE_ACPI_RECLAIM   3
#define MEMORY_TYPE_NVS           4
#define MEMORY_TYPE_UNUSABLE      5

/* Memory region descriptor */
typedef struct memory_region {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} memory_region_t;

/* Maximum number of memory regions to track */
#define MAX_MEMORY_REGIONS 32

/* Complete hardware capability structure */
typedef struct {
    /* CPU information */
    char cpu_vendor[13];        /* CPU vendor string */
    char cpu_model[49];         /* CPU model string */
    uint32_t cpu_signature;     /* CPU signature from CPUID */
    uint8_t cpu_family;         /* CPU family */
    uint8_t cpu_model_num;      /* CPU model number */
    uint8_t cpu_stepping;       /* CPU stepping */
    uint8_t cpu_reserved;
    
    /* Feature flags */
    cpu_features_t cpu_features;
    pci_devices_t pci_devices;
    acpi_info_t acpi_info;
    boot_info_t boot_info;
    
    /* Memory information */
    uint64_t total_memory_kb;   /* Total memory in KB */
    uint64_t usable_memory_kb;  /* Usable memory in KB */
    uint32_t memory_region_count;
    memory_region_t memory_regions[MAX_MEMORY_REGIONS];
    
    /* RSDP information */
    hw_uintptr_t rsdp_address;     /* ACPI RSDP physical address */
    uint8_t rsdp_revision;      /* ACPI revision */
    uint8_t rsdp_oem_id[6];     /* OEM ID */
} hw_capabilities_t;

/* Function prototypes */
void hw_detect_init(void);
const hw_capabilities_t *hw_get_capabilities(void);
int hw_has_pae(void);
int hw_has_apic(void);
int hw_has_ps2_controller(void);
int hw_has_storage_controller(void);
int hw_is_pae_enabled(void);
uint64_t hw_get_total_memory_kb(void);
uint64_t hw_get_usable_memory_kb(void);

/* CPUID helper functions */
static inline void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, 
                         uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf));
}

static inline void cpuid_sub(uint32_t leaf, uint32_t sub, uint32_t *eax, 
                             uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(sub));
}

#endif