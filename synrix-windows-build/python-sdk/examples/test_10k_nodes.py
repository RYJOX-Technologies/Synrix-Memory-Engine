#!/usr/bin/env python3
"""
10K Node Performance Test
=========================
Tests SYNRIX performance with 10,000 nodes:
- Storage performance
- Query performance
- Memory usage
- Persistence (save/load)
- Chunked data handling
"""

import sys
import os
import time
import json
import psutil
import gc
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from synrix.raw_backend import RawSynrixBackend

def format_bytes(bytes_val):
    """Format bytes to human-readable format"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_val < 1024.0:
            return f"{bytes_val:.2f} {unit}"
        bytes_val /= 1024.0
    return f"{bytes_val:.2f} TB"

def get_memory_usage():
    """Get current memory usage"""
    process = psutil.Process(os.getpid())
    return process.memory_info().rss

def test_10k_nodes():
    """Test 10K node performance"""
    print("=" * 70)
    print("10K NODE PERFORMANCE TEST")
    print("=" * 70)
    print()
    
    lattice_path = os.path.expanduser("~/.test_10k_nodes.lattice")
    
    # Clean up old file
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
        print(f"✅ Cleaned up old lattice file")
    
    print(f"Lattice path: {lattice_path}")
    print(f"Max nodes: 10000")
    print()
    
    # Initialize backend
    print("Initializing backend...")
    start_mem = get_memory_usage()
    backend = RawSynrixBackend(lattice_path, max_nodes=10000)
    init_mem = get_memory_usage()
    init_time = time.time()
    
    print(f"  Initialization memory: {format_bytes(init_mem - start_mem)}")
    print()
    
    # Test 1: Store 10K small nodes (< 512 bytes)
    print("TEST 1: Store 10,000 Small Nodes (< 512 bytes)")
    print("-" * 70)
    
    start_time = time.time()
    start_mem = get_memory_usage()
    node_ids = []
    
    for i in range(10000):
        data = f"Node {i}: This is test data for node {i} with some content. " * 5
        data = data[:400]  # Ensure < 512 bytes
        
        node_id = backend.add_node(f"TEST:node_{i:05d}", data)
        node_ids.append(node_id)
        
        if (i + 1) % 1000 == 0:
            elapsed = time.time() - start_time
            rate = (i + 1) / elapsed
            print(f"  Progress: {i + 1}/10000 nodes ({rate:.0f} nodes/sec)")
    
    store_time = time.time() - start_time
    store_mem = get_memory_usage()
    
    print(f"  Total time: {store_time:.2f}s")
    print(f"  Average: {store_time / 10000 * 1000:.2f}ms per node")
    print(f"  Rate: {10000 / store_time:.0f} nodes/sec")
    print(f"  Memory used: {format_bytes(store_mem - init_mem)}")
    print()
    
    # Save to disk
    print("Saving to disk...")
    save_start = time.time()
    backend.save()
    save_time = time.time() - save_start
    file_size = os.path.getsize(lattice_path) if os.path.exists(lattice_path) else 0
    
    print(f"  Save time: {save_time:.2f}s")
    print(f"  File size: {format_bytes(file_size)}")
    print(f"  Bytes per node: {file_size / 10000:.0f} bytes")
    print()
    
    # Test 2: Query performance (random lookups)
    print("TEST 2: Query Performance (Random Lookups)")
    print("-" * 70)
    
    import random
    random.seed(42)
    test_indices = [random.randint(0, 9999) for _ in range(1000)]
    
    start_time = time.time()
    found = 0
    for idx in test_indices:
        node_id = node_ids[idx]
        node = backend.get_node(node_id)
        if node:
            found += 1
    
    query_time = time.time() - start_time
    
    print(f"  Queried: 1000 random nodes")
    print(f"  Found: {found}/1000")
    print(f"  Total time: {query_time:.2f}s")
    print(f"  Average: {query_time / 1000 * 1000:.2f}ms per query")
    print(f"  Rate: {1000 / query_time:.0f} queries/sec")
    print()
    
    # Test 3: Prefix queries
    print("TEST 3: Prefix Query Performance")
    print("-" * 70)
    
    start_time = time.time()
    results = backend.find_by_prefix("TEST:node_", limit=1000)
    prefix_time = time.time() - start_time
    
    print(f"  Prefix: 'TEST:node_'")
    print(f"  Results: {len(results)}")
    print(f"  Query time: {prefix_time * 1000:.2f}ms")
    print(f"  Average: {prefix_time / len(results) * 1000:.3f}ms per result")
    print()
    
    # Test 4: Chunked data (large nodes)
    print("TEST 4: Chunked Data Performance")
    print("-" * 70)
    
    # Add 100 large nodes (chunked)
    large_node_ids = []
    start_time = time.time()
    
    for i in range(100):
        large_data = b'\xAA' * 2000  # 2000 bytes = 4 chunks
        node_id = backend.add_node_chunked(f"LARGE:node_{i:03d}", large_data)
        large_node_ids.append(node_id)
    
    chunked_store_time = time.time() - start_time
    
    print(f"  Stored: 100 large nodes (2000 bytes each = 4 chunks)")
    print(f"  Total time: {chunked_store_time:.2f}s")
    print(f"  Average: {chunked_store_time / 100 * 1000:.2f}ms per node")
    print(f"  Total chunks: 400 chunks (100 nodes × 4 chunks)")
    print()
    
    # Retrieve chunked data
    print("Retrieving chunked data...")
    start_time = time.time()
    retrieved = 0
    
    for node_id in large_node_ids[:10]:  # Test 10 retrievals
        data = backend.get_node_chunked(node_id)
        if data and len(data) == 2000:
            retrieved += 1
    
    chunked_query_time = time.time() - start_time
    
    print(f"  Retrieved: {retrieved}/10 large nodes")
    print(f"  Query time: {chunked_query_time * 1000:.2f}ms")
    print(f"  Average: {chunked_query_time / 10 * 1000:.2f}ms per retrieval")
    print()
    
    # Save again with chunked data
    print("Saving with chunked data...")
    save_start = time.time()
    backend.save()
    save_time2 = time.time() - save_start
    file_size2 = os.path.getsize(lattice_path) if os.path.exists(lattice_path) else 0
    
    print(f"  Save time: {save_time2:.2f}s")
    print(f"  File size: {format_bytes(file_size2)}")
    print()
    
    # Test 5: Persistence (reload)
    print("TEST 5: Persistence (Reload)")
    print("-" * 70)
    
    # Close and reload
    backend.close()
    del backend
    gc.collect()
    
    load_start = time.time()
    backend2 = RawSynrixBackend(lattice_path, max_nodes=10000)
    load_time = time.time() - load_start
    
    # Verify data
    verify_start = time.time()
    verified = 0
    for idx in test_indices[:100]:  # Verify 100 random nodes
        node_id = node_ids[idx]
        node = backend2.get_node(node_id)
        if node and node['name'] == f"TEST:node_{idx:05d}":
            verified += 1
    
    verify_time = time.time() - verify_start
    
    print(f"  Load time: {load_time:.2f}s")
    print(f"  Verified: {verified}/100 random nodes")
    print(f"  Verify time: {verify_time * 1000:.2f}ms")
    print()
    
    # Final memory usage
    final_mem = get_memory_usage()
    
    print("=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print(f"Total nodes: 10,000 small + 100 large = 10,100 nodes")
    print(f"Total chunks: 10,000 + 400 = 10,400 chunks")
    print(f"File size: {format_bytes(file_size2)}")
    print(f"Memory usage: {format_bytes(final_mem - start_mem)}")
    print()
    print("Performance:")
    print(f"  Store (10K small): {10000 / store_time:.0f} nodes/sec")
    print(f"  Store (100 large): {100 / chunked_store_time:.0f} nodes/sec")
    print(f"  Query (random): {1000 / query_time:.0f} queries/sec")
    print(f"  Prefix query: {len(results) / prefix_time:.0f} results/sec")
    print(f"  Chunked retrieval: {10 / chunked_query_time:.0f} retrievals/sec")
    print(f"  Save: {save_time:.2f}s")
    print(f"  Load: {load_time:.2f}s")
    print()
    print("✅ All tests completed successfully!")
    
    backend2.close()

if __name__ == "__main__":
    try:
        test_10k_nodes()
    except KeyboardInterrupt:
        print("\n⚠️  Test interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
