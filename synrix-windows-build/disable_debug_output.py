#!/usr/bin/env python3
"""
Disable debug output in C source files for production builds.
Comments out all fprintf statements that contain "DEBUG" in the message.
"""

import re
import sys
from pathlib import Path

def disable_debug_output(file_path):
    """Comment out debug fprintf statements"""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    lines = content.split('\n')
    modified = False
    new_lines = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this is a debug fprintf statement
        if 'fprintf' in line and 'DEBUG' in line:
            # Comment out the fprintf line
            if not line.strip().startswith('//'):
                new_lines.append('    // ' + line.lstrip())
                modified = True
            else:
                new_lines.append(line)
        elif 'fprintf' in line and ('[LATTICE-SAVE] DEBUG' in line or '[LATTICE-LOAD]' in line or '[WAL-DEBUG]' in line):
            # Also comment out LATTICE-SAVE/LOAD debug statements
            if not line.strip().startswith('//'):
                new_lines.append('    // ' + line.lstrip())
                modified = True
            else:
                new_lines.append(line)
        elif 'fflush(stderr)' in line and i > 0 and 'DEBUG' in lines[i-1]:
            # Comment out fflush after debug statements
            if not line.strip().startswith('//'):
                new_lines.append('    // ' + line.lstrip())
                modified = True
            else:
                new_lines.append(line)
        else:
            new_lines.append(line)
        
        i += 1
    
    if modified:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(new_lines))
        return True
    return False

if __name__ == '__main__':
    # Files to process
    files = [
        'build/windows/src/persistent_lattice.c',
        'build/windows/src/wal.c',
    ]
    
    for file_path in files:
        path = Path(file_path)
        if path.exists():
            print(f"Processing {file_path}...")
            if disable_debug_output(path):
                print(f"  [OK] Disabled debug output in {file_path}")
            else:
                print(f"  [INFO] No changes needed in {file_path}")
        else:
            print(f"  [WARN] File not found: {file_path}")
