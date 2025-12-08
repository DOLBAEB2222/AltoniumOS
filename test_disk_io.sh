#!/bin/bash

echo "=== Testing AltoniumOS Storage HAL ==="
echo

# Check if kernel is built
if [ ! -f "dist/kernel.elf" ]; then
    echo "Error: dist/kernel.elf not found. Run 'make build' first."
    exit 1
fi

# Create test disks
echo "Creating test disks..."
dd if=/dev/zero of=test_ata.img bs=512 count=1024 2>/dev/null
dd if=/dev/zero of=test_ahci.img bs=512 count=1024 2>/dev/null
dd if=/dev/zero of=test_nvme.img bs=512 count=1024 2>/dev/null

# Test 1: Legacy ATA boot (existing functionality)
echo ""
echo "=== Test 1: Legacy IDE/ATA Boot ==="
echo "Testing with legacy IDE disk..."
timeout 10 qemu-system-i386 -kernel dist/kernel.elf \
    -hda test_ata.img \
    -nographic \
    -serial mon:stdio << EOF
storage
quit
EOF

# Test 2: AHCI controller
echo ""
echo "=== Test 2: AHCI Controller Boot ==="
echo "Testing with AHCI controller..."
timeout 10 qemu-system-i386 -kernel dist/kernel.elf \
    -drive if=ahci,file=test_ahci.img \
    -nographic \
    -serial mon:stdio << EOF
storage
quit
EOF

# Test 3: NVMe (NVMe support is limited in QEMU with i386 kernels)
echo ""
echo "=== Test 3: NVMe Controller Detection ==="
echo "Testing with NVMe device..."
timeout 10 qemu-system-i386 -kernel dist/kernel.elf \
    -device nvme,serial=NVMe001 \
    -nographic \
    -serial mon:stdio << EOF
storage
quit
EOF

# Test 4: Combined with IDE
echo ""
echo "=== Test 4: Multiple Storage Devices ==="
echo "Testing with both IDE and AHCI..."
timeout 10 qemu-system-i386 -kernel dist/kernel.elf \
    -hda test_ata.img \
    -drive if=ahci,file=test_ahci.img \
    -nographic \
    -serial mon:stdio << EOF
storage
help
quit
EOF

# Clean up
rm -f test_ata.img test_ahci.img test_nvme.img

echo ""
echo "=== Storage HAL Tests Completed ==="
echo "If you saw storage device listings, the detection and initialization are working!"
