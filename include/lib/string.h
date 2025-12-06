#ifndef LIB_STRING_H
#define LIB_STRING_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long size_t;
typedef unsigned long uintptr_t;
typedef int ssize_t;

int strcmp_impl(const char *a, const char *b);
int strncmp_impl(const char *a, const char *b, size_t n);
void strcpy_impl(char *dest, const char *src);
size_t strlen_impl(const char *str);
int string_contains(const char *haystack, const char *needle);
const char *skip_whitespace(const char *str);
int read_token(const char **input, char *dest, int max_len);
int copy_path_argument(const char *input, char *dest, size_t max_len);
void print_unsigned(uint32_t value);
void print_decimal(int value);

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a" (data), "Nd" (port));
}

#endif
