#!/usr/bin/env python3
"""Simple SYNRIX benchmark - quick test"""

import sys
import os
import time
import statistics

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python-sdk"))

from synrix.raw_backend import RawSynrixBackend

def main():
    print("SYNRIX Windows Benchmark")
    print("=" * 60)
    
    lattice_path = "benchmark_simple.lattice"
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    
    backend = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    print("Platform: Windows (MSYS2 MinGW-w64)")
    print()
    
    # Test 1: Add nodes
    print("Test 1: Adding 1000 nodes...")
    times = []
    start_total = time.perf_counter()
    node_ids = []
    for i in range(1000):
        start = time.perf_counter()
        node_id = backend.add_node(f"BENCH:node_{i}", f"Test {i}", node_type=3)
        end = time.perf_counter()
        times.append((end - start) * 1000000)
        node_ids.append(node_id)
    end_total = time.perf_counter()
    total_ms = (end_total - start_total) * 1000
    
    print(f"  Total: {total_ms:.2f} ms")
    print(f"  Avg: {statistics.mean(times):.2f} us/node")
    print(f"  Throughput: {1000 / (total_ms / 1000):.0f} nodes/sec")
    print()
    
    # Test 2: Get nodes
    print("Test 2: Retrieving 100 nodes...")
    get_times = []
    for i in range(100):
        node_id = node_ids[i % len(node_ids)]
        start = time.perf_counter()
        node = backend.get_node(node_id)
        end = time.perf_counter()
        get_times.append((end - start) * 1000000)
    
    print(f"  Avg: {statistics.mean(get_times):.3f} us/lookup")
    print(f"  Median: {statistics.median(get_times):.3f} us")
    print()
    
    # Test 3: Prefix query
    print("Test 3: Prefix query (BENCH:) - 50 iterations...")
    query_times = []
    for _ in range(50):
        start = time.perf_counter()
        results = backend.find_by_prefix("BENCH:", limit=100)
        end = time.perf_counter()
        query_times.append((end - start) * 1000000)
    
    print(f"  Avg: {statistics.mean(query_times):.2f} us/query")
    print(f"  Results: {len(results)} nodes")
    print()
    
    # Summary
    print("=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Node addition:     {statistics.mean(times):.2f} us/node")
    print(f"Node lookup (O1):  {statistics.mean(get_times):.3f} us")
    print(f"Prefix query (Ok): {statistics.mean(query_times):.2f} us")
    print()
    print("Expected Linux (for comparison):")
    print("  Node addition:     ~1-10 us/node")
    print("  Node lookup (O1):  ~0.1-1.0 us")
    print("  Prefix query (Ok): ~10-100 us")
    print()
    
    backend.close()
    print("Done!")

if __name__ == "__main__":
    main()
