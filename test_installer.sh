#!/usr/bin/expect -f
#
# AltoniumOS Installer Test Script
# Tests the full installer wizard in a non-destructive dry-run mode
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

# Launch the installer
send "install\r"

expect {
    timeout { puts "ERROR: Installer launch timeout"; exit 1 }
    "AltoniumOS Installer"
}

expect {
    timeout { puts "ERROR: Disk selection timeout"; exit 1 }
    "Select Target Disk"
}

# Select first disk (press Enter)
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Partition table selection timeout"; exit 1 }
    "Select Partition Table Type"
}

# Select MBR (press Enter)
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Filesystem selection timeout"; exit 1 }
    "Select Filesystem Type"
}

# Select FAT32 (down arrow once, then Enter)
sleep 1
send "\033\[B"
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Confirmation prompt timeout"; exit 1 }
    "Confirm Formatting"
}

# Confirm formatting (press Y)
sleep 1
send "y"

expect {
    timeout { puts "ERROR: Formatting timeout"; exit 1 }
    -re "(formatted successfully|format may be incomplete)"
}

# Continue to file copy step
expect {
    timeout { puts "ERROR: File copy prompt timeout"; exit 1 }
    "Press any key to continue"
}
send "\r"

expect {
    timeout { puts "ERROR: File copy screen timeout"; exit 1 }
    "Copy System Files"
}

# Continue to bootloader step
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Bootloader screen timeout"; exit 1 }
    "Install Bootloader"
}

# Continue to completion
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Completion screen timeout"; exit 1 }
    "Installation Complete"
}

# Return to shell
sleep 1
send "\r"

expect {
    timeout { puts "ERROR: Shell return timeout"; exit 1 }
    ">"
}

# Test successful
puts "\n=== INSTALLER TEST PASSED ==="
set test_passed 1

# Clean exit
send "shutdown\r"
sleep 2

exit 0
