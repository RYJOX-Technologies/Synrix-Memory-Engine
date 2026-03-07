# Synrix Architecture

## What Is the Lattice?

Synrix stores data as a **lattice** — a dense, memory-mapped array of fixed-size nodes. Not a graph. Not a key-value store. A rigid, cache-optimal structure with mathematically predictable access costs.

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

## Why Not SQLite?

The "62 lines of SQLite" argument is valid for simple agent memory workloads — store a string, retrieve by prefix, survive a crash. For those workloads, SQLite with WAL mode and an indexed text column is a legitimate alternative with real advantages (open source, auditable, no node limits, full SQL).

It breaks down for workloads that need **in-place updates to typed numeric fields in a hot execution loop**.

Consider pattern learning: every time a pattern executes, its `success_rate` is updated via exponential moving average, its `frequency` incremented, its `last_used` timestamp written, and its `performance_gain` adjusted. In Synrix, this is a **direct memory write to known byte offsets within a memory-mapped struct** — the OS page is already mapped, the write goes straight to RAM, no locks, no query planner, no serialization.

The SQLite equivalent:

```sql
UPDATE nodes 
SET success_rate = (success_rate * 0.9) + (? * 0.1),
    frequency    = frequency + 1,
    last_used    = ?
WHERE id = ?
```

That's a B-tree lookup, row fetch, arithmetic, row write, WAL journal entry, and eventual fsync. At thousands of pattern updates per second in a generative loop, the overhead compounds into a measurable bottleneck — not because SQLite is slow, but because it's doing work (query planning, row serialization, B-tree traversal) that is unnecessary when you already know the exact byte offset of the field you're updating.

The same argument applies to confidence decay (`decay_rate` applied per time unit), trust zone transitions, and sidecar mapping updates — all of which are in-place writes to specific typed fields in the node's payload union.

**The payload union is the key differentiator.** Synrix nodes carry domain-typed fields — `success_rate`, `performance_gain`, `evolution_generation`, `trust_zone`, `decay_rate` — directly in the storage format. A generic database stores these as JSON in a text column, requiring parse-update-serialize on every write. Synrix writes directly to the field. For a system making high-frequency updates to learned state, that difference is the design constraint that drove the custom storage engine.

For workloads that don't need this — simple key-value storage, config, session memory — SQLite is the right tool. Synrix is purpose-built for systems that treat stored nodes as **live, updateable weighted state**, not static records.

---

## Further Reading

- [Benchmarks](BENCHMARKS.md)
- [.lattice Format Spec](LATTICE_FORMAT_SPEC.md)
