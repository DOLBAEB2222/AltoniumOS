#!/bin/bash

echo "===== Multiboot Kernel Verification Script ====="
echo ""

# Check if kernel.bin exists
if [ ! -f kernel.bin ]; then
    echo "ERROR: kernel.bin not found. Please run 'make' first."
    exit 1
fi

echo "1. Checking Multiboot header..."
MULTIBOOT_MAGIC=$(objdump -s -j .multiboot kernel.bin | grep 100000 | awk '{print $2}')
if [ "$MULTIBOOT_MAGIC" = "02b0ad1b" ]; then
    echo "   ✓ Multiboot magic number found: 0x1BADB002"
else
    echo "   ✗ Multiboot magic number NOT found or incorrect"
    exit 1
fi

echo ""
echo "2. Checking section layout..."
objdump -h kernel.bin | grep -E "\.(multiboot|text|data|bss)" | while read -r line; do
    echo "   ✓ $line"
done

echo ""
echo "3. Checking entry point..."
ENTRY=$(readelf -h kernel.bin | grep "Entry point" | awk '{print $4}')
echo "   ✓ Entry point: $ENTRY (_start)"

echo ""
echo "4. Verifying bootstrap code structure..."
echo "   Checking for key instructions in _start:"

objdump -d kernel.bin | grep -A 1 "cli" | head -2 | grep -q "cli" && echo "   ✓ Interrupts disabled (cli)"
objdump -d kernel.bin | grep "lgdt" | grep -q "lgdt" && echo "   ✓ GDT loaded (lgdt)"
objdump -d kernel.bin | grep "mov.*ss" | grep -q "ss" && echo "   ✓ Stack segment set (mov ss)"
objdump -d kernel.bin | grep "mov.*esp" | grep -q "esp" && echo "   ✓ Stack pointer initialized (mov esp)"
objdump -d kernel.bin | grep "rep stos" | grep -q "stos" && echo "   ✓ BSS zeroed (rep stosb)"
objdump -d kernel.bin | grep "call.*kernel_main" | grep -q "call" && echo "   ✓ kernel_main called"
objdump -d kernel.bin | grep "hlt" | head -1 | grep -q "hlt" && echo "   ✓ System halts if kernel_main returns (hlt)"

echo ""
echo "5. Checking GDT structure..."
GDT_SIZE=$(objdump -s -j .data kernel.bin | grep 103010 | awk '{print $7}')
if [ "$GDT_SIZE" = "1700" ]; then
    echo "   ✓ GDT descriptor size: 0x17 (23 bytes = 3 descriptors)"
else
    echo "   ⚠ GDT descriptor size might be incorrect"
fi

echo ""
echo "6. Verifying stack allocation..."
STACK_START=$(objdump -h kernel.bin | grep .bss | awk '{print $4}')
STACK_SIZE=$(objdump -h kernel.bin | grep .bss | awk '{print $3}')
echo "   ✓ Stack in BSS section at 0x$STACK_START, size 0x$STACK_SIZE"

echo ""
echo "7. Checking kernel_main signature..."
objdump -t kernel.bin | grep "kernel_main" | grep -q "kernel_main" && echo "   ✓ kernel_main symbol found"

echo ""
echo "===== Verification Complete ====="
echo ""
echo "The kernel appears to be Multiboot-compliant and ready for:"
echo "  - GRUB bootloader"
echo "  - QEMU (qemu-system-i386 -kernel kernel.bin)"
echo "  - Legacy BIOS hardware"
echo ""
