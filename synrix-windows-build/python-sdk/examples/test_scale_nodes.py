#!/usr/bin/env python3
"""
Scalable Node Performance Test
==============================
Tests SYNRIX performance with configurable node counts (10K to 1M+).
Usage: python3 test_scale_nodes.py [num_nodes] [--chunked-ratio RATIO]
"""

import sys
import os
import time
import json
import argparse
import gc
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    import psutil
    HAS_PSUTIL = True
except ImportError:
    HAS_PSUTIL = False
    print("⚠️  psutil not available - memory tracking disabled")

from synrix.raw_backend import RawSynrixBackend

def format_bytes(bytes_val):
    """Format bytes to human-readable format"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_val < 1024.0:
            return f"{bytes_val:.2f} {unit}"
        bytes_val /= 1024.0
    return f"{bytes_val:.2f} TB"

def format_number(num):
    """Format number with commas"""
    return f"{num:,}"

def get_memory_usage():
    """Get current memory usage"""
    if not HAS_PSUTIL:
        return 0
    process = psutil.Process(os.getpid())
    return process.memory_info().rss

def test_scale_nodes(num_nodes, chunked_ratio=0.01, progress_interval=1000):
    """
    Test SYNRIX performance with configurable node count.
    
    Args:
        num_nodes: Total number of nodes to create
        chunked_ratio: Ratio of nodes that should be chunked (0.0-1.0)
        progress_interval: Print progress every N nodes
    """
    print("=" * 70)
    print(f"SCALABLE NODE PERFORMANCE TEST - {format_number(num_nodes)} NODES")
    print("=" * 70)
    print()
    
    lattice_path = os.path.expanduser("~/.test_scale_nodes.lattice")
    
    # Clean up old file
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
        print(f"✅ Cleaned up old lattice file")
    
    print(f"Configuration:")
    print(f"  Lattice path: {lattice_path}")
    print(f"  Total nodes: {format_number(num_nodes)}")
    print(f"  Chunked ratio: {chunked_ratio * 100:.1f}%")
    print(f"  Small nodes: {format_number(int(num_nodes * (1 - chunked_ratio)))}")
    print(f"  Large nodes: {format_number(int(num_nodes * chunked_ratio))}")
    print()
    
    # Calculate max_nodes (add 20% buffer for chunked nodes)
    max_nodes = int(num_nodes * 1.2)
    
    # Initialize backend
    print("Initializing backend...")
    start_mem = get_memory_usage()
    backend = RawSynrixBackend(lattice_path, max_nodes=max_nodes)
    init_mem = get_memory_usage()
    init_time = time.time()
    
    if HAS_PSUTIL:
        print(f"  Initialization memory: {format_bytes(init_mem - start_mem)}")
    print()
    
    # Test 1: Store nodes
    print("TEST 1: Store Nodes")
    print("-" * 70)
    
    num_small = int(num_nodes * (1 - chunked_ratio))
    num_large = num_nodes - num_small
    
    start_time = time.time()
    start_mem = get_memory_usage()
    node_ids = []
    chunked_node_ids = []
    
    # Store small nodes
    print(f"Storing {format_number(num_small)} small nodes...")
    for i in range(num_small):
        data = f"Node {i}: This is test data for node {i} with some content. " * 5
        data = data[:400]  # Ensure < 512 bytes
        
        node_id = backend.add_node(f"TEST:node_{i:08d}", data)
        node_ids.append(node_id)
        
        if (i + 1) % progress_interval == 0:
            elapsed = time.time() - start_time
            rate = (i + 1) / elapsed if elapsed > 0 else 0
            remaining = num_small - (i + 1)
            eta = remaining / rate if rate > 0 else 0
            print(f"  Progress: {format_number(i + 1)}/{format_number(num_small)} "
                  f"({rate:.0f} nodes/sec, ETA: {eta:.0f}s)")
    
    # Store large nodes (chunked)
    if num_large > 0:
        print(f"\nStoring {format_number(num_large)} large nodes (chunked)...")
        for i in range(num_large):
            large_data = b'\xAA' * 2000  # 2000 bytes = 4 chunks
            node_id = backend.add_node_chunked(f"LARGE:node_{i:08d}", large_data)
            chunked_node_ids.append(node_id)
            
            if (i + 1) % max(1, progress_interval // 10) == 0:
                elapsed = time.time() - start_time
                rate = (num_small + i + 1) / elapsed if elapsed > 0 else 0
                remaining = num_large - (i + 1)
                eta = remaining / (rate / 10) if rate > 0 else 0
                print(f"  Progress: {format_number(i + 1)}/{format_number(num_large)} "
                      f"(ETA: {eta:.0f}s)")
    
    store_time = time.time() - start_time
    store_mem = get_memory_usage()
    
    print()
    print(f"  Total time: {store_time:.2f}s")
    print(f"  Average: {store_time / num_nodes * 1000:.2f}ms per node")
    print(f"  Rate: {num_nodes / store_time:.0f} nodes/sec")
    if HAS_PSUTIL:
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
    print(f"  Bytes per node: {file_size / num_nodes:.0f} bytes")
    print()
    
    # Test 2: Query performance (random lookups)
    print("TEST 2: Query Performance (Random Lookups)")
    print("-" * 70)
    
    import random
    random.seed(42)
    num_queries = min(1000, num_nodes // 10)  # 10% of nodes, max 1000
    test_indices = [random.randint(0, len(node_ids) - 1) for _ in range(num_queries)]
    
    start_time = time.time()
    found = 0
    for idx in test_indices:
        node_id = node_ids[idx]
        node = backend.get_node(node_id)
        if node:
            found += 1
    
    query_time = time.time() - start_time
    
    print(f"  Queried: {format_number(num_queries)} random nodes")
    print(f"  Found: {found}/{num_queries}")
    print(f"  Total time: {query_time:.2f}s")
    print(f"  Average: {query_time / num_queries * 1000:.2f}ms per query")
    print(f"  Rate: {num_queries / query_time:.0f} queries/sec")
    print()
    
    # Test 3: Prefix queries
    print("TEST 3: Prefix Query Performance")
    print("-" * 70)
    
    start_time = time.time()
    results = backend.find_by_prefix("TEST:node_", limit=min(1000, num_nodes // 10))
    prefix_time = time.time() - start_time
    
    print(f"  Prefix: 'TEST:node_'")
    print(f"  Results: {len(results)}")
    print(f"  Query time: {prefix_time * 1000:.2f}ms")
    if len(results) > 0:
        print(f"  Average: {prefix_time / len(results) * 1000:.3f}ms per result")
    print()
    
    # Test 4: Chunked data retrieval (if any)
    if len(chunked_node_ids) > 0:
        print("TEST 4: Chunked Data Retrieval")
        print("-" * 70)
        
        num_retrievals = min(10, len(chunked_node_ids))
        start_time = time.time()
        retrieved = 0
        
        for node_id in chunked_node_ids[:num_retrievals]:
            data = backend.get_node_chunked(node_id)
            if data and len(data) == 2000:
                retrieved += 1
        
        chunked_query_time = time.time() - start_time
        
        print(f"  Retrieved: {retrieved}/{num_retrievals} large nodes")
        print(f"  Query time: {chunked_query_time * 1000:.2f}ms")
        print(f"  Average: {chunked_query_time / num_retrievals * 1000:.2f}ms per retrieval")
        print()
    
    # Test 5: Persistence (reload)
    print("TEST 5: Persistence (Reload)")
    print("-" * 70)
    
    # Close and reload
    backend.close()
    del backend
    gc.collect()
    
    load_start = time.time()
    backend2 = RawSynrixBackend(lattice_path, max_nodes=max_nodes)
    load_time = time.time() - load_start
    
    # Verify data
    verify_count = min(100, num_nodes // 100)
    verify_start = time.time()
    verified = 0
    for idx in test_indices[:verify_count]:
        node_id = node_ids[idx]
        node = backend2.get_node(node_id)
        if node and node['name'] == f"TEST:node_{idx:08d}":
            verified += 1
    
    verify_time = time.time() - verify_start
    
    print(f"  Load time: {load_time:.2f}s")
    print(f"  Verified: {verified}/{verify_count} random nodes")
    print(f"  Verify time: {verify_time * 1000:.2f}ms")
    print()
    
    # Final memory usage
    final_mem = get_memory_usage()
    
    print("=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print(f"Total nodes: {format_number(num_nodes)}")
    print(f"  Small nodes: {format_number(num_small)}")
    print(f"  Large nodes: {format_number(num_large)}")
    if num_large > 0:
        print(f"  Total chunks: ~{format_number(num_small + num_large * 4)}")
    print(f"File size: {format_bytes(file_size)}")
    if HAS_PSUTIL:
        print(f"Memory usage: {format_bytes(final_mem - start_mem)}")
    print()
    print("Performance:")
    print(f"  Store: {num_nodes / store_time:.0f} nodes/sec")
    print(f"  Query (random): {num_queries / query_time:.0f} queries/sec")
    print(f"  Prefix query: {len(results) / prefix_time:.0f} results/sec")
    if len(chunked_node_ids) > 0:
        print(f"  Chunked retrieval: {num_retrievals / chunked_query_time:.0f} retrievals/sec")
    print(f"  Save: {save_time:.2f}s")
    print(f"  Load: {load_time:.2f}s")
    print()
    print("✅ All tests completed successfully!")
    
    backend2.close()

def main():
    parser = argparse.ArgumentParser(
        description="Test SYNRIX performance with configurable node counts",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 test_scale_nodes.py 10000          # 10K nodes
  python3 test_scale_nodes.py 100000         # 100K nodes
  python3 test_scale_nodes.py 1000000        # 1M nodes
  python3 test_scale_nodes.py 10000 --chunked-ratio 0.1  # 10K nodes, 10% chunked
        """
    )
    parser.add_argument(
        'num_nodes',
        type=int,
        nargs='?',
        default=10000,
        help='Number of nodes to create (default: 10000)'
    )
    parser.add_argument(
        '--chunked-ratio',
        type=float,
        default=0.01,
        help='Ratio of nodes that should be chunked (0.0-1.0, default: 0.01)'
    )
    parser.add_argument(
        '--progress-interval',
        type=int,
        default=1000,
        help='Print progress every N nodes (default: 1000)'
    )
    
    args = parser.parse_args()
    
    if args.num_nodes < 1:
        print("❌ Error: num_nodes must be >= 1")
        sys.exit(1)
    
    if args.chunked_ratio < 0 or args.chunked_ratio > 1:
        print("❌ Error: chunked_ratio must be between 0.0 and 1.0")
        sys.exit(1)
    
    try:
        test_scale_nodes(args.num_nodes, args.chunked_ratio, args.progress_interval)
    except KeyboardInterrupt:
        print("\n⚠️  Test interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
