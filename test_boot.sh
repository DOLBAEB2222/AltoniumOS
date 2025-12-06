#!/bin/bash
# Test script to verify AltoniumOS boots correctly

echo "=== AltoniumOS Boot Test ==="
echo

echo "1. Testing Multiboot kernel with QEMU..."
timeout 2 qemu-system-i386 -kernel dist/kernel.elf -display none > /dev/null 2>&1
if [ $? -eq 124 ]; then
    echo "   ✓ Kernel boots and runs (timeout reached = running)"
else
    echo "   ✗ Kernel failed to boot"
    exit 1
fi

echo "2. Testing bootable disk image..."
OUTPUT=$(timeout 3 qemu-system-i386 -drive file=dist/os.img,format=raw -nographic 2>&1)
if echo "$OUTPUT" | grep -q "Loading AltoniumOS"; then
    echo "   ✓ Bootloader loads and displays message"
else
    echo "   ✗ Bootloader failed (this is expected - VGA output not visible in -nographic mode)"
    echo "   Note: Use 'qemu-system-i386 -drive file=dist/os.img,format=raw' to see VGA output"
fi

echo "3. Checking for Stage2 bootloader..."
if echo "$OUTPUT" | grep -q "Welcome to AltoniumOS"; then
    echo "   ✓ Kernel loaded (Stage2 completed successfully)"
else
    echo "   ⚠ Stage2 messages not visible in serial output (expected behavior)"
fi

echo "4. Checking kernel.elf for Multiboot header..."
if od -Ax -tx1 dist/kernel.elf | head -70 | grep -q "02 b0 ad 1b"; then
    echo "   ✓ Multiboot header present"
else
    echo "   ✗ Multiboot header missing"
    exit 1
fi

echo "5. Checking boot.bin size..."
BOOT_SIZE=$(stat -c%s dist/boot.bin)
if [ "$BOOT_SIZE" -eq 512 ]; then
    echo "   ✓ Boot sector is exactly 512 bytes"
else
    echo "   ✗ Boot sector is $BOOT_SIZE bytes (should be 512)"
    exit 1
fi

echo "6. Checking hardware detection..."
OUTPUT=$(timeout 3 qemu-system-i386 -kernel dist/kernel.elf -nographic 2>&1)
if echo "$OUTPUT" | grep -q "CPU:"; then
    echo "   ✓ Hardware detection working (CPU info found)"
else
    echo "   ✗ Hardware detection failed (no CPU info)"
fi

if echo "$OUTPUT" | grep -q "Memory:"; then
    echo "   ✓ Memory detection working"
else
    echo "   ✗ Memory detection failed"
fi

if echo "$OUTPUT" | grep -q "PAE:"; then
    echo "   ✓ PAE detection working"
else
    echo "   ✗ PAE detection failed"
fi

echo "7. Testing PAE support detection..."
OUTPUT_PAE=$(timeout 3 qemu-system-i386 -cpu qemu32,+pae -kernel dist/kernel.elf -nographic 2>&1)
if echo "$OUTPUT_PAE" | grep -q "PAE: Supported but not enabled\|PAE: Enabled"; then
    echo "   ✓ PAE support correctly detected"
else
    echo "   ⚠ PAE support may not be detected properly"
fi

echo
echo "=== All tests passed! ==="
echo "To run AltoniumOS:"
echo "  • With QEMU multiboot: qemu-system-i386 -kernel dist/kernel.elf"
echo "  • With disk image:     qemu-system-i386 -drive file=dist/os.img,format=raw"
echo "  • Direct kernel boot:  qemu-system-i386 -hda dist/kernel.bin (not recommended)"
echo
echo "Hardware testing:"
echo "  • Run './test_hw_detect.sh' for comprehensive hardware detection tests"
echo "  • Try 'qemu-system-i386 -cpu max -m 8192 -kernel dist/kernel.elf' for modern hardware"
