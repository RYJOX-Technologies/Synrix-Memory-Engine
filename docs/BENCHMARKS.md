# Synrix Benchmarks

## Validated Performance (Jetson Orin Nano, 8 GB RAM, NVMe)

| Metric | Value |
|--------|-------|
| Hot-read latency | 192 ns |
| Warm-read average | 3.2 μs |
| Durable write | ~28 μs |
| Sustained ingestion | 512 MB/s |
| Max validated scale | 500K nodes (O(k) confirmed) |
| Max supported | 50M nodes (47.68 GB) |

## How We Measured

- **O(1) lookup**: `lattice_get_node_data` — direct memory offset
- **O(k) prefix search**: `lattice_find_nodes_by_name` — prefix index + iteration
- **Timing**: `clock_gettime(CLOCK_MONOTONIC)` nanosecond precision

## Run Your Own Benchmarks

```bash
# Quick latency diagnostic (1000 iterations)
./tools/run_query_latency_diagnostic.sh

# Full P99 benchmark (100k iterations, O(1) + O(k))
./tools/run_extended_p99_benchmark.sh 100000 1000000
```

## Comparison vs Other Systems

| | Synrix | Mem0 | Qdrant | ChromaDB |
|---|---|---|---|---|
| Read latency | 192 ns (hot) | 1.4s p95 | 4 ms p50 | 12 ms p50 |
| Embedding model | No | Yes | Yes | Yes |
| ACID + crash proof | Yes | No | Partial | No |

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
