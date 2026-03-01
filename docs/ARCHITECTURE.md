# Synrix Architecture

## What Is the Binary Lattice?

Synrix stores knowledge as a **Binary Lattice** — a dense, memory-mapped array of fixed-size nodes. Not a graph. Not a key-value store. A rigid, mathematically predictable structure.

### Core Design

| Principle | Implementation |
|-----------|-----------------|
| **Rigid structure** | 1216-byte nodes, 64-byte cache-aligned, contiguous storage |
| **O(1) lookup** | Arithmetic addressing: `offset = id * node_size` |
| **O(k) queries** | Dynamic prefix index — cost scales with matches (k), not corpus (N) |
| **No pointer chasing** | Direct memory access. CPU cache optimal. |

### Why O(k) Queries?

Traditional databases: query cost = O(N) or O(log N) with corpus size.

Synrix: query cost = O(k) where k = number of matches. At 500K nodes, 1000 matches take ~0.022 ms. The prefix index maps `LEARNING_PYTHON_*` → list of node IDs. No full scan.

### CPU-Cache Optimal Design

- **64-byte alignment** — matches L1 cache line, prevents false sharing
- **Memory-mapped files** — lattice can exceed RAM (20–30×), OS handles paging
- **Lock-free reads** — seqlocks for sub-microsecond concurrent access
- **WAL batching** — 100× reduction in fsync overhead

## Data Flow

```
Your App → Python SDK / C API → libsynrix → memory-mapped .lattice file
                                      ↓
                              WAL (.lattice.wal) for durability
```

## Node Structure

```
Node (1216 bytes):
  - id (64-bit)
  - type (enum)
  - confidence
  - name (64 bytes) — semantic prefix: ISA_ADD, LEARNING_PYTHON_ASYNCIO
  - payload (512 bytes) — text or binary
  - metadata (timestamps, flags)
```

## Retrieval Semantics

- **Prefix search**: `find_by_prefix("LEARNING_PYTHON_", limit=10)` → O(k)
- **Direct read**: `get_node(id)` → O(1)
- **No embeddings**: Semantic naming replaces vector similarity.

## Further Reading

- [Technical Whitepaper](../Synrix_Technical_Whitepaper_v1.9.md)
- [Benchmarks](BENCHMARKS.md)
