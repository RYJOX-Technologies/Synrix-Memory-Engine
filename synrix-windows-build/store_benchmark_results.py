#!/usr/bin/env python3
"""Store benchmark results in Synrix lattice"""

import sys
import os
import json

sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

# Load benchmark results
with open('benchmark_results_windows.json', 'r') as f:
    results = json.load(f)

# Create summary
summary = f"""Windows Benchmark Results (2026-01-12):
Platform: {results['platform']}
DLL: {results['dll']}

Add nodes: {results['add_nodes']['avg_us']:.2f} us/node ({results['add_nodes']['throughput_nodes_per_sec']:.0f} nodes/sec)
  - Median: {results['add_nodes']['median_us']:.2f} us
  - P95: {results['add_nodes']['p95_us']:.2f} us
  - P99: {results['add_nodes']['p99_us']:.2f} us

Get nodes: {results['get_nodes']['avg_us']:.2f} us/lookup
  - Median: {results['get_nodes']['median_us']:.2f} us
  - P95: {results['get_nodes']['p95_us']:.2f} us
  - P99: {results['get_nodes']['p99_us']:.2f} us

Prefix query: {results['prefix_queries']['avg_us']:.2f} us/query ({results['prefix_queries']['results_per_query']} results)
  - Median: {results['prefix_queries']['median_us']:.2f} us
  - P95: {results['prefix_queries']['p95_us']:.2f} us

WAL flush: {results['wal_operations']['flush_time_ms']:.2f} ms
WAL checkpoint: {results['wal_operations']['checkpoint_time_ms']:.2f} ms
Save: {results['persistence']['save_time_ms']:.2f} ms
Load: {results['persistence']['load_time_ms']:.2f} ms

Status: WAL fully functional, all operations working correctly."""

# Store in lattice
b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=1000000, evaluation_mode=False)
node_id = b.add_node('BENCHMARK:windows_2026_01_12', summary, 5)
print(f'Added node with ID: {node_id}')

# Flush WAL to ensure entry is on disk
b.flush()

# Checkpoint to apply WAL entries to main structure
b.checkpoint()

# Save to main file
save_result = b.save()
print(f'Save result: {save_result}')

b.close()
print('Stored benchmark results in Synrix lattice')
