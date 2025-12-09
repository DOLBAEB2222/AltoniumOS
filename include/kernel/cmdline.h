#ifndef KERNEL_CMDLINE_H
#define KERNEL_CMDLINE_H

// Parse kernel command line and extract video mode flag
// Returns 1 if video=text is specified, 0 if novideo is specified, -1 if not found
int parse_video_mode(const char *cmdline);

// Helper function to check if string starts with a prefix
int string_starts_with(const char *str, const char *prefix);

#endif