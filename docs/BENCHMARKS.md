# Synrix Benchmarks

## Validated Performance

### Jetson Orin Nano (8 GB RAM, NVMe)

| Metric | Value |
|--------|-------|
| Hot-read latency | 192 ns |
| Warm-read average | 3.2 μs |
| Durable write | ~28 μs |
| Sustained ingestion | 512 MB/s |
| Max validated scale | 500K nodes (O(k) confirmed) |
| Max supported | 50M nodes (47.68 GB) |

### Windows x86_64 (Recent hardware)

| Metric | Value | Notes |
|--------|-------|-------|
| Prefix query (5K nodes) | 0.36ms | First query after adds |
| Prefix query (50K nodes) | 0.31ms | **Critical test** - proves O(k) at scale |
| Prefix query (100K nodes) | 0.07ms | Ongoing queries |
| Add latency (with indexing) | ~0.105ms | Includes incremental index update |

**The 50K test is the proof:** Before optimization, crossing 10K nodes would trigger an O(n) index rebuild on first query (50-100ms). Now it's **0.31ms** - proving true O(k) scaling with incremental index updates at all scales.

## How We Measured

- **O(1) lookup**: `lattice_get_node_data` — direct memory offset
- **O(k) prefix search**: `lattice_find_nodes_by_name` — prefix index + iteration
- **Timing**: `clock_gettime(CLOCK_MONOTONIC)` nanosecond precision

## Run Your Own Benchmarks

### Linux (Jetson, Pi, x86_64)

```bash
# Quick latency diagnostic (1000 iterations)
./tools/run_query_latency_diagnostic.sh

# Full P99 benchmark (100k iterations, O(1) + O(k))
./tools/run_extended_p99_benchmark.sh 100000 1000000
```

### Windows x86_64

```bash
# O(k) scaling verification at scale
python scripts/test_ok_indexing_fix.py

# This test proves:
# - No artificial limits (tests up to 100K nodes)
# - O(k) queries at all scales (<1ms)
# - No O(n) rebuild spikes
```

## Comparison vs Other Systems

| | Synrix | Mem0 | Qdrant | ChromaDB |
|---|---|---|---|---|
| Read latency | 192 ns (hot) | 1.4s p95 | 4 ms p50 | 12 ms p50 |
| Embedding model | No | Yes | Yes | Yes |
| Durable + crash proof | Yes | No | Partial | No |

*Caveats: Mem0/Qdrant latency includes embedding + retrieval. Synrix uses prefix lookup, not fuzzy similarity.*

## Latency Distribution

See [SYNRIX_QUERY_LATENCY_CLAIMS.md](SYNRIX_QUERY_LATENCY_CLAIMS.md) for:
- What 96/192 ns actually measure
- O(1) vs O(k) distinction
- Per-query diagnostic tool

## Example Output

```
Query Latency Diagnostic
  A) lattice_find_nodes_by_name:  Min 500 ns,  Avg 1.2 μs
  B) lattice_get_node_data (O(1)): Min 200 ns,  Avg 400 ns
```
