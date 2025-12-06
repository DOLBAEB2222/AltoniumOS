#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stdint.h>
#include "hw_detect.h"

/* Page size definitions */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12

/* PAE page table entries */
#define PAE_PAGE_ENTRIES    512
#define PAE_PAGE_SIZE       4096

/* PAE page table structure */
typedef struct {
    uint64_t entries[PAE_PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) pae_page_table_t;

/* Page directory pointer table (PDPT) */
typedef struct {
    uint64_t entries[4];
} __attribute__((aligned(PAGE_SIZE))) pae_pdpt_t;

/* Physical memory frame */
typedef struct {
    uint64_t base_addr;
    uint32_t order;      /* Allocation order (log2 of number of pages) */
    uint32_t flags;
} memory_frame_t;

/* Memory allocation flags */
#define MEM_FLAG_USED      0x01
#define MEM_FLAG_RESERVED  0x02
#define MEM_FLAG_DMA       0x04

/* Maximum memory frames to track */
#define MAX_MEMORY_FRAMES  8192  /* Supports up to 32GB with 4KB pages */

/* Physical memory manager state */
typedef struct {
    uint64_t total_memory;
    uint64_t usable_memory;
    uint64_t allocated_memory;
    uint32_t total_frames;
    uint32_t free_frames;
    uint32_t used_frames;
    memory_frame_t frames[MAX_MEMORY_FRAMES];
    uint32_t next_free_frame;
} physical_memory_manager_t;

/* Function prototypes */
void memory_init(void);
int memory_enable_pae(void);
int memory_is_pae_enabled(void);
void *pmm_alloc(uint32_t num_pages);
void pmm_free(void *addr, uint32_t num_pages);
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);

/* PAE page table functions */
int pae_init_page_tables(void);
void pae_map_page(uint64_t phys_addr, uint64_t virt_addr, uint32_t flags);
void pae_unmap_page(uint64_t virt_addr);
uint64_t pae_get_phys_addr(uint64_t virt_addr);

/* Memory management helper functions */
static inline uint64_t page_align_up(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static inline uint64_t page_align_down(uint64_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static inline uint32_t addr_to_page(uint64_t addr) {
    return addr >> PAGE_SHIFT;
}

static inline uint64_t page_to_addr(uint32_t page) {
    return (uint64_t)page << PAGE_SHIFT;
}

/* Page table entry flags */
#define PAE_FLAG_PRESENT    (1 << 0)
#define PAE_FLAG_RW         (1 << 1)
#define PAE_FLAG_USER       (1 << 2)
#define PAE_FLAG_PWT        (1 << 3)
#define PAE_FLAG_PCD        (1 << 4)
#define PAE_FLAG_ACCESSED   (1 << 5)
#define PAE_FLAG_DIRTY      (1 << 6)
#define PAE_FLAG_PAT        (1 << 7)
#define PAE_FLAG_GLOBAL     (1 << 8)
#define PAE_FLAG_XD         (1ULL << 63)

/* Control register bits */
#define CR4_PAE             (1 << 5)
#define CR4_PGE             (1 << 7)
#define CR0_PG              (1 << 31)

#endif