#!/usr/bin/env python3
with open('Makefile', 'r') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if line.startswith('    ') and not line.startswith('    $(BUILD'):
        if i > 0 and ':' in lines[i-1] and not lines[i-1].strip().startswith('#'):
            lines[i] = '\t' + line.lstrip()

with open('Makefile', 'w') as f:
    f.writelines(lines)

print("Fixed")
