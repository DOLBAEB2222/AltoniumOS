#!/bin/bash
# Test script to demonstrate filesystem commands in AltoniumOS
# This script provides QEMU command examples to test all filesystem features

echo "=== AltoniumOS Filesystem Commands Test Guide ==="
echo
echo "This guide shows how to test all filesystem commands in QEMU."
echo "Run AltoniumOS in QEMU with:"
echo "  qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide"
echo
echo "=== Basic Navigation Commands ==="
echo
echo "1. List root directory contents (both commands show same output):"
echo "   > ls"
echo "   > dir"
echo "   Expected: Shows [DIR] DOCS, README.TXT, and SYSTEM.CFG"
echo
echo "2. Show current working directory:"
echo "   > pwd"
echo "   Expected: /"
echo
echo "3. Change to DOCS directory:"
echo "   > cd docs"
echo "   Expected: /DOCS"
echo
echo "4. List DOCS directory:"
echo "   > ls"
echo "   > dir"
echo "   Expected: Shows INFO.TXT"
echo
echo "5. Go back to parent directory:"
echo "   > cd .."
echo "   > pwd"
echo "   Expected: /"
echo
echo "=== File Operations Commands ==="
echo
echo "6. Display file contents:"
echo "   > cat README.TXT"
echo "   Expected: Shows file content"
echo
echo "7. Create zero-length file:"
echo "   > touch EMPTY.TXT"
echo "   > ls"
echo "   Expected: Shows EMPTY.TXT (0 bytes)"
echo
echo "8. Create file with content:"
echo "   > write TEST.TXT Hello from AltoniumOS"
echo "   Expected: Wrote 22 bytes"
echo
echo "9. Display new file:"
echo "   > cat TEST.TXT"
echo "   Expected: Hello from AltoniumOS"
echo
echo "=== Directory Operations Commands ==="
echo
echo "10. Create new directory:"
echo "    > mkdir MYDIR"
echo "    > ls"
echo "    Expected: Shows [DIR] MYDIR"
echo
echo "11. Navigate to new directory:"
echo "    > cd mydir"
echo "    > pwd"
echo "    Expected: /MYDIR"
echo
echo "12. Create file in subdirectory:"
echo "    > touch NOTE.TXT"
echo "    > write NOTE.TXT Testing subdirectory"
echo "    > dir"
echo "    Expected: Shows NOTE.TXT"
echo
echo "=== Error Handling Tests ==="
echo
echo "13. Test file not found:"
echo "    > cat NOTEXIST.TXT"
echo "    Expected: cat failed (not found code -7)"
echo
echo "14. Test invalid directory:"
echo "    > cd NOTEXIST"
echo "    Expected: cd failed (not found code -7)"
echo
echo "15. Test directory vs file operations:"
echo "    > cat DOCS"
echo "    Expected: cat failed (not file code -12)"
echo
echo "=== Path Navigation Tests ==="
echo
echo "16. Absolute path navigation:"
echo "    > cd /"
echo "    > cd /docs"
echo "    > pwd"
echo "    Expected: /DOCS"
echo
echo "17. Relative path with parent:"
echo "    > cd /docs"
echo "    > cd .."
echo "    > pwd"
echo "    Expected: /"
echo
echo "=== File Deletion Commands ==="
echo
echo "18. Delete a file:"
echo "    > rm EMPTY.TXT"
echo "    > ls"
echo "    Expected: EMPTY.TXT no longer shown"
echo
echo "=== Testing Tips ==="
echo
echo "- All filenames follow DOS 8.3 convention (e.g., FILENAME.EXT)"
echo "- Commands are case-insensitive but filenames are stored in uppercase"
echo "- Both 'ls' and 'dir' provide identical output with type and size info"
echo "- The [DIR] prefix identifies directories"
echo "- File sizes are shown in bytes for regular files"
echo "- 'touch' creates zero-byte files useful for placeholders"
echo "- 'write' can create or overwrite files with command-line text"
echo "- Use '.. ' for parent directory navigation"
echo "- Use '/' for root directory"
echo "- Error messages include meaningful error codes and descriptions"
echo
echo "=== Quick Test Sequence ==="
echo
echo "To quickly test all commands, run these in order:"
echo "  help"
echo "  pwd"
echo "  ls"
echo "  dir"
echo "  cat README.TXT"
echo "  touch EMPTY.TXT"
echo "  write TEST.TXT Sample content"
echo "  cat TEST.TXT"
echo "  mkdir TESTDIR"
echo "  cd testdir"
echo "  pwd"
echo "  touch FILE.TXT"
echo "  ls"
echo "  cd .."
echo "  dir"
echo "  rm EMPTY.TXT"
echo "  ls"
echo
echo "=== Running the Tests ==="
echo
if [ -f dist/kernel.elf ] && [ -f dist/os.img ]; then
    echo "✓ Build artifacts present - ready to test!"
    echo
    echo "Start QEMU with:"
    echo "  qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide"
    echo
    echo "Or with VGA display:"
    echo "  qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide -display gtk"
else
    echo "✗ Build artifacts missing!"
    echo "Run: make build && make img"
    exit 1
fi

echo "=== Test Complete ==="
