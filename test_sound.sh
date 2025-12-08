#!/bin/bash

set -e

echo "Testing PC Speaker Audio..."

make clean > /dev/null 2>&1
make build > /dev/null 2>&1

timeout 20s qemu-system-i386 -kernel dist/kernel.elf -serial mon:stdio << 'EOF' > /tmp/qemu_output.log 2>&1 || true
beep 440 200
beep melody C4:250,E4:250,G4:500
beep piano
\e
EOF

echo "Checking for expected outputs..."

if grep -q "Beep: frequency=440Hz, duration=200ms" /tmp/qemu_output.log; then
    echo "✓ Single tone test passed"
else
    echo "✗ Single tone test failed"
    exit 1
fi

if grep -q "Playing melody with 3 notes.*C4.*E4.*G4" /tmp/qemu_output.log; then
    echo "✓ Melody test passed"
else
    echo "✗ Melody test failed"
    exit 1
fi

if grep -q "Piano mode started" /tmp/qemu_output.log; then
    echo "✓ Piano mode test passed"
else
    echo "✗ Piano mode test failed"
    exit 1
fi

if grep -q "Exiting piano mode" /tmp/qemu_output.log; then
    echo "✓ Piano exit test passed"
else
    echo "✗ Piano exit test failed"
    exit 1
fi

echo "All audio tests passed!"

make clean > /dev/null 2>&1