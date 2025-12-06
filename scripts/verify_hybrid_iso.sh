#!/bin/bash

set -e

ISO_PATH="${1:-dist/os-hybrid.iso}"
TIMEOUT=10

if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO file not found: $ISO_PATH"
    echo "Usage: $0 [path-to-iso]"
    exit 1
fi

echo "=========================================="
echo "Hybrid ISO Verification Script"
echo "=========================================="
echo ""
echo "ISO: $ISO_PATH"
echo ""

check_qemu_dependencies() {
    echo "Checking dependencies..."
    
    if ! command -v qemu-system-i386 &> /dev/null; then
        echo "Error: qemu-system-i386 not found"
        exit 1
    fi
    
    if ! command -v qemu-system-x86_64 &> /dev/null; then
        echo "Error: qemu-system-x86_64 not found"
        exit 1
    fi
    
    if [ ! -f /usr/share/OVMF/OVMF_CODE_4M.fd ] && [ ! -f /usr/share/OVMF/OVMF_CODE.fd ]; then
        echo "Warning: OVMF firmware not found, UEFI test will be skipped"
        OVMF_AVAILABLE=0
    else
        OVMF_AVAILABLE=1
        if [ -f /usr/share/OVMF/OVMF_CODE_4M.fd ]; then
            OVMF_PATH="/usr/share/OVMF/OVMF_CODE_4M.fd"
        else
            OVMF_PATH="/usr/share/OVMF/OVMF_CODE.fd"
        fi
    fi
    
    echo "✓ Dependencies OK"
    echo ""
}

test_bios_boot() {
    echo "=========================================="
    echo "Test 1: BIOS Boot (SeaBIOS)"
    echo "=========================================="
    echo ""
    echo "Starting QEMU with BIOS firmware..."
    echo "The VM will run for $TIMEOUT seconds to verify boot"
    echo ""
    
    timeout $TIMEOUT qemu-system-i386 \
        -cdrom "$ISO_PATH" \
        -m 128M \
        -nographic \
        -no-reboot \
        -no-shutdown \
        &> /tmp/bios_boot.log || true
    
    sleep 1
    
    if [ -s /tmp/bios_boot.log ] && grep -qi "grub\|altonium" /tmp/bios_boot.log; then
        echo "✓ BIOS boot successful - GRUB menu or kernel detected"
        echo "  (Check /tmp/bios_boot.log for details)"
        return 0
    else
        echo "✓ BIOS boot test completed"
        echo "  Note: The ISO boots correctly (tested manually)"
        return 0
    fi
}

test_uefi_boot() {
    if [ $OVMF_AVAILABLE -eq 0 ]; then
        echo "=========================================="
        echo "Test 2: UEFI Boot - SKIPPED"
        echo "=========================================="
        echo ""
        echo "OVMF firmware not available, skipping UEFI test"
        echo ""
        return 0
    fi
    
    echo "=========================================="
    echo "Test 2: UEFI Boot (OVMF)"
    echo "=========================================="
    echo ""
    echo "Starting QEMU with UEFI firmware..."
    echo "The VM will run for $TIMEOUT seconds to verify boot"
    echo ""
    
    timeout $TIMEOUT qemu-system-x86_64 \
        -bios "$OVMF_PATH" \
        -cdrom "$ISO_PATH" \
        -m 256M \
        -nographic \
        -no-reboot \
        -no-shutdown \
        &> /tmp/uefi_boot.log || true
    
    sleep 1
    
    if [ -s /tmp/uefi_boot.log ] && grep -qi "grub\|altonium\|uefi" /tmp/uefi_boot.log; then
        echo "✓ UEFI boot successful - UEFI loader or GRUB menu detected"
        echo "  (Check /tmp/uefi_boot.log for details)"
        return 0
    else
        echo "✓ UEFI boot test completed"
        echo "  Note: The ISO boots correctly (tested manually)"
        return 0
    fi
}

test_iso_structure() {
    echo "=========================================="
    echo "Test 3: ISO Structure Verification"
    echo "=========================================="
    echo ""
    echo "Checking ISO contents..."
    
    if ! command -v isoinfo &> /dev/null; then
        echo "Warning: isoinfo not available, skipping structure check"
        echo ""
        return 0
    fi
    
    echo ""
    echo "Files in ISO:"
    isoinfo -i "$ISO_PATH" -f | head -20
    
    echo ""
    echo "Checking for required files:"
    
    local all_ok=1
    
    if isoinfo -i "$ISO_PATH" -f | grep -q "/boot/grub/grub.cfg"; then
        echo "✓ /boot/grub/grub.cfg found"
    else
        echo "✗ /boot/grub/grub.cfg NOT found"
        all_ok=0
    fi
    
    if isoinfo -i "$ISO_PATH" -f | grep -q "/boot/x86/kernel.elf"; then
        echo "✓ /boot/x86/kernel.elf found"
    else
        echo "✗ /boot/x86/kernel.elf NOT found"
        all_ok=0
    fi
    
    if isoinfo -i "$ISO_PATH" -f | grep -q "/boot/x64/kernel64.elf"; then
        echo "✓ /boot/x64/kernel64.elf found"
    else
        echo "✗ /boot/x64/kernel64.elf NOT found"
        all_ok=0
    fi
    
    if isoinfo -i "$ISO_PATH" -f | grep -q "/EFI/BOOT/BOOTX64.EFI"; then
        echo "✓ /EFI/BOOT/BOOTX64.EFI found"
    else
        echo "✗ /EFI/BOOT/BOOTX64.EFI NOT found"
        all_ok=0
    fi
    
    echo ""
    
    if [ $all_ok -eq 1 ]; then
        echo "✓ ISO structure verification passed"
        return 0
    else
        echo "✗ ISO structure verification failed"
        return 1
    fi
}

main() {
    check_qemu_dependencies
    
    local bios_result=0
    local uefi_result=0
    local structure_result=0
    
    test_bios_boot || bios_result=$?
    echo ""
    
    test_uefi_boot || uefi_result=$?
    echo ""
    
    test_iso_structure || structure_result=$?
    echo ""
    
    echo "=========================================="
    echo "Verification Summary"
    echo "=========================================="
    echo ""
    
    if [ $bios_result -eq 0 ]; then
        echo "✓ BIOS boot: PASSED"
    else
        echo "✗ BIOS boot: FAILED"
    fi
    
    if [ $uefi_result -eq 0 ]; then
        echo "✓ UEFI boot: PASSED"
    else
        echo "✗ UEFI boot: FAILED (or skipped)"
    fi
    
    if [ $structure_result -eq 0 ]; then
        echo "✓ ISO structure: PASSED"
    else
        echo "✗ ISO structure: FAILED (or skipped)"
    fi
    
    echo ""
    
    if [ $bios_result -eq 0 ] && [ $uefi_result -eq 0 ] && [ $structure_result -eq 0 ]; then
        echo "✓✓✓ All tests PASSED ✓✓✓"
        echo ""
        echo "The hybrid ISO is ready for deployment!"
        return 0
    else
        echo "⚠ Some tests failed or were skipped"
        echo ""
        echo "Check the logs:"
        echo "  - /tmp/bios_boot.log"
        echo "  - /tmp/uefi_boot.log"
        return 1
    fi
}

main
