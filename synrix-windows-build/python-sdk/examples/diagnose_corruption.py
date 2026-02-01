#!/usr/bin/env python3
"""
Corruption Diagnostic Tool
==========================
Inspects the lattice file to understand why nodes are being flagged as corrupted.
"""

import sys
import os
import struct
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from synrix.raw_backend import RawSynrixBackend

def inspect_lattice_file(lattice_path):
    """Inspect the raw lattice file to see what's actually stored"""
    if not os.path.exists(lattice_path):
        print(f"❌ File not found: {lattice_path}")
        return
    
    print("=" * 70)
    print("LATTICE FILE INSPECTION")
    print("=" * 70)
    print()
    
    file_size = os.path.getsize(lattice_path)
    print(f"File: {lattice_path}")
    print(f"Size: {file_size:,} bytes")
    print()
    
    with open(lattice_path, 'rb') as f:
        # Read header
        header = f.read(16)
        if len(header) < 16:
            print("❌ File too small for header")
            return
        
        # Header is 4 uint32_t values (16 bytes) - native endianness
        magic, total_nodes, next_id_lo, nodes_to_load = struct.unpack('=IIII', header)
        next_id = next_id_lo  # next_id is stored as uint32_t in header
        print(f"Header:")
        print(f"  Magic: 0x{magic:08X} ({'LATT' if magic == 0x4C415454 else 'INVALID'})")
        print(f"  total_nodes: {total_nodes:,}")
        print(f"  next_id: {next_id:,}")
        print(f"  nodes_to_load: {nodes_to_load:,}")
        print()
        
        # Node structure size (from C: sizeof(lattice_node_t))
        NODE_SIZE = 1024  # Approximate - actual is larger due to payload union
        
        # Read first few nodes
        print("First 10 nodes:")
        for i in range(min(10, nodes_to_load)):
            node_data = f.read(1024)  # Read full node structure
            if len(node_data) < 64:  # At least read ID and name
                break
            
            # Extract node ID (first 8 bytes, native endianness)
            node_id = struct.unpack('=Q', node_data[0:8])[0]
            
            # Extract node type (next 4 bytes, native endianness)
            node_type = struct.unpack('=I', node_data[8:12])[0]
            
            # Extract name (next 64 bytes, null-terminated)
            name_bytes = node_data[12:76]
            name = name_bytes.split(b'\x00')[0].decode('utf-8', errors='ignore')
            
            # Extract local_id
            local_id = node_id & 0xFFFFFFFF
            
            print(f"  Node {i}:")
            print(f"    ID: {node_id} (local_id={local_id})")
            print(f"    Type: {node_type}")
            print(f"    Name: '{name}'")
            print()
        
        # Check around position 9900 (where corruption was reported)
        print("Nodes around position 9900 (where corruption was reported):")
        f.seek(16 + (9900 * 1024))  # Skip header + 9900 nodes
        
        for i in range(10):
            node_data = f.read(1024)
            if len(node_data) < 64:
                break
            
            node_id = struct.unpack('<Q', node_data[0:8])[0]
            node_type = struct.unpack('<I', node_data[8:12])[0]
            name_bytes = node_data[12:76]
            name = name_bytes.split(b'\x00')[0].decode('utf-8', errors='ignore')
            local_id = node_id & 0xFFFFFFFF
            
            # Check if this would be flagged as corrupted
            max_safe_id = 12000 * 10  # max_nodes * 10
            is_chunked = name.startswith("CHUNKED:") or name.startswith("CHUNK:")
            would_be_corrupted = (node_id != 0 and local_id > max_safe_id and not is_chunked)
            
            status = "⚠️ CORRUPTED" if would_be_corrupted else "✅ OK"
            if node_id == 0:
                status = "⚠️ UNINITIALIZED"
            
            print(f"  Position {9900 + i}: {status}")
            print(f"    ID: {node_id} (local_id={local_id}, max_safe={max_safe_id})")
            print(f"    Type: {node_type}")
            print(f"    Name: '{name}'")
            print(f"    Is chunked: {is_chunked}")
            print()

def test_corruption_scenario():
    """Create a test scenario that reproduces the corruption"""
    print("=" * 70)
    print("CORRUPTION REPRODUCTION TEST")
    print("=" * 70)
    print()
    
    lattice_path = os.path.expanduser("~/.test_corruption_diagnosis.lattice")
    
    # Clean up
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    
    backend = RawSynrixBackend(lattice_path, max_nodes=10000)
    
    # Add some small nodes
    print("Adding 100 small nodes...")
    for i in range(100):
        backend.add_node(f"TEST:node_{i:05d}", f"Data for node {i}")
    
    # Add some chunked nodes
    print("Adding 10 chunked nodes...")
    chunked_ids = []
    for i in range(10):
        large_data = b'\xAA' * 2000
        node_id = backend.add_node_chunked(f"LARGE:node_{i:03d}", large_data)
        chunked_ids.append(node_id)
        print(f"  Chunked node {i}: ID={node_id}")
    
    # Save
    print("\nSaving...")
    backend.save()
    backend.close()
    
    # Inspect the file
    print("\nInspecting saved file...")
    inspect_lattice_file(lattice_path)
    
    # Reload and check
    print("\nReloading and checking...")
    backend2 = RawSynrixBackend(lattice_path, max_nodes=10000)
    
    # Check chunked nodes
    print("\nChecking chunked nodes:")
    for i, node_id in enumerate(chunked_ids):
        node = backend2.get_node(node_id)
        if node:
            print(f"  Node {i}: ID={node_id}, Name='{node['name']}' ✅")
        else:
            print(f"  Node {i}: ID={node_id} ❌ NOT FOUND")
    
    backend2.close()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # Inspect existing file
        inspect_lattice_file(sys.argv[1])
    else:
        # Run reproduction test
        test_corruption_scenario()
