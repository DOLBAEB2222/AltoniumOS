#include "../include/lib/string.h"

void console_putchar(char c);

int strcmp_impl(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

int strncmp_impl(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!a[i] || !b[i] || a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}

void strcpy_impl(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

size_t strlen_impl(const char *str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

int string_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return 0;
    }
    size_t needle_len = strlen_impl(needle);
    if (needle_len == 0) {
        return 0;
    }
    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (haystack[i + j] != '\0' && needle[j] != '\0' && haystack[i + j] == needle[j]) {
            j++;
        }
        if (j == needle_len) {
            return 1;
        }
    }
    return 0;
}

const char *skip_whitespace(const char *str) {
    if (!str) {
        return str;
    }
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

int read_token(const char **input, char *dest, int max_len) {
    if (!input || !dest || max_len <= 0) {
        return 0;
    }
    const char *ptr = skip_whitespace(*input);
    if (!ptr || *ptr == '\0') {
        return 0;
    }
    int len = 0;
    while (*ptr && *ptr != ' ' && *ptr != '\t') {
        if (len < max_len - 1) {
            dest[len++] = *ptr;
        }
        ptr++;
    }
    dest[len] = '\0';
    *input = ptr;
    return len;
}

int copy_path_argument(const char *input, char *dest, size_t max_len) {
    if (!input || !dest || max_len == 0) {
        if (dest && max_len > 0) {
            dest[0] = '\0';
        }
        return 0;
    }
    size_t write = 0;
    while (*input && *input != '\n' && *input != '\r') {
        if (write + 1 >= max_len) {
            dest[write] = '\0';
            return -1;
        }
        dest[write++] = *input++;
    }
    while (write > 0 && (dest[write - 1] == ' ' || dest[write - 1] == '\t')) {
        write--;
    }
    dest[write] = '\0';
    return (int)write;
}

void print_unsigned(uint32_t value) {
    char buffer[16];
    int pos = 0;
    if (value == 0) {
        console_putchar('0');
        return;
    }
    while (value > 0 && pos < (int)sizeof(buffer)) {
        buffer[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        console_putchar(buffer[i]);
    }
}

void print_decimal(int value) {
    uint32_t magnitude;
    if (value < 0) {
        console_putchar('-');
        magnitude = (uint32_t)(- (value + 1)) + 1;
    } else {
        magnitude = (uint32_t)value;
    }
    print_unsigned(magnitude);
}

int string_length(const char *str) {
    return (int)strlen_impl(str);
}

void string_copy(char *dest, const char *src) {
    strcpy_impl(dest, src);
}

void string_concat(char *dest, const char *src) {
    char *end = dest;
    while (*end) end++;
    while (*src) {
        *end++ = *src++;
    }
    *end = '\0';
}

void string_memset(uint8_t *dest, uint8_t value, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = value;
    }
}

void string_memcpy(uint8_t *dest, const uint8_t *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}
