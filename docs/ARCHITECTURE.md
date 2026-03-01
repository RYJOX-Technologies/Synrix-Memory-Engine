# Synrix Architecture

## What Is the Binary Lattice?

Synrix stores knowledge as a **Binary Lattice** — a dense, memory-mapped array of fixed-size nodes. Not a graph. Not a key-value store. A rigid, mathematically predictable structure.

### Core Design

| Principle | Implementation |
|-----------|-----------------|
| **Rigid structure** | Fixed-size, cache-aligned nodes; contiguous storage |
| **O(1) lookup** | Arithmetic addressing by node ID |
| **O(k) queries** | Prefix index — cost scales with matches (k), not corpus (N) |
| **No pointer chasing** | Direct memory access. CPU cache friendly. |

### Why O(k) Queries?

Traditional databases: query cost = O(N) or O(log N) with corpus size.

Synrix: query cost = O(k) where k = number of matches. At 500K nodes, 1000 matches take ~0.022 ms. The prefix index maps `LEARNING_PYTHON_*` → list of node IDs. No full scan.

### CPU-Cache Optimal Design

- **Cache-line alignment** — nodes aligned for L1/cache efficiency
- **Memory-mapped files** — lattice can exceed RAM; OS handles paging
- **Lock-free reads** — sub-microsecond concurrent access
- **WAL batching** — batched durability for high throughput

## Data Flow

```
Your App → Python SDK / C API → libsynrix → memory-mapped .lattice file
                                      ↓
                              WAL (.lattice.wal) for durability
```

## Node Structure

Each node is fixed-size and cache-aligned, with:

- **id** — unique node identifier
- **type** — node type (e.g. LEARNING, PATTERN, PRIMITIVE)
- **confidence** — optional confidence score
- **name** — semantic prefix (e.g. `ISA_ADD`, `LEARNING_PYTHON_ASYNCIO`) used for prefix search
- **payload** — text or binary data
- **metadata** — timestamps, flags

## Retrieval Semantics

- **Prefix search**: `find_by_prefix("LEARNING_PYTHON_", limit=10)` → O(k)
- **Direct read**: `get_node(id)` → O(1)
- **No embeddings**: Semantic naming replaces vector similarity.

## Further Reading

- [Benchmarks](BENCHMARKS.md)
