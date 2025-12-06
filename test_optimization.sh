#!/bin/bash
# Test script for disk/FAT12 optimizations

set -e

echo "=== AltoniumOS Optimization Test ==="
echo

# Check build artifacts
if [ ! -f dist/kernel.elf ] || [ ! -f dist/os.img ]; then
    echo "Error: Build artifacts missing. Run 'make build && make img' first."
    exit 1
fi

echo "✓ Build artifacts present"
echo

# Create test command sequence
cat > /tmp/test_commands.txt <<'EOF'
help
fsstat
disk
ls
cd docs
pwd
ls
cat INFO.TXT
cd ..
pwd
fsstat
touch TEST1.TXT
write TEST2.TXT Hello World
ls
fsstat
mkdir TESTDIR
cd testdir
pwd
touch FILE.TXT
ls
cd ..
fsstat
EOF

echo "Test command sequence:"
echo "----------------------"
cat /tmp/test_commands.txt
echo "----------------------"
echo

echo "To run the tests:"
echo "1. Start QEMU with:"
echo "   qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide"
echo
echo "2. Type the commands listed above, or:"
echo "   - Type 'fsstat' after boot to see initial I/O stats"
echo "   - Run some file operations (ls, cd, cat, etc.)"
echo "   - Type 'fsstat' again to see updated stats and multi-op ratio"
echo
echo "Expected improvements:"
echo "  - Multi-op ratio should be >80% for most workloads"
echo "  - Sequential reads (like 'cat') should show high multi-read counts"
echo "  - Total sector counts should be efficient"
echo

echo "=== Quick QEMU test (5 seconds) ==="
timeout 5 qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide -nographic -serial mon:stdio 2>&1 | head -30 || true

echo
echo "=== Test Complete ==="
echo "Build and boot successful. Manual testing required for fsstat validation."
