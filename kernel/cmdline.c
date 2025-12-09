#include "../include/kernel/cmdline.h"

// Parse kernel command line and extract video mode flag
// Returns 1 if video=text is specified, 0 if novideo is specified, -1 if not found
int parse_video_mode(const char *cmdline) {
    if (!cmdline) return -1;
    
    // Look for "video=text" or "novideo" flags
    const char *p = cmdline;
    while (*p) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') p++;
        
        if (string_starts_with(p, "video=text")) {
            return 1; // Text mode requested
        } else if (string_starts_with(p, "novideo")) {
            return 0; // No video requested
        }
        
        // Move to next token
        while (*p && *p != ' ' && *p != '\t') p++;
    }
    
    return -1; // No video flag found
}

// Helper function to check if string starts with a prefix
int string_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}