#!/usr/bin/env python3
"""Quiet benchmark - saves results to JSON file"""

import sys
import os
import time
import statistics
import json

# Suppress debug output
import sys
sys.stderr = open(os.devnull, 'w')

sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def benchmark_add_nodes(backend, count=1000):
    """Benchmark adding nodes"""
    times = []
    start_total = time.perf_counter()
    
    for i in range(count):
        start = time.perf_counter()
        backend.add_node(f"BENCH:node_{i}", f"data_{i}", 5)
        end = time.perf_counter()
        times.append((end - start) * 1e6)
    
    end_total = time.perf_counter()
    total_time = (end_total - start_total) * 1000
    
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
    results = backend.find_by_prefix("BENCH:", limit=count)
    if len(results) < count:
        count = len(results)
    if count == 0:
        return None
    
    node_ids = [r['id'] for r in results[:count]]
    times = []
    
    for node_id in node_ids:
        start = time.perf_counter()
        node = backend.get_node(node_id)
        end = time.perf_counter()
        times.append((end - start) * 1e6)
    
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
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        results = backend.find_by_prefix(prefix, limit=1000)
        end = time.perf_counter()
        times.append((end - start) * 1e6)
    
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
    """Benchmark WAL flush and checkpoint"""
    start = time.perf_counter()
    backend.flush()
    flush_time = (time.perf_counter() - start) * 1000
    
    start = time.perf_counter()
    backend.checkpoint()
    checkpoint_time = (time.perf_counter() - start) * 1000
    
    return {
        'flush_time_ms': flush_time,
        'checkpoint_time_ms': checkpoint_time
    }

def benchmark_persistence(backend, test_name="bench_persistence"):
    """Benchmark persistence"""
    start = time.perf_counter()
    backend.save()
    save_time = (time.perf_counter() - start) * 1000
    
    node_count_before = len(backend.find_by_prefix("BENCH:", limit=10000))
    backend.close()
    
    start = time.perf_counter()
    backend2 = RawSynrixBackend(f"{test_name}.lattice", max_nodes=1000000, evaluation_mode=False)
    load_time = (time.perf_counter() - start) * 1000
    
    node_count_after = len(backend2.find_by_prefix("BENCH:", limit=10000))
    backend2.close()
    
    return {
        'save_time_ms': save_time,
        'load_time_ms': load_time,
        'nodes_before': node_count_before,
        'nodes_after': node_count_after,
        'persistence_ok': node_count_before == node_count_after
    }

def main():
    backend = RawSynrixBackend("benchmark_quiet.lattice", max_nodes=1000000, evaluation_mode=False)
    
    results = {
        'platform': 'Windows (MSYS2 MinGW-w64)',
        'dll': 'libsynrix.dll',
        'timestamp': time.time()
    }
    
    try:
        results['add_nodes'] = benchmark_add_nodes(backend, count=1000)
        results['get_nodes'] = benchmark_get_nodes(backend, count=1000)
        results['prefix_queries'] = benchmark_prefix_queries(backend, prefix="BENCH:", iterations=100)
        results['wal_operations'] = benchmark_wal_operations(backend)
        results['persistence'] = benchmark_persistence(backend, "benchmark_quiet")
    finally:
        backend.close()
    
    # Save to JSON
    with open("benchmark_results_windows.json", "w") as f:
        json.dump(results, f, indent=2)
    
    # Print summary
    print("BENCHMARK RESULTS")
    print("="*70)
    print(f"Add nodes:     {results['add_nodes']['avg_us']:.2f} us/node, {results['add_nodes']['throughput_nodes_per_sec']:.0f} nodes/sec")
    print(f"Get nodes:     {results['get_nodes']['avg_us']:.2f} us/lookup")
    print(f"Prefix query:  {results['prefix_queries']['avg_us']:.2f} us/query")
    print(f"WAL flush:     {results['wal_operations']['flush_time_ms']:.2f} ms")
    print(f"WAL checkpoint: {results['wal_operations']['checkpoint_time_ms']:.2f} ms")
    print(f"Save:          {results['persistence']['save_time_ms']:.2f} ms")
    print(f"Load:          {results['persistence']['load_time_ms']:.2f} ms")
    print(f"Persistence:   {'PASS' if results['persistence']['persistence_ok'] else 'FAIL'}")
    print(f"\nResults saved to: benchmark_results_windows.json")
    
    return results

if __name__ == "__main__":
    main()
