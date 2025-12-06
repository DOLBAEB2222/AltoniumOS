#include "../include/kernel/memory.h"
#include "../include/kernel/hw_detect.h"
#include "../include/kernel/bootlog.h"

/* Global memory manager state */
static physical_memory_manager_t g_pmm = {0};
static int g_pae_enabled = 0;

/* Page tables for PAE */
static pae_pdpt_t *g_pdpt = 0;
static pae_page_table_t *g_page_dir[4] = {0};
static pae_page_table_t *g_page_tables[2048] = {0}; /* Maximum with current setup */

/* External multiboot information */
extern uint32_t multiboot_magic_storage;
extern uint32_t multiboot_info_ptr_storage;

/* Control register access functions */
static inline uint32_t read_cr0(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

static inline uint32_t read_cr3(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline uint32_t read_cr4(void) {
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

static inline void write_cr3(uint32_t cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

static inline void write_cr4(uint32_t cr4) {
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
}

static inline void write_cr0(uint32_t cr0) {
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/* Simple frame allocator */
static void init_frame_allocator(void) {
    const hw_capabilities_t *hw_caps = hw_get_capabilities();
    
    g_pmm.total_memory = hw_caps->total_memory_kb * 1024;
    g_pmm.usable_memory = hw_caps->usable_memory_kb * 1024;
    g_pmm.total_frames = g_pmm.total_memory / PAGE_SIZE;
    g_pmm.free_frames = g_pmm.usable_memory / PAGE_SIZE;
    g_pmm.used_frames = 0;
    g_pmm.allocated_memory = 0;
    g_pmm.next_free_frame = 0;
    
    /* Initialize frame tracking */
    for (uint32_t i = 0; i < MAX_MEMORY_FRAMES && i < g_pmm.total_frames; i++) {
        g_pmm.frames[i].base_addr = (uint64_t)i * PAGE_SIZE;
        g_pmm.frames[i].order = 0;
        g_pmm.frames[i].flags = 0;
        
        /* Mark reserved memory regions as used */
        int is_reserved = 1;
        for (uint32_t j = 0; j < hw_caps->memory_region_count; j++) {
            const memory_region_t *region = &hw_caps->memory_regions[j];
            if (region->type == MEMORY_TYPE_AVAILABLE &&
                g_pmm.frames[i].base_addr >= region->base &&
                g_pmm.frames[i].base_addr < region->base + region->length) {
                is_reserved = 0;
                break;
            }
        }
        
        if (is_reserved) {
            g_pmm.frames[i].flags |= MEM_FLAG_RESERVED;
            g_pmm.used_frames++;
            g_pmm.free_frames--;
        }
    }
}

static uint32_t find_free_frames(uint32_t num_pages) {
    uint32_t consecutive = 0;
    uint32_t start = g_pmm.next_free_frame;
    
    for (uint32_t i = 0; i < g_pmm.total_frames; i++) {
        uint32_t frame = (start + i) % g_pmm.total_frames;
        
        if ((g_pmm.frames[frame].flags & (MEM_FLAG_USED | MEM_FLAG_RESERVED)) == 0) {
            if (consecutive == 0) {
                start = frame;
            }
            consecutive++;
            if (consecutive >= num_pages) {
                return start;
            }
        } else {
            consecutive = 0;
        }
    }
    
    return UINT32_MAX; /* No space available */
}

/* PAE page table management */
static int allocate_page_tables(void) {
    /* Allocate PDPT */
    g_pdpt = (pae_pdpt_t*)pmm_alloc(1);
    if (!g_pdpt) return -1;
    
    /* Allocate 4 page directories for PAE */
    for (int i = 0; i < 4; i++) {
        g_page_dir[i] = (pae_page_table_t*)pmm_alloc(1);
        if (!g_page_dir[i]) return -1;
        
        /* Set up PDPT entry */
        g_pdpt->entries[i] = (uint64_t)(uintptr_t)g_page_dir[i];
        g_pdpt->entries[i] |= PAE_FLAG_PRESENT | PAE_FLAG_RW;
    }
    
    return 0;
}

static void setup_identity_mapping(void) {
    /* Map low memory (first 2MB) for kernel and critical structures */
    for (uint64_t addr = 0; addr < 2 * 1024 * 1024; addr += PAGE_SIZE) {
        pae_map_page(addr, addr, PAE_FLAG_PRESENT | PAE_FLAG_RW);
    }
    
    /* Map kernel area (assuming kernel is loaded at 1MB) */
    for (uint64_t addr = 0x100000; addr < 0x100000 + 4 * 1024 * 1024; addr += PAGE_SIZE) {
        pae_map_page(addr, addr, PAE_FLAG_PRESENT | PAE_FLAG_RW);
    }
}

/* Public API functions */
void memory_init(void) {
    bootlog_print("Initializing memory manager...\n");
    
    init_frame_allocator();
    bootlog_print("Physical memory manager initialized\n");
    
    /* Try to enable PAE if supported */
    if (hw_has_pae()) {
        if (memory_enable_pae() == 0) {
            bootlog_print("PAE paging enabled\n");
        } else {
            bootlog_print("Failed to enable PAE, continuing without\n");
        }
    } else {
        bootlog_print("PAE not supported by CPU\n");
    }
}

int memory_enable_pae(void) {
    if (g_pae_enabled) {
        return 0; /* Already enabled */
    }
    
    if (!hw_has_pae()) {
        return -1; /* PAE not supported */
    }
    
    /* Allocate page tables */
    if (allocate_page_tables() != 0) {
        return -1;
    }
    
    /* Set up initial mappings */
    setup_identity_mapping();
    
    /* Enable PAE in CR4 */
    uint32_t cr4 = read_cr4();
    cr4 |= CR4_PAE;
    write_cr4(cr4);
    
    /* Load new page directory */
    write_cr3((uint32_t)(uintptr_t)g_pdpt);
    
    /* Enable paging in CR0 */
    uint32_t cr0 = read_cr0();
    cr0 |= CR0_PG;
    write_cr0(cr0);
    
    /* Update hardware capabilities */
    hw_capabilities_t *hw_caps = (hw_capabilities_t*)hw_get_capabilities();
    hw_caps->boot_info.pae_enabled = 1;
    
    g_pae_enabled = 1;
    return 0;
}

int memory_is_pae_enabled(void) {
    return g_pae_enabled;
}

void *pmm_alloc(uint32_t num_pages) {
    if (num_pages == 0) return 0;
    
    uint32_t start_frame = find_free_frames(num_pages);
    if (start_frame == UINT32_MAX) {
        return 0; /* Out of memory */
    }
    
    /* Mark frames as used */
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t frame = start_frame + i;
        g_pmm.frames[frame].flags |= MEM_FLAG_USED;
        g_pmm.frames[frame].order = 0;
    }
    
    g_pmm.used_frames += num_pages;
    g_pmm.free_frames -= num_pages;
    g_pmm.allocated_memory += num_pages * PAGE_SIZE;
    g_pmm.next_free_frame = start_frame + num_pages;
    
    return (void*)(uintptr_t)g_pmm.frames[start_frame].base_addr;
}

void pmm_free(void *addr, uint32_t num_pages) {
    if (!addr || num_pages == 0) return;
    
    uint64_t base_addr = (uint64_t)(uintptr_t)addr;
    uint32_t start_frame = addr_to_page(base_addr);
    
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t frame = start_frame + i;
        if (frame < g_pmm.total_frames) {
            g_pmm.frames[frame].flags &= ~MEM_FLAG_USED;
        }
    }
    
    g_pmm.used_frames -= num_pages;
    g_pmm.free_frames += num_pages;
    g_pmm.allocated_memory -= num_pages * PAGE_SIZE;
}

uint64_t pmm_get_total_memory(void) {
    return g_pmm.total_memory;
}

uint64_t pmm_get_free_memory(void) {
    return g_pmm.free_frames * PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void) {
    return g_pmm.used_frames * PAGE_SIZE;
}

int pae_init_page_tables(void) {
    return allocate_page_tables();
}

void pae_map_page(uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    if (!g_pae_enabled) return;
    
    /* Extract indices */
    uint32_t pdpt_index = (virt_addr >> 30) & 0x3;
    uint32_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint32_t pt_index = (virt_addr >> 12) & 0x1FF;
    
    /* Get page directory */
    pae_page_table_t *pd = g_page_dir[pdpt_index];
    if (!pd) return;
    
    /* Get page table entry from page directory */
    uint64_t pde = pd->entries[pd_index];
    if ((pde & PAE_FLAG_PRESENT) == 0) {
        /* Allocate page table */
        pae_page_table_t *pt = (pae_page_table_t*)pmm_alloc(1);
        if (!pt) return;
        
        /* Set up PDE */
        pd->entries[pd_index] = (uint64_t)(uintptr_t)pt;
        pd->entries[pd_index] |= PAE_FLAG_PRESENT | PAE_FLAG_RW;
        
        pde = pd->entries[pd_index];
    }
    
    /* Get page table */
    pae_page_table_t *pt = (pae_page_table_t*)(uintptr_t)(pde & ~0xFFFULL);
    
    /* Set up PTE */
    pt->entries[pt_index] = phys_addr | flags;
}

void pae_unmap_page(uint64_t virt_addr) {
    if (!g_pae_enabled) return;
    
    uint32_t pdpt_index = (virt_addr >> 30) & 0x3;
    uint32_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint32_t pt_index = (virt_addr >> 12) & 0x1FF;
    
    pae_page_table_t *pd = g_page_dir[pdpt_index];
    if (!pd) return;
    
    uint64_t pde = pd->entries[pd_index];
    if ((pde & PAE_FLAG_PRESENT) == 0) return;
    
    pae_page_table_t *pt = (pae_page_table_t*)(uintptr_t)(pde & ~0xFFFULL);
    pt->entries[pt_index] = 0;
}

uint64_t pae_get_phys_addr(uint64_t virt_addr) {
    if (!g_pae_enabled) return virt_addr;
    
    uint32_t pdpt_index = (virt_addr >> 30) & 0x3;
    uint32_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint32_t pt_index = (virt_addr >> 12) & 0x1FF;
    
    pae_page_table_t *pd = g_page_dir[pdpt_index];
    if (!pd) return 0;
    
    uint64_t pde = pd->entries[pd_index];
    if ((pde & PAE_FLAG_PRESENT) == 0) return 0;
    
    pae_page_table_t *pt = (pae_page_table_t*)(uintptr_t)(pde & ~0xFFFULL);
    uint64_t pte = pt->entries[pt_index];
    
    if ((pte & PAE_FLAG_PRESENT) == 0) return 0;
    
    return (pte & ~0xFFFULL) | (virt_addr & 0xFFFULL);
}