#!/usr/bin/expect -f

# Test script for nano editor using expect
set timeout 10

spawn qemu-system-i386 -cdrom /home/engine/project/dist/os.iso -nographic

# Wait for GRUB
expect "AltoniumOS"

# Send Enter to boot
send "\r"

# Wait for shell prompt
expect "Type 'help' for available commands"

# Test help command first
send "help\r"
expect "nano FILE"

# Test nano command
send "nano test.txt\r"

# Wait for editor to load
sleep 2

# Type some text
send "Hello World!\r"
send "This is a test of the nano editor.\r"
send "Third line of text."

# Save with Ctrl+S
send "\x13"

# Exit with Ctrl+X  
send "\x18"

# Verify file was created
send "cat test.txt\r"
expect "Hello World!"

# Shutdown
send "shutdown\r"

expect eof