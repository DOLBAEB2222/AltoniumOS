#!/usr/bin/env python3
"""Generate a FAT12 disk image with the kernel stored in the reserved region."""

import argparse
import struct

BYTES_PER_SECTOR = 512
TOTAL_SECTORS = 20480
RESERVED_SECTORS = 128
NUM_FATS = 2
SECTORS_PER_FAT = 8
ROOT_ENTRIES = 256
ROOT_DIR_SECTORS = 16
SECTORS_PER_CLUSTER = 8
MEDIA_DESCRIPTOR = 0xF8

DATA_START_SECTOR = RESERVED_SECTORS + NUM_FATS * SECTORS_PER_FAT + ROOT_DIR_SECTORS
CLUSTER_SIZE = BYTES_PER_SECTOR * SECTORS_PER_CLUSTER
TOTAL_DATA_SECTORS = TOTAL_SECTORS - RESERVED_SECTORS - NUM_FATS * SECTORS_PER_FAT - ROOT_DIR_SECTORS
TOTAL_CLUSTERS = TOTAL_DATA_SECTORS // SECTORS_PER_CLUSTER
EOC = 0x0FFF


def to_short_name(name: str) -> bytes:
    parts = name.split('.')
    base = parts[0][:8].upper()
    ext = parts[1][:3].upper() if len(parts) > 1 else ''
    base = base.ljust(8, ' ')
    ext = ext.ljust(3, ' ')
    return (base + ext).encode('ascii')


