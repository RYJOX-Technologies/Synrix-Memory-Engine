#!/usr/bin/env python3
import struct
import os

test_file = 'test_persistence_debug.lattice'
if os.path.exists(test_file):
    with open(test_file, 'rb') as f:
        header = struct.unpack('IIII', f.read(16))
        print(f'File header:')
        print(f'  magic: 0x{header[0]:08X} (expected: 0x4C415454)')
        print(f'  total_nodes: {header[1]}')
        print(f'  next_id: {header[2]}')
        print(f'  nodes_to_load: {header[3]}')
        print(f'  File size: {os.path.getsize(test_file)} bytes')
        print(f'  Expected size: {16 + (header[3] * 1216)} bytes')
else:
    print(f'File {test_file} does not exist')
