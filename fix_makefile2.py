#!/usr/bin/env python3

with open('Makefile', 'r') as f:
    lines = f.readlines()

# Find the line with uefi_loader.o rule
insert_pos = None
for i, line in enumerate(lines):
    if '$(BUILD_DIR)/uefi_loader.o: bootloader/uefi_loader.c dirs' in line:
        insert_pos = i
        break

if insert_pos:
    # Insert new rules before uefi_loader
    new_rules = [
        '\n',
        '$(BUILD_DIR)/tui.o: lib/tui.c dirs\n',
        '\t$(CC) $(CFLAGS) -c -o $@ $<\n',
        '\n',
        '$(BUILD_DIR)/partition_table.o: lib/partition_table.c dirs\n',
        '\t$(CC) $(CFLAGS) -c -o $@ $<\n',
        '\n',
        '$(BUILD_DIR)/fat32_format.o: lib/fat32_format.c dirs\n',
        '\t$(CC) $(CFLAGS) -c -o $@ $<\n',
        '\n',
        '$(BUILD_DIR)/ext2_format.o: lib/ext2_format.c dirs\n',
        '\t$(CC) $(CFLAGS) -c -o $@ $<\n',
        '\n',
        '$(BUILD_DIR)/installer.o: apps/installer/installer.c dirs\n',
        '\t$(CC) $(CFLAGS) -c -o $@ $<\n',
    ]
    
    lines = lines[:insert_pos] + new_rules + lines[insert_pos:]
    
    with open('Makefile', 'w') as f:
        f.writelines(lines)
    
    print("Makefile updated successfully")
else:
    print("Could not find insertion point")
