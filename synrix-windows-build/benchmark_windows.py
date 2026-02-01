#!/usr/bin/env python3
"""
Windows Performance Benchmark for SYNRIX
Compares Windows DLL performance with expected Linux performance
"""

import sys
import os
import time
import statistics

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python-sdk"))

from synrix.raw_backend import RawSynrixBackend

def benchmark_add_nodes(backend, count=1000):
    """Benchmark node addition performance"""
    times = []
    
    print(f"Benchmarking: Adding {count} nodes...")
    start_total = time.perf_counter()
    
    for i in range(count):
        start = time.perf_counter()
        node_id = backend.add_node(
            f"BENCH:node_{i}",
            f"Test data for node {i}",
            node_type=3  # PATTERN
        )
        end = time.perf_counter()
        times.append((end - start) * 1000000)  # Convert to microseconds
    
    end_total = time.perf_counter()
    total_time = (end_total - start_total) * 1000  # Convert to milliseconds
    
    return {
        "count": count,
        "total_time_ms": total_time,
        "avg_time_us": statistics.mean(times),
        "min_time_us": min(times),
        "max_time_us": max(times),
        "median_time_us": statistics.median(times),
        "throughput_nodes_per_sec": count / (total_time / 1000)
    }

def benchmark_get_nodes(backend, node_ids, count=100):
    """Benchmark node retrieval (O(1) lookup)"""
    times = []
    
    print(f"Benchmarking: Retrieving {count} nodes (O(1) lookup)...")
    
    for i in range(min(count, len(node_ids))):
        node_id = node_ids[i % len(node_ids)]
        start = time.perf_counter()
        node = backend.get_node(node_id)
        end = time.perf_counter()
        times.append((end - start) * 1000000)  # Convert to microseconds
    
    return {
        "count": count,
        "avg_time_us": statistics.mean(times),
        "min_time_us": min(times),
        "max_time_us": max(times),
        "median_time_us": statistics.median(times),
        "p95_time_us": sorted(times)[int(len(times) * 0.95)],
        "p99_time_us": sorted(times)[int(len(times) * 0.99)]
    }

def benchmark_prefix_queries(backend, prefix, iterations=100):
    """Benchmark prefix query performance (O(k) semantic search)"""
    times = []
    result_counts = []
    
    print(f"Benchmarking: Prefix queries '{prefix}' ({iterations} iterations)...")
    
    for i in range(iterations):
        start = time.perf_counter()
        results = backend.find_by_prefix(prefix, limit=100)
        end = time.perf_counter()
        times.append((end - start) * 1000000)  # Convert to microseconds
        result_counts.append(len(results))
        if (i + 1) % 10 == 0:
            print(f"  Progress: {i + 1}/{iterations} queries completed...")
    
    return {
        "prefix": prefix,
        "iterations": iterations,
        "avg_time_us": statistics.mean(times),
        "min_time_us": min(times),
        "max_time_us": max(times),
        "median_time_us": statistics.median(times),
        "avg_results": statistics.mean(result_counts),
        "throughput_queries_per_sec": iterations / (sum(times) / 1000000)
    }

def benchmark_batch_operations(backend, batch_size=100):
    """Benchmark batch add operations"""
    times = []
    
    print(f"Benchmarking: Batch operations (batch size: {batch_size})...")
    
    for batch in range(10):
        if batch % 2 == 0:
            print(f"  Progress: batch {batch + 1}/10...")
        start = time.perf_counter()
        node_ids = []
        for i in range(batch_size):
            node_id = backend.add_node(
                f"BATCH:batch_{batch}_node_{i}",
                f"Batch data {batch}-{i}",
                node_type=5
            )
            node_ids.append(node_id)
        end = time.perf_counter()
        times.append((end - start) * 1000)  # Convert to milliseconds
    
    return {
        "batch_size": batch_size,
        "batches": 10,
        "avg_batch_time_ms": statistics.mean(times),
        "throughput_nodes_per_sec": (batch_size * 10) / (sum(times) / 1000)
    }

