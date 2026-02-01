#!/usr/bin/env python3
"""
Storage Format Options Example
==============================

Demonstrates the three storage format options:
1. JSON - Human-readable (demos, debugging)
2. Binary - Maximum performance (production, autonomous)
3. Simple - Fast text (middle ground)
"""

import sys
import os
import time
import struct

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
    from synrix.storage_formats import json_format, binary_format, simple_format
except ImportError:
    sys.path.insert(0, os.path.join(parent_dir, 'synrix'))
    from raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
    from storage_formats import json_format, binary_format, simple_format


def demo_json_format():
    """JSON format - human-readable"""
    print("=" * 64)
    print("JSON Format - Human-Readable")
    print("=" * 64)
    print()
    print("Use case: Demos, debugging, human-readable data")
    print("Overhead: ~0.008μs per operation")
    print()
    
    backend = RawSynrixBackend("~/.demo_json.lattice")
    formatter = json_format()
    
    # Store
    data = {
        'title': 'Vector Database',
        'content': 'A vector database is optimized for...',
        'source': 'wikipedia'
    }
    
    start = time.time()
    data_bytes = formatter.encode(data)
    encoded_time = (time.time() - start) * 1000000
    
    data_str = data_bytes.decode('utf-8')
    backend.add_node("doc:vector_db", data_str, LATTICE_NODE_LEARNING)
    
    # Retrieve
    node = backend.get_node(1)
    start = time.time()
    decoded = formatter.decode(node['data'].encode('utf-8'))
    decoded_time = (time.time() - start) * 1000000
    
    print(f"Encoded: {encoded_time:.4f}μs")
    print(f"Decoded: {decoded_time:.4f}μs")
    print(f"Data: {decoded}")
    print()
    
    backend.close()


def demo_binary_format():
    """Binary format - maximum performance"""
    print("=" * 64)
    print("Binary Format - Maximum Performance")
    print("=" * 64)
    print()
    print("Use case: Production, autonomous systems, maximum performance")
    print("Overhead: Zero (no parsing)")
    print()
    
    backend = RawSynrixBackend("~/.demo_binary.lattice")
    formatter = binary_format()
    
    # Store binary data (e.g., performance metrics)
    # Format: timestamp (8 bytes) + value (8 bytes) + count (4 bytes)
    timestamp = int(time.time() * 1000)
    value = 123.456
    count = 42
    
    start = time.time()
    binary_data = struct.pack('QdI', timestamp, value, count)  # Q=uint64, d=double, I=uint32
    data_bytes = formatter.encode(binary_data)
    encoded_time = (time.time() - start) * 1000000
    
    # Binary format stores with length header
    data_str = data_bytes.decode('latin-1', errors='ignore')  # Binary-safe encoding
    backend.add_node("metric:performance", data_str, LATTICE_NODE_LEARNING)
    
    # Retrieve
    node = backend.get_node(1)
    start = time.time()
    decoded_bytes = formatter.decode(node['data'].encode('latin-1', errors='ignore'))
    decoded_time = (time.time() - start) * 1000000
    
    if decoded_bytes:
        unpacked = struct.unpack('QdI', decoded_bytes)
        print(f"Encoded: {encoded_time:.4f}μs")
        print(f"Decoded: {decoded_time:.4f}μs")
        print(f"Data: timestamp={unpacked[0]}, value={unpacked[1]}, count={unpacked[2]}")
    print()
    
    backend.close()


def demo_simple_format():
    """Simple format - fast text"""
    print("=" * 64)
    print("Simple Format - Fast Text")
    print("=" * 64)
    print()
    print("Use case: Simple structured data, performance-sensitive text")
    print("Overhead: ~0.0009μs per operation (10× faster than JSON)")
    print()
    
    backend = RawSynrixBackend("~/.demo_simple.lattice")
    formatter = simple_format()
    
    # Store simple fields
    fields = ['Vector Database', 'A vector database is optimized...', 'wikipedia']
    
    start = time.time()
    data_bytes = formatter.encode(fields)
    encoded_time = (time.time() - start) * 1000000
    
    data_str = data_bytes.decode('utf-8')
    backend.add_node("doc:vector_db", data_str, LATTICE_NODE_LEARNING)
    
    # Retrieve
    node = backend.get_node(1)
    start = time.time()
    decoded = formatter.decode(node['data'].encode('utf-8'))
    decoded_time = (time.time() - start) * 1000000
    
    print(f"Encoded: {encoded_time:.4f}μs")
    print(f"Decoded: {decoded_time:.4f}μs")
    print(f"Data: {decoded}")
    print()
    
    backend.close()


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║        SYNRIX Storage Format Options                           ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    print("Three formats for different use cases:")
    print("  1. JSON - Human-readable (demos, debugging)")
    print("  2. Binary - Maximum performance (production, autonomous)")
    print("  3. Simple - Fast text (middle ground)")
    print()
    
    demo_json_format()
    demo_binary_format()
    demo_simple_format()
    
    print("=" * 64)
    print("RECOMMENDATION:")
    print("  • Demos/Development: Use JSON (human-readable)")
    print("  • Production/Autonomous: Use Binary (zero overhead)")
    print("  • Simple structured data: Use Simple (fast text)")
    print("=" * 64)


if __name__ == "__main__":
    main()
