# Synrix Query Latency: What Is Actually Measured

## The 96–192 ns Question

**Where do 96/192/126 ns come from?**

These numbers appear in marketing/whitepaper claims. This document clarifies what is measured and how to verify.

---

## What the Benchmarks Measure

| Benchmark | Operation | What It Measures |
|-----------|-----------|------------------|
| **O(1) lookup** | `lattice_get_node_data(lattice, id, &node)` | Single node read by ID – direct offset + memory access |
| **O(k) prefix search** | `lattice_find_nodes_by_name(lattice, prefix, ids, max)` | Prefix index lookup + iterate k matching nodes |
| **O(k) full flow** | prefix search + k×`lattice_get_node_data` | What Python SDK does: find + fetch each result |

---

## Answer to “Are these per-query latencies?”

| Claim | Likely Source | What It Is |
|-------|---------------|------------|
| **192 ns hot-read** | Whitepaper “minimum hot-read latency” | Best-case **O(1) direct lookup** (`lattice_get_node_data`) – single node, hot in cache |
| **96 ns** | Not explicitly documented | Possibly O(1) min on a faster run, or a different measurement |
| **3.2 μs warm-read** | Whitepaper | O(1) or O(k) **average** when data is in RAM but not L1/L2 |

---

## Critical Distinction

```
O(1) lattice_get_node_data:  ~96–192 ns min (single memory access + offset)
O(k) lattice_find_nodes_by_name:  scales with k; typically 0.5–5 μs for k=100
O(k) full flow (find + k fetches):  ~0.5–3 μs for k=100 (extended_p99 numbers)
```

**The 192 ns claim is for the O(1) path**, not the full semantic prefix query. The prefix query (`find_by_prefix`) does more work: index lookup + iteration.

---

## How to Verify: Query Latency Diagnostic

Run the diagnostic to get **per-call** min/max/avg and a distribution:

```bash
./tools/run_query_latency_diagnostic.sh [lattice_path] [iterations]
```

Example:
```bash
./tools/run_query_latency_diagnostic.sh /tmp/mylattice.lattice 1000
```

**Prerequisites:**
- A lattice file with data (e.g. from extended benchmark, or code_gen)
- If no lattice exists and the 25k global limit is not reached, the diagnostic creates a minimal one

**Output:**
- **Test A**: `lattice_find_nodes_by_name` (prefix search) – min, max, avg, p50, p99, histogram
- **Test B**: `lattice_get_node_data` (O(1) direct read) – same stats

---

## Interpreting Results

| If you see… | Interpretation |
|-------------|----------------|
| O(1) min 96–200 ns | Consistent with RAM-speed access; CPU-optimal |
| O(1) min &lt; 100 ns | L1/L2 cache; very hot path |
| O(k) prefix min 200–500 ns | Index + small k; still memory-speed |
| O(k) p99 &gt; 10 μs | Cold cache / disk paging |
| O(k) p99 &gt; 1 ms | Disk I/O or large k |

---

## Throughput Math (If O(1) Is ~96 ns)

```
Per-query: 96 ns
Queries per second: 1e9 / 96 ≈ 10.4 million
1000 queries: 96 μs
```

So **0.16 ms for 1000+ queries** would imply either:
1. Queries are pipelined/batched
2. Many are served from cache (faster than 96 ns)
3. Fewer than ~1600 queries in that 0.16 ms window

---

## Files

| File | Purpose |
|------|---------|
| `tools/query_latency_diagnostic.c` | Per-call nanosecond timing, histogram |
| `tools/run_query_latency_diagnostic.sh` | Build + run diagnostic |
| `tools/extended_p99_benchmark.c` | O(1) and O(k) benchmarks with percentiles |
| `tools/locality_benchmark.c` | Hot/warm/cold cache scenarios |
