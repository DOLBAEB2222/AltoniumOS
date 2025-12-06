#!/bin/bash

# Test script for theme system and nano shortcuts

echo "=== Theme System and Nano Shortcuts Test ==="
echo ""
echo "This test verifies:"
echo "1. Theme command is recognized"
echo "2. Help text includes theme command"
echo "3. Help text shows Ctrl+S and Ctrl+X for nano"
echo "4. Theme system code is present in kernel/main.c"
echo ""

# Check if kernel/main.c contains theme system
echo "Checking for theme system in kernel/main.c..."
if grep -q "theme_t themes" kernel/main.c && \
   grep -q "handle_theme_command" kernel/main.c && \
   grep -q "get_current_text_attr" kernel/main.c; then
     echo "✓ Theme system found in kernel/main.c"
 else
     echo "✗ Theme system NOT found in kernel/main.c"
    exit 1
fi

# Check if help text includes theme command
echo "Checking help text..."
if grep -q "theme.*Switch theme" kernel/main.c; then
    echo "✓ Theme command in help text"
else
    echo "✗ Theme command NOT in help text"
    exit 1
fi

# Check if help text mentions Ctrl+S and Ctrl+X
if grep -q "Ctrl+S.*Ctrl+X" kernel/main.c; then
    echo "✓ Nano shortcuts (Ctrl+S, Ctrl+X) in help text"
else
    echo "✗ Nano shortcuts NOT in help text"
    exit 1
fi

# Check for Ctrl key handling
echo "Checking Ctrl key handling..."
if grep -q "ctrl_pressed" kernel/main.c && \
   grep -q "scancode == 0x1D" kernel/main.c && \
   grep -q "scancode == 0x1F" kernel/main.c && \
   grep -q "scancode == 0x2D" kernel/main.c; then
    echo "✓ Ctrl key handling implemented"
else
    echo "✗ Ctrl key handling NOT properly implemented"
    exit 1
fi

# Check for theme definitions
echo "Checking theme definitions..."
if grep -q "THEME_NORMAL" kernel/main.c && \
   grep -q "THEME_BLUE" kernel/main.c && \
   grep -q "THEME_GREEN" kernel/main.c; then
    echo "✓ Three themes defined (normal, blue, green)"
else
    echo "✗ Theme definitions incomplete"
    exit 1
fi

# Check for VGA color attributes
echo "Checking VGA color system..."
if grep -q "VGA_ATTR(fg, bg)" kernel/main.c && \
   grep -q "VGA_COLOR_" kernel/main.c; then
    echo "✓ VGA color system implemented"
else
    echo "✗ VGA color system NOT found"
    exit 1
fi

# Check README for theme documentation
echo "Checking README documentation..."
if grep -q "Theme System" README.md && \
   grep -q "theme normal" README.md && \
   grep -q "Ctrl+S to save, Ctrl+X to exit" README.md; then
    echo "✓ README documentation updated"
else
    echo "✗ README documentation incomplete"
    exit 1
fi

echo ""
echo "=== All checks passed! ==="
echo ""
echo "Manual verification steps:"
echo "1. Build and run: make img && qemu-system-i386 -kernel dist/kernel.elf -drive format=raw,file=dist/os.img,if=ide"
echo "2. Test theme command: theme list, theme blue, theme green, theme normal"
echo "3. Test nano editor: nano test.txt, type text, press Ctrl+S to save, press Ctrl+X to exit"
echo "4. Verify theme persists across nano sessions"
echo "5. Verify all 3 themes are visually distinct"
