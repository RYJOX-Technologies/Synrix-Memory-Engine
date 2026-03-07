# Synrix Memory Engine — Architecture Decomposition

---

## 1. System Overview

Synrix is a three-layer system: a closed-source C engine, a Python SDK, and an HTTP server that bridges them. The architecture can be summarized as:

```
┌─────────────────────────────────────────────────────────────────────┐
│                        USER APPLICATION                             │
│  (Python script, AI agent, LangChain pipeline, Cursor IDE, etc.)   │
└────────┬─────────────────────┬──────────────────────┬───────────────┘
         │                     │                      │
    ┌────▼─────┐        ┌──────▼──────┐       ┌───────▼────────┐
    │ SynrixClient│      │RawSynrixBackend│    │SynrixDirectClient│
    │ (HTTP/JSON)│       │ (ctypes/FFI)  │    │ (shared memory) │
    └────┬─────┘        └──────┬──────┘       └───────┬────────┘
         │                     │                      │
    ┌────▼─────────────────────▼──────────────────────▼───────┐
    │              synrix-server-evaluation (.exe)              │
    │                 (PROPRIETARY BINARY)                      │
    │  ┌─────────────┐  ┌────────────┐  ┌──────────────────┐  │
    │  │ HTTP Server  │  │ Shared Mem │  │ libsynrix C API  │  │
    │  │ (Qdrant API) │  │ (mmap IPC) │  │ (direct calls)   │  │
    │  └──────┬──────┘  └─────┬──────┘  └────────┬─────────┘  │
    │         └───────────────┼───────────────────┘            │
    │                    ┌────▼─────┐                           │
    │                    │ LATTICE  │                           │
    │                    │ ENGINE   │                           │
    │                    └────┬─────┘                           │
    │              ┌──────────┼──────────┐                     │
    │         ┌────▼───┐ ┌────▼───┐ ┌────▼──────┐             │
    │         │ Node   │ │ Prefix │ │ WAL       │             │
    │         │ Array  │ │ Index  │ │ Journal   │             │
    │         └────┬───┘ └────────┘ └────┬──────┘             │
    └──────────────┼──────────────────────┼────────────────────┘
              ┌────▼───┐            ┌─────▼─────┐
              │ .lattice│           │ .lattice.wal│
              │  (mmap) │           │  (append)  │
              └─────────┘           └────────────┘
                  DISK                   DISK
```

---

## 2. Layer-by-Layer Decomposition

### Layer 1: Storage Engine (C, Closed Source)

**What we can infer from the Python bindings, C test files, and header references:**

#### 2.1.1 The Node Structure

From `raw_backend.py` (ctypes struct definitions):

```c
struct lattice_node_t {
    uint64_t id;              // 8 bytes - unique node ID
    uint32_t type;            // 4 bytes - enum (PRIMITIVE=1, KERNEL=2, PATTERN=3, etc.)
    char     name[64];        // 64 bytes - prefix-searchable name field
    char     data[512];       // 512 bytes - payload (text or binary)
    uint64_t parent_id;       // 8 bytes - parent reference (tree structure)
    uint32_t child_count;     // 4 bytes
    uint64_t *children;       // 8 bytes (pointer) - dynamically allocated child array
    double   confidence;      // 8 bytes
    uint64_t timestamp;       // 8 bytes
    union    payload;         // ~288 bytes (largest member: lattice_learning_t)
};
// Total estimated: ~950-1000 bytes per node
```

**Analysis**: Each node is approximately 1KB. At 500K nodes, the lattice file is ~500MB. At 50M nodes (claimed max), it would be ~47GB. These are contiguous, fixed-size records.

#### 2.1.2 The "Binary Lattice" (Memory-Mapped Array)

The core data structure is:

```c
struct persistent_lattice_t {
    lattice_node_t *nodes;     // mmap'd array of fixed-size nodes
    uint32_t node_count;       // current count of nodes
    uint32_t total_nodes;      // total including deleted
    uint32_t max_nodes;        // capacity
    char     storage_path[];   // file path
    wal_t    *wal;             // write-ahead log handle
    bool     wal_enabled;      // WAL toggle
    // ... additional fields for prefix index, locks, etc.
};
```

