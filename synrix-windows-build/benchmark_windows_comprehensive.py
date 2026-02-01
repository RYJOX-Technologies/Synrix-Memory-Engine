#!/usr/bin/env python3
"""
Comprehensive SYNRIX Windows Performance Benchmark
Tests all major operations and provides detailed metrics
"""

import sys
import os
import time
import statistics

# Add Python SDK to path
sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def benchmark_add_nodes(backend, count=1000):
    """Benchmark adding nodes"""
    print(f"\n{'='*70}")
    print(f"Benchmarking: Adding {count} nodes...")
    print(f"{'='*70}")
    
    times = []
    start_total = time.perf_counter()
    
    for i in range(count):
        start = time.perf_counter()
        backend.add_node(f"BENCH:node_{i}", f"data_{i}", 5)
        end = time.perf_counter()
        times.append((end - start) * 1e6)  # Convert to microseconds
    
    end_total = time.perf_counter()
    total_time = (end_total - start_total) * 1000  # Convert to milliseconds
    
    print(f"  Total time: {total_time:.2f} ms")
    print(f"  Average: {statistics.mean(times):.2f} us per node")
    print(f"  Median: {statistics.median(times):.2f} us per node")
    print(f"  Min: {min(times):.2f} us")
    print(f"  Max: {max(times):.2f} us")
    print(f"  P95: {sorted(times)[int(len(times) * 0.95)]:.2f} us")
    print(f"  P99: {sorted(times)[int(len(times) * 0.99)]:.2f} us")
    print(f"  Throughput: {count / (total_time / 1000):.0f} nodes/sec")
    
    return {
        'count': count,
        'total_time_ms': total_time,
        'avg_us': statistics.mean(times),
        'median_us': statistics.median(times),
        'min_us': min(times),
        'max_us': max(times),
        'p95_us': sorted(times)[int(len(times) * 0.95)],
        'p99_us': sorted(times)[int(len(times) * 0.99)],
        'throughput_nodes_per_sec': count / (total_time / 1000)
    }

def benchmark_get_nodes(backend, count=1000):
    """Benchmark retrieving nodes by ID"""
    print(f"\n{'='*70}")
    print(f"Benchmarking: Retrieving {count} nodes (O(1) lookup)...")
    print(f"{'='*70}")
    
    # First, get some node IDs
    results = backend.find_by_prefix("BENCH:", limit=count)
    if len(results) < count:
        count = len(results)
    
    if count == 0:
        print("  No nodes to retrieve")
        return None
    
    node_ids = [r['id'] for r in results[:count]]
    
    times = []
    for node_id in node_ids:
        start = time.perf_counter()
        node = backend.get_node(node_id)
        end = time.perf_counter()
        times.append((end - start) * 1e6)  # Convert to microseconds
    
    print(f"  Average: {statistics.mean(times):.2f} us per lookup")
    print(f"  Median: {statistics.median(times):.2f} us per lookup")
    print(f"  P95: {sorted(times)[int(len(times) * 0.95)]:.2f} us")
    print(f"  P99: {sorted(times)[int(len(times) * 0.99)]:.2f} us")
    print(f"  Min: {min(times):.2f} us")
    print(f"  Max: {max(times):.2f} us")
    
    return {
        'count': count,
        'avg_us': statistics.mean(times),
        'median_us': statistics.median(times),
        'p95_us': sorted(times)[int(len(times) * 0.95)],
        'p99_us': sorted(times)[int(len(times) * 0.99)],
        'min_us': min(times),
        'max_us': max(times)
    }

def benchmark_prefix_queries(backend, prefix="BENCH:", iterations=100):
    """Benchmark prefix queries"""
    print(f"\n{'='*70}")
    print(f"Benchmarking: Prefix queries '{prefix}' ({iterations} iterations)...")
    print(f"{'='*70}")
    
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        results = backend.find_by_prefix(prefix, limit=1000)
        end = time.perf_counter()
        times.append((end - start) * 1e6)  # Convert to microseconds
    
    print(f"  Average: {statistics.mean(times):.2f} us per query")
    print(f"  Median: {statistics.median(times):.2f} us per query")
    print(f"  P95: {sorted(times)[int(len(times) * 0.95)]:.2f} us")
    print(f"  P99: {sorted(times)[int(len(times) * 0.99)]:.2f} us")
    print(f"  Min: {min(times):.2f} us")
    print(f"  Max: {max(times):.2f} us")
    print(f"  Results per query: {len(backend.find_by_prefix(prefix, limit=1000))} nodes")
    
    return {
        'iterations': iterations,
        'avg_us': statistics.mean(times),
        'median_us': statistics.median(times),
        'p95_us': sorted(times)[int(len(times) * 0.95)],
        'p99_us': sorted(times)[int(len(times) * 0.99)],
        'min_us': min(times),
        'max_us': max(times),
        'results_per_query': len(backend.find_by_prefix(prefix, limit=1000))
    }

