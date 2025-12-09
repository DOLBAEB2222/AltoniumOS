#include "../include/kernel/bootlog.h"
#include "../include/drivers/console.h"

bootlog_data_t *bootlog_data = (bootlog_data_t *)0;

void bootlog_init(void) {
    bootlog_data = (bootlog_data_t *)BOOTLOG_ADDR;
    
    /* Check if bootlog is valid */
    if (bootlog_data->magic != BOOTLOG_MAGIC) {
        /* Bootlog not initialized, set defaults */
        bootlog_data->magic = BOOTLOG_MAGIC;
        bootlog_data->int13_extensions = 0;
        bootlog_data->int13_status = 0;
        bootlog_data->boot_method = 0;
        bootlog_data->retry_count = 0;
        bootlog_data->cylinders = 0;
        bootlog_data->heads = 0;
        bootlog_data->sectors_per_track = 0;
        bootlog_data->memory_mb = 0;
        bootlog_data->bios_vendor[0] = '\0';
        bootlog_data->status_string[0] = '\0';
    }
}

void bootlog_print(void) {
    if (!bootlog_data) {
        bootlog_init();
    }
    
    console_print("Boot diagnostics:\n");
    console_print("  Extensions:    ");
    if (bootlog_data->int13_extensions) {
        console_print("EDD supported\n");
    } else {
        console_print("CHS fallback\n");
    }
    
    console_print("  Boot method:   ");
    switch (bootlog_data->boot_method) {
        case 0: console_print("CHS"); break;
        case 1: console_print("EDD"); break;
        case 2: console_print("Error"); break;
        default: console_print("Unknown"); break;
    }
    console_print("\n");
    
    console_print("  INT13 status:  0x");
    if (bootlog_data->int13_status < 16) {
        console_print("0");
    }
    char hex[3];
    hex[0] = "0123456789ABCDEF"[bootlog_data->int13_status >> 4];
    hex[1] = "0123456789ABCDEF"[bootlog_data->int13_status & 0x0F];
    hex[2] = '\0';
    console_print(hex);
    console_print("\n");
    
    console_print("  Retry count:   ");
    unsigned int retry = bootlog_data->retry_count;
    char buf[16];
    int i = 0;
    if (retry == 0) {
        buf[i++] = '0';
    } else {
        char temp[16];
        int ti = 0;
        while (retry > 0) {
            temp[ti++] = '0' + (retry % 10);
            retry /= 10;
        }
        for (int j = ti - 1; j >= 0; j--) {
            buf[i++] = temp[j];
        }
    }
    buf[i] = '\0';
    console_print(buf);
    console_print("\n");
    
    if (bootlog_data->status_string[0] != '\0') {
        console_print("  Status:        ");
        console_print(bootlog_data->status_string);
        console_print("\n");
    }
    
    if (bootlog_data->memory_mb > 0) {
        console_print("  Memory:        ");
        unsigned int mem = bootlog_data->memory_mb;
        buf[0] = 0;
        i = 0;
        if (mem == 0) {
            buf[i++] = '0';
        } else {
            char temp[16];
            int ti = 0;
            while (mem > 0) {
                temp[ti++] = '0' + (mem % 10);
                mem /= 10;
            }
            for (int j = ti - 1; j >= 0; j--) {
                buf[i++] = temp[j];
            }
        }
        buf[i] = '\0';
        console_print(buf);
        console_print(" MB\n");
    }
    
    if (bootlog_data->cylinders > 0) {
        console_print("  Cylinders:     ");
        unsigned int cyl = bootlog_data->cylinders;
        i = 0;
        if (cyl == 0) {
            buf[i++] = '0';
        } else {
            char temp[16];
            int ti = 0;
            while (cyl > 0) {
                temp[ti++] = '0' + (cyl % 10);
                cyl /= 10;
            }
            for (int j = ti - 1; j >= 0; j--) {
                buf[i++] = temp[j];
            }
        }
        buf[i] = '\0';
        console_print(buf);
        console_print("\n");
    }
    
    if (bootlog_data->heads > 0) {
        console_print("  Heads:         ");
        unsigned int h = bootlog_data->heads;
        i = 0;
        if (h == 0) {
            buf[i++] = '0';
        } else {
            char temp[16];
            int ti = 0;
            while (h > 0) {
                temp[ti++] = '0' + (h % 10);
                h /= 10;
            }
            for (int j = ti - 1; j >= 0; j--) {
                buf[i++] = temp[j];
            }
        }
        buf[i] = '\0';
        console_print(buf);
        console_print("\n");
    }
    
    if (bootlog_data->sectors_per_track > 0) {
        console_print("  Sectors/track: ");
        unsigned int spt = bootlog_data->sectors_per_track;
        i = 0;
        if (spt == 0) {
            buf[i++] = '0';
        } else {
            char temp[16];
            int ti = 0;
            while (spt > 0) {
                temp[ti++] = '0' + (spt % 10);
                spt /= 10;
            }
            for (int j = ti - 1; j >= 0; j--) {
                buf[i++] = temp[j];
            }
        }
        buf[i] = '\0';
        console_print(buf);
        console_print("\n");
    }
    
    if (bootlog_data->bios_vendor[0] != '\0') {
        console_print("  BIOS vendor:   ");
        console_print(bootlog_data->bios_vendor);
        console_print("\n");
    }
    
    // Check if console output was buffered (text-only mode)
    if (!console_is_enabled()) {
        console_print("  Console mode:  Text-only (buffered output)\n");
        char buf[32];
        int buflen = console_buffer_get(buf, sizeof(buf) - 1);
        if (buflen > 0) {
            console_print("  Buffered output available via 'bootlog' command\n");
        }
    } else {
        console_print("  Console mode:  Video enabled\n");
    }
}
