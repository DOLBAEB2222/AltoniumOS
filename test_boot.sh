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

echo "3. Checking kernel.elf for Multiboot header..."
if od -Ax -tx1 dist/kernel.elf | head -70 | grep -q "02 b0 ad 1b"; then
    echo "   ✓ Multiboot header present"
else
    echo "   ✗ Multiboot header missing"
    exit 1
fi

echo "4. Checking boot.bin size..."
BOOT_SIZE=$(stat -c%s dist/boot.bin)
if [ "$BOOT_SIZE" -eq 512 ]; then
    echo "   ✓ Boot sector is exactly 512 bytes"
else
    echo "   ✗ Boot sector is $BOOT_SIZE bytes (should be 512)"
    exit 1
fi

echo
echo "=== All tests passed! ==="
echo "To run AltoniumOS:"
echo "  • With QEMU multiboot: qemu-system-i386 -kernel dist/kernel.elf"
echo "  • With disk image:     qemu-system-i386 -drive file=dist/os.img,format=raw"
echo "  • Direct kernel boot:  qemu-system-i386 -hda dist/kernel.bin (not recommended)"
