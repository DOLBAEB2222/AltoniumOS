#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include "../lib/string.h"

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

#define LOG_BUFFER_SIZE 4096

typedef struct {
    char buffer[LOG_BUFFER_SIZE];
    uint32_t write_pos;
    uint32_t read_pos;
    uint32_t wrapped;
    int filesystem_ready;
} kernel_log_t;

void klog_init(void);
void klog_set_filesystem_ready(int ready);
void klog_write(int level, const char *subsystem, const char *message);
void klog_flush_to_disk(void);
void klog_print_buffer(void);
int klog_get_buffer(char *dest, uint32_t max_size);

#define KLOG_DEBUG(subsys, msg) klog_write(LOG_LEVEL_DEBUG, subsys, msg)
#define KLOG_INFO(subsys, msg)  klog_write(LOG_LEVEL_INFO, subsys, msg)
#define KLOG_WARN(subsys, msg)  klog_write(LOG_LEVEL_WARN, subsys, msg)
#define KLOG_ERROR(subsys, msg) klog_write(LOG_LEVEL_ERROR, subsys, msg)

#endif
