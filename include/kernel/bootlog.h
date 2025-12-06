#ifndef KERNEL_BOOTLOG_H
#define KERNEL_BOOTLOG_H

#include "../lib/string.h"

/* Bootlog structure stored in low memory by bootloader */
#define BOOTLOG_ADDR 0x500  /* Safe location in low memory */
#define BOOTLOG_MAGIC 0x424F4F54  /* "BOOT" */

typedef struct {
    uint32_t magic;           /* 0x424F4F54 */
    uint8_t int13_extensions; /* 1 = EDD supported, 0 = fallback to CHS */
    uint8_t int13_status;     /* Status from last INT 13h call */
    uint16_t reserved1;
    
    /* Drive geometry (from INT 13h, AH=08h) */
    uint16_t cylinders;
    uint8_t heads;
    uint8_t sectors_per_track;
    
    /* BIOS vendor string (up to 32 chars, NUL-terminated) */
    char bios_vendor[32];
    
    /* Status string (up to 64 chars, NUL-terminated) */
    char status_string[64];
    
    uint8_t reserved[64];  /* Padding for future use */
} bootlog_data_t;

/* Global bootlog data for kernel access */
extern bootlog_data_t *bootlog_data;

/* Kernel-side functions */
void bootlog_init(void);
void bootlog_print(void);

#endif