**How it works:**
1. `lattice_init()` opens/creates a `.lattice` file and `mmap()`s it into virtual memory
2. Nodes are appended at `nodes[node_count++]`
3. The OS handles page faults — reads from cold data trigger disk I/O; hot data stays in page cache
4. `lattice_save()` calls `msync()` or writes the file explicitly

**This is functionally identical to:**
- `mmap()` + a flat array in any C program
- SQLite's memory-mapped mode
- LMDB's memory-mapped B-tree (but without the B-tree)
- Redis's RDB snapshots (but memory-mapped instead of forked)

#### 2.1.3 The Prefix Index

The prefix index enables `lattice_find_nodes_by_name(lattice, "PREFIX_", ids, max_ids)`.

**What we know from behavior:**
- It returns node IDs matching a string prefix of the `name` field
- It's described as O(k) where k = number of matches
- There's a `lattice_build_prefix_index()` function for explicit index construction

**Most likely implementation** (inferred):
- A sorted array of (name, node_id) pairs, searched via binary search to find the range of matching prefixes
- OR a trie/radix tree mapping prefixes to lists of node IDs
- OR a hash map from prefix buckets to node ID lists

**This is functionally identical to:**
- A B-tree index on a VARCHAR column with `LIKE 'prefix%'` queries
- Python's `[x for x in items if x.name.startswith(prefix)]` on a sorted list
- A trie data structure (textbook computer science, ~1960s)

#### 2.1.4 The Write-Ahead Log (WAL)

From `crash_test.c` and `wal_test.c`:

```c
lattice_enable_wal(&lattice);
lattice_add_node_with_wal(&lattice, type, name, data, parent_id);
lattice_wal_checkpoint(&lattice);
lattice_recover_from_wal(&lattice);
```

**How it works:**
1. On `add_node_with_wal()`: append the node data to `.lattice.wal` file, `fsync()`
2. On `checkpoint()`: apply WAL entries to the main `.lattice` file, truncate WAL
3. On `recover()`: read WAL entries from last checkpoint, replay them into the lattice

**This is the standard WAL pattern used by:**
- SQLite (WAL mode, default since 2010)
- PostgreSQL (WAL since inception)
- LevelDB/RocksDB (write-ahead log + compaction)
- Every serious database since the 1970s

#### 2.1.5 Concurrency Model

From the documentation: "seqlock gives lock-free reads, not full isolation levels"

**Seqlock**: A sequence-counter lock where:
- Writers increment a counter before and after modification
- Readers check the counter; if it changed during their read, they retry
- Provides lock-free reads at the cost of potential retries

This is a real concurrency primitive, appropriate for read-heavy workloads with rare writes.

---

### Layer 2: HTTP Server (Closed Source Binary)

The `synrix-server-evaluation` binary exposes:

```
HTTP Server (port 6334, Qdrant-compatible)
├── PUT  /collections/{name}              → Create collection
├── GET  /collections/{name}              → Get collection info
├── GET  /collections                     → List collections
├── DELETE /collections/{name}            → Delete collection
├── PUT  /collections/{name}/points       → Upsert points (vectors)
├── POST /collections/{name}/points/search → Search points (vectors)
├── GET  /collections/{name}/points/{id}  → Get point by ID
├── POST /synrix/nodes                    → Add node (native API)
└── POST /collections/{name}/query        → Prefix query (native API)

Shared Memory IPC (POSIX shm, /synrix_qdrant_shm)
├── ADD_NODE command
├── QUERY_PREFIX command
└── GET_NODE_BY_ID command
```

**The Qdrant compatibility layer** accepts vectors and stores them, but:
- The `search_points` endpoint's actual similarity implementation is unknown
- The Python `SynrixVectorStore` (LangChain adapter) does call `search_points`, but whether this does real cosine/euclidean distance or something else is unverifiable

**The shared memory IPC** uses a fixed-size shared memory segment (~70KB):
- Control structure (56 bytes): ready flags, request counter
- Command buffer (4184 bytes): command string + query payload
- Response buffer (65552 bytes): response JSON

This is a standard producer-consumer pattern over POSIX shared memory.

---

### Layer 3: Python SDK (Open Source, MIT)

The SDK is the only fully auditable layer. It contains:

