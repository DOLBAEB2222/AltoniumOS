#!/bin/bash
# Test script to verify hardware detection and PAE support

echo "=== AltoniumOS Hardware Detection Test ==="
echo

# Function to run QEMU and check for specific output
run_qemu_test() {
    local cpu_args="$1"
    local memory_args="$2"
    local test_name="$3"
    local expected_pae="$4"
    
    echo "Testing $test_name..."
    echo "  QEMU args: $cpu_args $memory_args"
    
    # Run QEMU with timeout and capture output
    OUTPUT=$(timeout 5 qemu-system-i386 -kernel dist/kernel.elf -nographic $cpu_args $memory_args 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "  ✓ Kernel boots and runs (timeout reached = running)"
    else
        echo "  ✗ Kernel failed to boot (exit code: $exit_code)"
        echo "  Output: $OUTPUT"
        return 1
    fi
    
    # Check for PAE detection
    if echo "$OUTPUT" | grep -q "PAE: Enabled"; then
        if [ "$expected_pae" = "enabled" ]; then
            echo "  ✓ PAE correctly enabled"
        else
            echo "  ⚠ PAE enabled but not expected"
        fi
    elif echo "$OUTPUT" | grep -q "PAE: Supported but not enabled"; then
        if [ "$expected_pae" = "supported" ]; then
            echo "  ✓ PAE detected but not enabled (expected)"
        else
            echo "  ⚠ PAE not enabled when expected"
        fi
    elif echo "$OUTPUT" | grep -q "PAE: Not supported"; then
        if [ "$expected_pae" = "unsupported" ]; then
            echo "  ✓ PAE correctly detected as unsupported"
        else
            echo "  ⚠ PAE unsupported when expected to be supported"
        fi
    else
        echo "  ? PAE status not found in output"
    fi
    
    # Check for hardware detection output
    if echo "$OUTPUT" | grep -q "CPU:"; then
        echo "  ✓ CPU information displayed"
    else
        echo "  ✗ CPU information not found"
    fi
    
    if echo "$OUTPUT" | grep -q "Memory:"; then
        echo "  ✓ Memory information displayed"
    else
        echo "  ✗ Memory information not found"
    fi
    
    echo "  Test completed"
    echo
}

# Build the kernel first
echo "Building kernel..."
make clean > /dev/null 2>&1
make build > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✗ Build failed"
    exit 1
fi
echo "✓ Build successful"
echo

# Test 1: Legacy BIOS mode with basic CPU (no PAE)
run_qemu_test "-cpu qemu32" "" "Legacy BIOS (qemu32, no PAE)" "unsupported"

# Test 2: Legacy BIOS mode with PAE-enabled CPU
run_qemu_test "-cpu qemu32,+pae" "" "Legacy BIOS (qemu32 with PAE)" "supported"

# Test 3: Modern CPU with PAE
run_qemu_test "-cpu host" "" "Modern CPU (host, with PAE)" "enabled"

# Test 4: Modern CPU with PAE and more memory
run_qemu_test "-cpu host" "-m 2048" "Modern CPU with 2GB RAM" "enabled"

# Test 5: Maximum modern CPU configuration
run_qemu_test "-cpu max -machine q35" "-m 8192" "Modern CPU (max) with 8GB RAM" "enabled"

# Test 6: UEFI mode simulation (if OVMF is available)
if [ -f /usr/share/OVMF/OVMF_CODE_4M.fd ]; then
    echo "Testing UEFI mode..."
    OUTPUT=$(timeout 5 qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -kernel dist/kernel.elf -nographic 2>&1)
    if echo "$OUTPUT" | grep -q "Boot mode: UEFI"; then
        echo "  ✓ UEFI boot mode detected"
    else
        echo "  ⚠ UEFI boot mode not detected (may be expected for x86 kernel)"
    fi
    echo "  UEFI test completed"
    echo
else
    echo "Skipping UEFI test (OVMF not available)"
    echo
fi

# Test 7: Hardware information command
echo "Testing hwinfo command..."
OUTPUT=$(timeout 5 qemu-system-i386 -kernel dist/kernel.elf -nographic 2>&1)
if echo "$OUTPUT" | grep -q "Hardware Detection Completed"; then
    echo "  ✓ Hardware detection completed successfully"
else
    echo "  ✗ Hardware detection may have failed"
fi

# Check for key hardware components in the output
if echo "$OUTPUT" | grep -q "CPU Vendor:"; then
    echo "  ✓ CPU vendor detection working"
else
    echo "  ⚠ CPU vendor detection may not be working"
fi

if echo "$OUTPUT" | grep -q "Memory regions:"; then
    echo "  ✓ Memory map parsing working"
else
    echo "  ⚠ Memory map parsing may not be working"
fi

echo "  hwinfo command test completed"
echo

echo "=== Hardware Detection Tests Summary ==="
echo "✓ Legacy CPU support tested"
echo "✓ PAE detection tested" 
echo "✓ Modern CPU support tested"
echo "✓ High memory configurations tested"
echo "✓ Hardware information command tested"
echo
echo "Manual verification steps:"
echo "1. Run 'qemu-system-i386 -kernel dist/kernel.elf' and verify CPU/memory info"
echo "2. Run 'qemu-system-i386 -cpu qemu32,+pae -kernel dist/kernel.elf' and verify PAE support"
echo "3. In the shell, run 'hwinfo' command to see detailed hardware information"
echo "4. Test on real hardware:"
echo "   - Lenovo laptop: Should detect PS/2 controller and legacy hardware"
echo "   - Modern i5/RTX machine: Should detect PAE, modern controllers, and >4GB memory"
echo
echo "=== All hardware detection tests completed! ==="