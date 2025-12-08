#include "../include/kernel/log.h"
#include "../include/drivers/console.h"
#include "../fat12.h"

static kernel_log_t klog;

static const char *level_strings[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

void klog_init(void) {
    for (uint32_t i = 0; i < LOG_BUFFER_SIZE; i++) {
        klog.buffer[i] = '\0';
    }
    klog.write_pos = 0;
    klog.read_pos = 0;
    klog.wrapped = 0;
    klog.filesystem_ready = 0;
}

void klog_set_filesystem_ready(int ready) {
    klog.filesystem_ready = ready;
    if (ready) {
        klog_flush_to_disk();
    }
}

static void klog_append_char(char c) {
    klog.buffer[klog.write_pos] = c;
    klog.write_pos = (klog.write_pos + 1) % LOG_BUFFER_SIZE;
    if (klog.write_pos == klog.read_pos) {
        klog.read_pos = (klog.read_pos + 1) % LOG_BUFFER_SIZE;
        klog.wrapped = 1;
    }
}

static void klog_append_string(const char *str) {
    while (*str) {
        klog_append_char(*str);
        str++;
    }
}

void klog_write(int level, const char *subsystem, const char *message) {
    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_ERROR) {
        level = LOG_LEVEL_INFO;
    }
    
    klog_append_char('[');
    klog_append_string(level_strings[level]);
    klog_append_char(']');
    klog_append_char(' ');
    
    if (subsystem) {
        klog_append_string(subsystem);
        klog_append_char(':');
        klog_append_char(' ');
    }
    
    if (message) {
        klog_append_string(message);
    }
    
    klog_append_char('\n');
}

void klog_flush_to_disk(void) {
    if (!klog.filesystem_ready) {
        return;
    }
    
    int result = fat12_create_directory("/VAR");
    if (result != FAT12_OK && result != FAT12_ERR_ALREADY_EXISTS) {
        return;
    }
    
    result = fat12_create_directory("/VAR/LOG");
    if (result != FAT12_OK && result != FAT12_ERR_ALREADY_EXISTS) {
        return;
    }
    
    char log_buffer[LOG_BUFFER_SIZE];
    uint32_t size = klog_get_buffer(log_buffer, LOG_BUFFER_SIZE);
    
    if (size > 0) {
        fat12_write_file("/VAR/LOG/BOOT.LOG", (uint8_t *)log_buffer, size);
    }
}

int klog_get_buffer(char *dest, uint32_t max_size) {
    if (!dest || max_size == 0) {
        return 0;
    }
    
    uint32_t copied = 0;
    uint32_t pos = klog.wrapped ? klog.read_pos : 0;
    uint32_t end = klog.write_pos;
    
    while (pos != end && copied < max_size - 1) {
        dest[copied++] = klog.buffer[pos];
        pos = (pos + 1) % LOG_BUFFER_SIZE;
    }
    
    dest[copied] = '\0';
    return copied;
}

void klog_print_buffer(void) {
    uint32_t pos = klog.wrapped ? klog.read_pos : 0;
    uint32_t end = klog.write_pos;
    
    console_print("=== Kernel Log Buffer ===\n");
    
    while (pos != end) {
        console_putchar(klog.buffer[pos]);
        pos = (pos + 1) % LOG_BUFFER_SIZE;
    }
    
    console_print("=== End of Log ===\n");
}