```
python-sdk/synrix/
├── Core Access
│   ├── client.py              → HTTP client (requests library)
│   ├── raw_backend.py         → ctypes FFI to libsynrix.so/.dll
│   ├── direct_client.py       → Shared memory IPC client
│   └── mock.py                → In-memory mock for testing
│
├── Agent Memory Abstractions
│   ├── agent_memory.py        → Episodic memory (task attempts, failures)
│   ├── agent_backend.py       → Backend auto-detection (HTTP vs raw vs mock)
│   ├── agent_hooks.py         → Event hooks for agent lifecycle
│   ├── agent_integration.py   → Generic agent integration
│   ├── agent_auto_save.py     → Auto-save behavior
│   ├── agent_context_restore.py → Context restoration after restart
│   ├── agent_failure_tracker.py → Failure pattern tracking
│   ├── agent_memory_layer.py  → Higher-level memory layer
│   ├── ai_memory.py           → Simple add/query/count API
│   └── assistant_memory.py    → Assistant-specific memory
│
├── Integrations
│   ├── cursor_integration.py  → Cursor IDE integration
│   ├── devin_integration.py   → Devin AI integration
│   ├── langchain/             → LangChain VectorStore + Retriever
│   └── robotics.py            → RoboticsNexus (sensor/state/trajectory)
│
├── Organization & Formatting
│   ├── auto_organizer.py      → Keyword-based prefix classification
│   ├── auto_organizer_enhanced.py
│   ├── auto_organizer_dynamic.py
│   ├── hierarchical_organizer.py
│   ├── simplified_hierarchical_organizer.py
│   └── storage_formats.py     → JSON / Binary / Simple formatters
│
├── Infrastructure
│   ├── engine.py              → Engine download/install/management
│   ├── _download_binary.py    → Binary download from GitHub releases
│   ├── cli.py                 → CLI commands (install-engine, run)
│   ├── telemetry.py           → Opt-in usage telemetry
│   ├── feedback.py            → User feedback collection
│   ├── usage_report.py        → Usage reporting
│   ├── track_growth.py        → Growth tracking
│   └── exceptions.py          → Custom exception classes
│
└── Extras
    ├── auto_memory.py
    └── advanced_retrieval_tricks.py
```

**Key observation**: There are ~40 Python files wrapping a C library that exposes ~10 functions. The ratio of wrapper code to actual functionality is very high. Many of these modules (robotics, cursor_integration, devin_integration) are thin convenience wrappers that add naming conventions on top of the same `add_node()` / `find_by_prefix()` calls.

---

## 3. Data Flow Analysis

### Write Path

```
User calls client.add_node("LEARNING_X", "data")
    │
    ▼
SynrixClient._request("POST", "/synrix/nodes", {...})
    │
    ▼
HTTP Server receives request, parses JSON
    │
    ▼
lattice_add_node_with_wal(&lattice, type, name, data, parent_id)
    │
    ├──► WAL: append entry to .lattice.wal, fsync()
    │
    └──► Node Array: write struct to nodes[node_count], increment count
         │
         └──► Prefix Index: update index with new name→id mapping
    │
    ▼
Return node ID to client
```

**Latency breakdown (estimated):**
- HTTP overhead: ~100-500μs (TCP, JSON parse, response)
- WAL append + fsync: ~10-50μs (disk dependent)
- Node write: ~0.1-1μs (memory write)
- Prefix index update: ~0.1-5μs (data structure dependent)
- **Total write: ~100-600μs via HTTP, ~10-50μs via raw backend**

### Read Path (Prefix Query)

```
User calls client.query_prefix("LEARNING_")
    │
    ▼
SynrixClient._request("POST", "/collections/{col}/query", {...})
    │
    ▼
HTTP Server receives request, parses JSON
    │
    ▼
lattice_find_nodes_by_name(&lattice, "LEARNING_", ids, max_ids)
    │
    └──► Prefix Index: lookup "LEARNING_" → [id1, id2, ..., idk]
    │
    ▼
For each id in results:
    lattice_get_node_data(&lattice, id, &node)  ← O(1) array access
    │
    ▼
Serialize results to JSON, return via HTTP
```

