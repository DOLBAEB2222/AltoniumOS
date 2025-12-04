#!/bin/bash

echo "Testing AltoniumOS disk I/O..."
echo "Starting QEMU for 10 seconds..."
timeout 10 qemu-system-i386 -drive format=raw,file=dist/os.img -nographic 2>&1 | tee test_output.log

echo ""
echo "Test output saved to test_output.log"
echo "Last few lines of output:"
tail -20 test_output.log