def main():
    try:
        print("=" * 70)
        print("SYNRIX Windows Performance Benchmark")
        print("=" * 70)
        print()
        
        # Initialize backend
        lattice_path = "benchmark_test.lattice"
        if os.path.exists(lattice_path):
            os.remove(lattice_path)
        
        backend = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
        print("Platform: Windows (MSYS2 MinGW-w64)")
        print("DLL: libsynrix.dll")
        print()
        
        results = {}
        
        # Test 1: Node addition
        print("-" * 70)
        add_results = benchmark_add_nodes(backend, count=1000)
        results["add_nodes"] = add_results
        print(f"  Total time: {add_results['total_time_ms']:.2f} ms")
        print(f"  Average: {add_results['avg_time_us']:.2f} us per node")
        print(f"  Median: {add_results['median_time_us']:.2f} us per node")
        print(f"  Min: {add_results['min_time_us']:.2f} us")
        print(f"  Max: {add_results['max_time_us']:.2f} us")
        print(f"  Throughput: {add_results['throughput_nodes_per_sec']:.0f} nodes/sec")
        print()
        
        # Test 2: Node retrieval (O(1))
        print("-" * 70)
        # Get some node IDs from the add test
        get_results = benchmark_get_nodes(backend, [i for i in range(1, 1001)], count=1000)
        results["get_nodes"] = get_results
        print(f"  Average: {get_results['avg_time_us']:.3f} us per lookup")
        print(f"  Median: {get_results['median_time_us']:.3f} us per lookup")
        print(f"  P95: {get_results['p95_time_us']:.3f} us")
        print(f"  P99: {get_results['p99_time_us']:.3f} us")
        print(f"  Min: {get_results['min_time_us']:.3f} us")
        print(f"  Max: {get_results['max_time_us']:.3f} us")
        print()
        
        # Test 3: Prefix queries (O(k))
        print("-" * 70)
        prefix_results = benchmark_prefix_queries(backend, "BENCH:", iterations=50)
        results["prefix_query"] = prefix_results
        print(f"  Prefix: '{prefix_results['prefix']}'")
        print(f"  Average: {prefix_results['avg_time_us']:.2f} us per query")
        print(f"  Median: {prefix_results['median_time_us']:.2f} us per query")
        print(f"  Avg results: {prefix_results['avg_results']:.1f} nodes")
        print(f"  Throughput: {prefix_results['throughput_queries_per_sec']:.0f} queries/sec")
        print()
        
        # Test 4: Batch operations
        print("-" * 70)
        batch_results = benchmark_batch_operations(backend, batch_size=100)
        results["batch_ops"] = batch_results
        print(f"  Batch size: {batch_results['batch_size']}")
        print(f"  Avg batch time: {batch_results['avg_batch_time_ms']:.2f} ms")
        print(f"  Throughput: {batch_results['throughput_nodes_per_sec']:.0f} nodes/sec")
        print()
        
        # Test 5: Large prefix query (more nodes)
        print("-" * 70)
        # Add more nodes for larger query test
        print("Adding 5000 more nodes for large query test...")
        for i in range(5000):
            backend.add_node(f"LARGE:node_{i}", f"Large test {i}", node_type=3)
        
        large_query = benchmark_prefix_queries(backend, "LARGE:", iterations=50)
        results["large_prefix_query"] = large_query
        print(f"  Prefix: '{large_query['prefix']}' (5000 nodes)")
        print(f"  Average: {large_query['avg_time_us']:.2f} us per query")
        print(f"  Avg results: {large_query['avg_results']:.1f} nodes")
        print()
        
        # Summary
        print("=" * 70)
        print("BENCHMARK SUMMARY")
        print("=" * 70)
        print(f"Platform: Windows (MSYS2 MinGW-w64)")
        print(f"Lattice file: {lattice_path}")
        print()
        print("Key Metrics:")
        print(f"  Node addition:     {add_results['avg_time_us']:.2f} us/node")
        print(f"  Node lookup (O1):  {get_results['avg_time_us']:.3f} us")
        print(f"  Prefix query (Ok): {prefix_results['avg_time_us']:.2f} us")
        print(f"  Large query (5k):  {large_query['avg_time_us']:.2f} us")
        print()
        print("Expected Linux Performance (for comparison):")
        print("  Node addition:     ~1-10 us/node")
        print("  Node lookup (O1):  ~0.1-1.0 us")
        print("  Prefix query (Ok): ~10-100 us")
        print()
        
        backend.close()
        
        # Save results to file
        import json
        with open("benchmark_results_windows.json", "w") as f:
            json.dump(results, f, indent=2)
        print("Results saved to: benchmark_results_windows.json")
        print()
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