**Latency breakdown (estimated):**
- HTTP overhead: ~100-500μs
- Prefix index lookup: ~0.1-5μs
- k × node reads: k × ~0.1-0.5μs
- JSON serialization: ~10-100μs
- **Total read: ~100-600μs via HTTP, ~0.5-10μs via raw backend**

The "192 ns" claim corresponds to a single `lattice_get_node_data()` call when the data is in L1/L2 cache — not the full query path.

### Read Path (Direct / Raw Backend)

```
User calls backend.find_by_prefix("LEARNING_", limit=10)
    │
    ▼
ctypes call: lib.lattice_find_nodes_by_name(ptr, "LEARNING_", ids, 10)
    │  ← ~0.5-5μs: prefix index lookup in C
    ▼
For each id: lib.lattice_get_node_copy(ptr, id)
    │  ← ~0.1-0.5μs per node: mmap read
    ▼
Copy C struct fields to Python dict
    │  ← ~5-50μs: Python object allocation overhead
    ▼
Return list of dicts
```

---

## 4. What's Missing (Architectural Gaps)

### No Graph Structure
Despite "knowledge graph" terminology:
- Nodes have `parent_id` and `children[]` fields, but these are unused in all SDK code
- No graph traversal functions exist in the Python SDK
- No edge types, no relationship queries, no path finding
- It's a flat record store with optional parent-child pointers

### No Actual Semantic Understanding
- The "semantic" in "semantic queries" means "prefix string matching"
- The `auto_organizer.py` does keyword-based classification (regex, word lists) — not NLP
- There's no embedding, no word2vec, no BERT, no semantic similarity

### No Distributed Operation
- Single-node, single-file architecture
- `device_id` parameter exists but no clustering or replication
- No sharding, no consensus, no multi-node anything

### No Real Vector Search
- The Qdrant-compatible API accepts vectors but the underlying storage is struct-based
- `SynrixVectorStore` (LangChain) calls `search_points` but the actual similarity computation in the engine is unverifiable
- No HNSW, no IVF, no product quantization — standard vector index algorithms

### No Query Language
- Only two operations: get by ID, search by prefix
- No filtering, no sorting, no aggregation, no joins
- No SQL, no GraphQL, no query planner

### No Schema Evolution
- Fixed 64-byte name, 512-byte data — hardcoded in struct
- No migration path if you need larger fields
- No versioning of node format

---

## 5. Component Interaction Matrix

| Component | Talks To | Protocol | Latency |
|-----------|----------|----------|---------|
| SynrixClient | HTTP Server | HTTP/JSON | ~100-500μs |
| RawSynrixBackend | libsynrix.so | ctypes/FFI | ~1-50μs |
| SynrixDirectClient | Shared Memory | mmap IPC | ~10-100μs |
| SynrixMockClient | In-memory dict | Python | ~1-10μs |
| HTTP Server | Lattice Engine | Direct C call | ~0.1-10μs |
| Lattice Engine | .lattice file | mmap | ~0.1μs (hot) to ~ms (cold) |
| Lattice Engine | .lattice.wal | append + fsync | ~10-50μs |
| Auto-Organizer | Nothing | Pure Python | ~0.01-1ms |
| Telemetry | Nothing (local) | JSON file | N/A |

---

## 6. File Format

### .lattice File
```
[Node 0 (~1KB)] [Node 1 (~1KB)] [Node 2 (~1KB)] ... [Node N (~1KB)]
```
- Contiguous array of fixed-size `lattice_node_t` structs
- Memory-mapped for direct access via pointer arithmetic: `nodes[i]` = offset `i * sizeof(lattice_node_t)`
- The prefix index is either embedded in the file or built in-memory on load

### .lattice.wal File
```
[WAL Header] [Entry 1] [Entry 2] ... [Entry M]
```
- Append-only log of node additions
- Each entry contains enough data to reconstruct the node
- Truncated on checkpoint

### Approximate Size Math
| Nodes | .lattice Size | RAM (mmap'd) |
|-------|--------------|--------------|
| 1K | ~1 MB | ~1 MB |
| 25K (free tier) | ~25 MB | ~25 MB |
| 500K (tested) | ~500 MB | ~500 MB |
| 50M (claimed) | ~47 GB | Paged by OS |
