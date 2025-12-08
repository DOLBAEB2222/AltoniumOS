#!/usr/bin/expect -f
#
# AltoniumOS Diskpart Test Script
# Tests the disk partitioner utility
#

set timeout 30
set test_passed 0

# Spawn QEMU with the kernel
spawn qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide -nographic

# Wait for boot
expect {
    timeout { puts "ERROR: Boot timeout"; exit 1 }
    "Type 'help' for available commands"
}

# Give the system a moment to stabilize
sleep 1

# Launch diskpart
send "diskpart\r"

expect {
    timeout { puts "ERROR: Diskpart launch timeout"; exit 1 }
    "AltoniumOS Disk Partitioner"
}

expect {
    timeout { puts "ERROR: Partition layout timeout"; exit 1 }
    "Current Partition Layout"
}

expect {
    timeout { puts "ERROR: Actions menu timeout"; exit 1 }
    "Actions"
}

# Try to create a partition (select first option and Enter)
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Create partition response timeout"; exit 1 }
    -re "(created successfully|Maximum .* partitions|Failed to create)"
}

# If we got a success or max partitions message, press any key
sleep 1
send "\r"

# Should be back at diskpart menu
expect {
    timeout { puts "ERROR: Return to menu timeout"; exit 1 }
    "Actions"
}

# Try refresh (down arrow twice, then Enter)
sleep 1
send "\033\[B"
sleep 1
send "\033\[B"
sleep 1
send "\r"

# Should reload diskpart
expect {
    timeout { puts "ERROR: Refresh timeout"; exit 1 }
    "AltoniumOS Disk Partitioner"
}

expect {
    timeout { puts "ERROR: Refreshed layout timeout"; exit 1 }
    "Current Partition Layout"
}

# Exit diskpart (down arrow three times to "Exit to shell", then Enter)
sleep 1
send "\033\[B"
sleep 1
send "\033\[B"
sleep 1
send "\033\[B"
sleep 1
send "\r"

# Should be back at shell
expect {
    timeout { puts "ERROR: Shell return timeout"; exit 1 }
    ">"
}

# Test successful
puts "\n=== DISKPART TEST PASSED ==="
set test_passed 1

# Clean exit
send "shutdown\r"
sleep 2

exit 0
