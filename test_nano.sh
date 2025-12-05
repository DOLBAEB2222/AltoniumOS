#!/bin/bash

# Test script for nano editor
# This script runs QEMU with automated input to test the nano editor

cd /home/engine/project

# Start QEMU in background
qemu-system-i386 -cdrom dist/os.iso -nographic -serial mon:stdio &
QEMU_PID=$!

# Wait for GRUB to load
sleep 3

# Send Enter to boot AltoniumOS
echo -ne '\n' > /proc/$QEMU_PID/fd/0

# Wait for OS to boot
sleep 5

# Send "nano test.txt" command
echo -ne 'nano test.txt\n' > /proc/$QEMU_PID/fd/0

# Wait for nano to start
sleep 2

# Type some text
echo -ne 'Hello World!\nThis is a test.\nThird line.' > /proc/$QEMU_PID/fd/0

# Wait a bit
sleep 2

# Save with Ctrl+S
echo -ne '\x13' > /proc/$QEMU_PID/fd/0

# Wait for save
sleep 2

# Exit with Ctrl+X
echo -ne '\x18' > /proc/$QEMU_PID/fd/0

# Wait for exit
sleep 2

# Verify file was created with cat
echo -ne 'cat test.txt\n' > /proc/$QEMU_PID/fd/0

# Wait for output
sleep 3

# Shutdown
echo -ne 'shutdown\n' > /proc/$QEMU_PID/fd/0

# Wait a bit more
sleep 2

# Kill QEMU if still running
kill $QEMU_PID 2>/dev/null

echo "Test completed"