def benchmark_wal_operations(backend):
    """Benchmark WAL flush and checkpoint operations"""
    print(f"\n{'='*70}")
    print(f"Benchmarking: WAL operations...")
    print(f"{'='*70}")
    
    # Flush
    start = time.perf_counter()
    backend.flush()
    flush_time = (time.perf_counter() - start) * 1000  # Convert to ms
    print(f"  Flush time: {flush_time:.2f} ms")
    
    # Checkpoint
    start = time.perf_counter()
    backend.checkpoint()
    checkpoint_time = (time.perf_counter() - start) * 1000  # Convert to ms
    print(f"  Checkpoint time: {checkpoint_time:.2f} ms")
    
    return {
        'flush_time_ms': flush_time,
        'checkpoint_time_ms': checkpoint_time
    }

def benchmark_persistence(backend, test_name="bench_persistence"):
    """Benchmark persistence (save/load cycle)"""
    print(f"\n{'='*70}")
    print(f"Benchmarking: Persistence (save/load cycle)...")
    print(f"{'='*70}")
    
    # Save
    start = time.perf_counter()
    backend.save()
    save_time = (time.perf_counter() - start) * 1000  # Convert to ms
    print(f"  Save time: {save_time:.2f} ms")
    
    # Close and reopen
    node_count_before = len(backend.find_by_prefix("BENCH:", limit=10000))
    backend.close()
    
    start = time.perf_counter()
    backend2 = RawSynrixBackend(f"{test_name}.lattice", max_nodes=1000000, evaluation_mode=False)
    load_time = (time.perf_counter() - start) * 1000  # Convert to ms
    print(f"  Load time: {load_time:.2f} ms")
    
    node_count_after = len(backend2.find_by_prefix("BENCH:", limit=10000))
    backend2.close()
    
    print(f"  Nodes before: {node_count_before}")
    print(f"  Nodes after: {node_count_after}")
    print(f"  Persistence: {'✅ PASS' if node_count_before == node_count_after else '❌ FAIL'}")
    
    return {
        'save_time_ms': save_time,
        'load_time_ms': load_time,
        'nodes_before': node_count_before,
        'nodes_after': node_count_after,
        'persistence_ok': node_count_before == node_count_after
    }

def main():
    print("="*70)
    print("SYNRIX Windows Performance Benchmark (Comprehensive)")
    print("="*70)
    print(f"Platform: Windows (MSYS2 MinGW-w64)")
    print(f"DLL: libsynrix.dll")
    print(f"{'='*70}")
    
    # Create backend
    backend = RawSynrixBackend("benchmark_comprehensive.lattice", max_nodes=1000000, evaluation_mode=False)
    
    results = {}
    
    # Run benchmarks
    try:
        results['add_nodes'] = benchmark_add_nodes(backend, count=1000)
        results['get_nodes'] = benchmark_get_nodes(backend, count=1000)
        results['prefix_queries'] = benchmark_prefix_queries(backend, prefix="BENCH:", iterations=100)
        results['wal_operations'] = benchmark_wal_operations(backend)
        results['persistence'] = benchmark_persistence(backend, "benchmark_comprehensive")
        
        # Summary
        print(f"\n{'='*70}")
        print("SUMMARY")
        print(f"{'='*70}")
        print(f"Add nodes:     {results['add_nodes']['avg_us']:.2f} us/node (avg), {results['add_nodes']['throughput_nodes_per_sec']:.0f} nodes/sec")
        print(f"Get nodes:     {results['get_nodes']['avg_us']:.2f} us/lookup (avg)")
        print(f"Prefix query:  {results['prefix_queries']['avg_us']:.2f} us/query (avg)")
        print(f"WAL flush:     {results['wal_operations']['flush_time_ms']:.2f} ms")
        print(f"WAL checkpoint: {results['wal_operations']['checkpoint_time_ms']:.2f} ms")
        print(f"Save:          {results['persistence']['save_time_ms']:.2f} ms")
        print(f"Load:          {results['persistence']['load_time_ms']:.2f} ms")
        print(f"Persistence:   {'✅ PASS' if results['persistence']['persistence_ok'] else '❌ FAIL'}")
        
    finally:
        backend.close()
    
    return results

if __name__ == "__main__":
    results = main()