def set_fat_entry(fat: bytearray, cluster: int, value: int) -> None:
    value &= 0x0FFF
    index = cluster + (cluster // 2)
    if cluster % 2 == 0:
        fat[index] = value & 0xFF
        fat[index + 1] = (fat[index + 1] & 0xF0) | ((value >> 8) & 0x0F)
    else:
        fat[index] = (fat[index] & 0x0F) | ((value & 0x0F) << 4)
        fat[index + 1] = (value >> 4) & 0xFF


def build_dir_entry(short_name: bytes, attr: int, first_cluster: int, size: int) -> bytes:
    entry = bytearray(32)
    entry[0:11] = short_name
    entry[11] = attr
    # Remaining time/date fields stay zeroed
    entry[26:28] = struct.pack('<H', first_cluster)
    entry[28:32] = struct.pack('<I', size)
    return bytes(entry)


def cluster_to_offset(cluster: int) -> int:
    return ((cluster - 2) * SECTORS_PER_CLUSTER + DATA_START_SECTOR) * BYTES_PER_SECTOR


def write_cluster(image: bytearray, cluster: int, data: bytes) -> None:
    start = cluster_to_offset(cluster)
    padded = bytearray(CLUSTER_SIZE)
    padded[:len(data)] = data
    image[start:start + CLUSTER_SIZE] = padded


def write_file(image: bytearray, fat: bytearray, allocator, data: bytes) -> int:
    if not data:
        return 0
    clusters_needed = (len(data) + CLUSTER_SIZE - 1) // CLUSTER_SIZE
    clusters = allocator(clusters_needed)
    for i, cluster in enumerate(clusters):
        chunk = data[i * CLUSTER_SIZE:(i + 1) * CLUSTER_SIZE]
        write_cluster(image, cluster, chunk)
    return clusters[0]


def create_allocator(fat: bytearray):
    next_cluster = 2

    def allocate(count: int):
        nonlocal next_cluster
        if count <= 0:
            return []
        clusters = []
        for _ in range(count):
            if next_cluster >= TOTAL_CLUSTERS + 2:
                raise RuntimeError("disk full")
            clusters.append(next_cluster)
            next_cluster += 1
        for idx, cluster in enumerate(clusters):
            value = clusters[idx + 1] if idx + 1 < len(clusters) else EOC
            set_fat_entry(fat, cluster, value)
        return clusters

    return allocate


def build_image(boot_path: str, kernel_path: str, output_path: str, stage2_path: str = None) -> None:
    total_bytes = TOTAL_SECTORS * BYTES_PER_SECTOR
    image = bytearray(total_bytes)

    with open(boot_path, 'rb') as boot_file:
        boot_sector = boot_file.read()
    if len(boot_sector) != BYTES_PER_SECTOR:
        raise RuntimeError("boot sector must be exactly 512 bytes")
    image[0:BYTES_PER_SECTOR] = boot_sector

    # Place stage 2 at sector 1 (offset BYTES_PER_SECTOR)
    if stage2_path:
        with open(stage2_path, 'rb') as stage2_file:
            stage2_data = stage2_file.read()
        if len(stage2_data) > BYTES_PER_SECTOR:
            raise RuntimeError("stage2 must be exactly 512 bytes or less")
        image[BYTES_PER_SECTOR:BYTES_PER_SECTOR + len(stage2_data)] = stage2_data

    # Place kernel at sector 2 (offset 2*BYTES_PER_SECTOR)
    with open(kernel_path, 'rb') as kernel_file:
        kernel_data = kernel_file.read()
    kernel_sectors = (len(kernel_data) + BYTES_PER_SECTOR - 1) // BYTES_PER_SECTOR
    kernel_start_sector = 2 if stage2_path else 1
    if kernel_sectors > RESERVED_SECTORS - kernel_start_sector:
        raise RuntimeError("kernel is too large for reserved area")
    kernel_offset = kernel_start_sector * BYTES_PER_SECTOR
    image[kernel_offset:kernel_offset + len(kernel_data)] = kernel_data

    fat_primary = bytearray(SECTORS_PER_FAT * BYTES_PER_SECTOR)
    fat_primary[0] = MEDIA_DESCRIPTOR
    fat_primary[1] = 0xFF
    fat_primary[2] = 0xFF

    allocator = create_allocator(fat_primary)

    root_dir = bytearray(ROOT_DIR_SECTORS * BYTES_PER_SECTOR)
    root_index = 0

    def add_root_entry(name: str, attr: int, first_cluster: int, size: int):
        nonlocal root_index
        if root_index >= ROOT_ENTRIES:
            raise RuntimeError("Root directory full while seeding image")
        entry = build_dir_entry(to_short_name(name), attr, first_cluster, size)
        offset = root_index * 32
        root_dir[offset:offset + 32] = entry
        root_index += 1

    readme_text = (
        b"Welcome to AltoniumOS FAT12 volume!\r\n"
        b"Use 'ls', 'cat', and 'write' inside the kernel shell.\r\n"
    )
    readme_cluster = write_file(image, fat_primary, allocator, readme_text)
    add_root_entry("README.TXT", 0x20, readme_cluster, len(readme_text))

    config_text = b"boot=altoniumos\nversion=1.0\nfat12=enabled\n"
    config_cluster = write_file(image, fat_primary, allocator, config_text)
    add_root_entry("SYSTEM.CFG", 0x20, config_cluster, len(config_text))

    dot_name = (b'.' + b' ' * 7) + (b' ' * 3)
    dotdot_name = (b'..' + b' ' * 6) + (b' ' * 3)
    docs_cluster = allocator(1)[0]
    docs_buffer = bytearray(CLUSTER_SIZE)
    docs_buffer[0:32] = build_dir_entry(dot_name, 0x10, docs_cluster, 0)
    docs_buffer[32:64] = build_dir_entry(dotdot_name, 0x10, 0, 0)

    info_text = b"Sample notes live inside DOCS/INFO.TXT\r\n"
    info_cluster = write_file(image, fat_primary, allocator, info_text)
    docs_buffer[64:96] = build_dir_entry(to_short_name("INFO.TXT"), 0x20, info_cluster, len(info_text))
    write_cluster(image, docs_cluster, docs_buffer)
    add_root_entry("DOCS", 0x10, docs_cluster, 0)

    var_cluster = allocator(1)[0]
    var_buffer = bytearray(CLUSTER_SIZE)
    var_buffer[0:32] = build_dir_entry(dot_name, 0x10, var_cluster, 0)
    var_buffer[32:64] = build_dir_entry(dotdot_name, 0x10, 0, 0)

    log_cluster = allocator(1)[0]
    log_buffer = bytearray(CLUSTER_SIZE)
    log_buffer[0:32] = build_dir_entry(dot_name, 0x10, log_cluster, 0)
    log_buffer[32:64] = build_dir_entry(dotdot_name, 0x10, var_cluster, 0)
    write_cluster(image, log_cluster, log_buffer)

    var_buffer[64:96] = build_dir_entry(to_short_name("LOG"), 0x10, log_cluster, 0)
    write_cluster(image, var_cluster, var_buffer)
    add_root_entry("VAR", 0x10, var_cluster, 0)

    # Write FATs
    fat_offset = RESERVED_SECTORS * BYTES_PER_SECTOR
    image[fat_offset:fat_offset + len(fat_primary)] = fat_primary
    if NUM_FATS > 1:
        second_offset = fat_offset + SECTORS_PER_FAT * BYTES_PER_SECTOR
        image[second_offset:second_offset + len(fat_primary)] = fat_primary

    # Write root directory
    root_offset = (RESERVED_SECTORS + NUM_FATS * SECTORS_PER_FAT) * BYTES_PER_SECTOR
    image[root_offset:root_offset + len(root_dir)] = root_dir

    with open(output_path, 'wb') as output_file:
        output_file.write(image)


def main() -> None:
    parser = argparse.ArgumentParser(description="Build FAT12 disk image")
    parser.add_argument('--boot', required=True, help='Path to boot sector binary')
    parser.add_argument('--stage2', required=False, help='Path to stage2 bootloader binary')
    parser.add_argument('--kernel', required=True, help='Path to kernel binary')
    parser.add_argument('--output', required=True, help='Output disk image path')
    args = parser.parse_args()

    build_image(args.boot, args.kernel, args.output, args.stage2)
    print(f"FAT12 image written to {args.output}")


if __name__ == '__main__':
    main()
