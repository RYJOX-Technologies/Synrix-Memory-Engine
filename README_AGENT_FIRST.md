# Synrix: State Infrastructure for Autonomous Agents

**Your agent runs at machine speed. Its memory should too.**

[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

---

## The problem

Every AI agent framework eventually hits the same wall: the agent's reasoning is fast, but its memory isn't. Vector databases require embedding models. SQL databases serialize every write through a query planner. Key-value stores give you no structure. In-memory dicts don't survive a crash.

None of these were designed for an agent that fires 10,000 state updates per second and cannot afford to wait.

Synrix was.

---

## What Synrix is

A binary lattice — a flat, memory-mapped array of fixed-size, cache-aligned nodes. Every node is reachable in O(1) by ID. Every prefix group is reachable in O(k) where k is the number of matching results, regardless of total dataset size. No query planner. No embedding model. No network round-trip in the direct access path.

The engine is written in C. It runs on the same machine as your agent. It uses `mmap` so reads hit RAM, not disk. Writes go through a WAL before they hit the file, so a `SIGKILL` mid-write loses nothing. The node layout is fixed — 1216 bytes, 64-byte cache-aligned — so the CPU prefetcher works with you, not against you.

This is not a general-purpose database. It is purpose-built state infrastructure for agents that need to read and write structured state at frequencies no human will ever reach.

---

## Design decisions and why

| Decision | Reason |
|----------|--------|
| Fixed 1216-byte nodes | Predictable layout → CPU prefetch, O(1) offset calculation, no fragmentation |
| Prefix-only queries | Agents design their own namespace at init time; O(k) with zero schema overhead |
| No deletion | Agents append state; a learning system that deletes its own memory is broken |
| No multi-op transactions | Single-operation atomicity is sufficient for append-only agent state |
| `parent_id` stored on disk | Agents build reasoning chains; the hierarchy persists across restarts |
| 512-byte data field | Agent state is structured and small: confidence scores, patterns, outcomes |
| WAL + fsync | Agent processes get killed; state must survive it |
| No embeddings | Agents know what they're looking for; semantic search is a human convenience |
| `mmap` + direct C access | The Python layer is optional; raw access is sub-microsecond |

---

## What an agent loop looks like with Synrix

```python
from synrix.raw_backend import RawSynrixBackend

# Initialise once at agent startup — O(n) index build, then O(1) incremental forever
db = RawSynrixBackend("~/.agent/state.lattice")

# Agent records an attempt — O(1) write
node_id = db.add_node(
    "TASK:stripe_api:attempt",
    '{"success": false, "error": "timeout", "retry": 3}',
    parent_id=reasoning_chain_root
)

# Agent queries its own history — O(k) where k = matching results, not dataset size
# At 100K nodes, this takes 0.07ms. At 10K nodes, same speed.
history = db.find_by_prefix("TASK:stripe_api:")

# Agent updates a confidence weight in-place — direct mmap write, no transaction overhead
node = db.get_node(pattern_node_id)
# payload fields (confidence, decay_rate, success_rate) are typed structs
# updated directly in mapped memory — no serialize/deserialize cycle
```

No HTTP. No JSON round-trip. No embedding model warming up. The agent writes, reads, and updates at the speed of memory.

---

## What agents store in Synrix

Synrix nodes are typed. The payload union carries domain-specific structs:

| Node type | What an agent stores |
|-----------|---------------------|
| `PATTERN_` | Learned behaviour patterns with `confidence` and `decay_rate` |
| `TASK_` | Task attempt records with outcome and parent reasoning chain |
| `FAILURE_` | Failure events with error type and retry count |
| `FUNC_` | Function call signatures and success rates |
| `ISA_` | Ontological relationships between concepts |
| `AGENT_id:` | Per-agent namespaced state (isolated by namespace prefix) |

The prefix scheme is not a convention. It is enforced by the engine. A node named `my_data` is rejected. A node named `TASK:stripe:attempt_3` is accepted. This is intentional — it prevents prefix explosion and guarantees O(k) query performance.

---

## Reasoning chains

The `parent_id` field lets an agent record multi-step reasoning as a tree stored in the flat lattice:

```
TASK:solve_problem (root, id=1001)
├── TASK:solve_problem:step_1 (parent_id=1001, id=1002)
│   └── FAILURE:solve_problem:step_1:attempt_1 (parent_id=1002, id=1003)
└── TASK:solve_problem:step_2 (parent_id=1001, id=1004)
    └── TASK:solve_problem:step_2:attempt_1 (parent_id=1004, id=1005)
```

The hierarchy is stored on disk and survives crashes. Every node is at a fixed offset — O(1) access by ID regardless of depth. This is the "compressed knowledge graph on a flat plane" design: you get the contextual richness of a graph with the access speed of an array.

The SDK exposes three traversal methods on `RawSynrixBackend`:

```python
# All direct children of a node — O(k) with prefix_hint
children = db.get_children(root_id, prefix_hint="TASK:solve:")

# Full subtree as a nested dict — BFS, depth-capped
tree = db.get_subtree(root_id, prefix_hint="TASK:solve:")

# Ancestor chain from a leaf up to the root — O(1) per hop
ancestors = db.get_ancestors(leaf_id)
```

`prefix_hint` is optional but recommended — it scopes the candidate scan to the relevant prefix group, keeping traversal O(k) on the subtree size rather than scanning the full dataset.

---

## Latency

These are not synthetic benchmarks. These are measured on real hardware with real data.

| Operation | Latency |
|-----------|---------|
| Node lookup by ID | 192 ns (hot cache) / 3.2 μs (warm) |
| Prefix query — 50K node dataset | 0.31 ms first query / 0.07 ms ongoing |
| Prefix query — 100K node dataset | 0.07 ms (same — O(k), not O(n)) |
| WAL write + fsync | ~1–5 ms (disk-dependent) |
| In-place payload field update | Sub-microsecond (direct mmap write) |

For comparison: Mem0 p95 read is 1.4 seconds. Qdrant p50 is 4 ms. Those systems require an embedding model and a network hop. Synrix requires neither. The comparison is not "which is better" — it is "which was designed for your workload."

---

## Crash safety

An agent process can be killed at any point. Synrix handles this:

1. Every write goes to the WAL first (fdatasync)
2. The WAL is replayed on next open if the process died mid-write
3. Checkpoints happen automatically every N writes
4. A corrupt partial write is detected by CRC during load and the WAL takes over

Tested with `SIGKILL` mid-write at 500 nodes. Zero data loss.

---

## Access paths

Three ways to access Synrix from Python, in order of speed:

| Path | How | Latency | Use when |
|------|-----|---------|----------|
| `RawSynrixBackend` | ctypes → C directly | Sub-microsecond | Agent on same machine, performance-critical |
| `SynrixDirectClient` | Shared memory | Microseconds | Agent subprocess, shared state |
| `SynrixClient` | HTTP to local server | ~1 ms | Multi-process, remote agent, or development |

The HTTP path exists for convenience and multi-agent setups. For a single agent loop, `RawSynrixBackend` is the right choice.

---

## Install

```bash
pip install synrix
synrix install-engine   # downloads engine binary for your platform
synrix run              # starts local server (only needed for SynrixClient path)
```

Free tier: 25,000 nodes, no key required. For unlimited: [request a license key](https://synrix.io).

---

## Platform support

| Platform | Status |
|----------|--------|
| Windows x86_64 | Ready |
| Linux ARM64 (Jetson, Pi, edge) | Ready |

Edge deployment is a first-class use case. An agent running on a Jetson at the edge needs durable local state that survives power loss. Synrix is 632 KB. It has no runtime dependencies. It runs without a network connection.

---

## What Synrix is not

- Not a vector database. No embeddings. Use Qdrant or ChromaDB if you need fuzzy similarity search.
- Not a general-purpose database. No joins, no arbitrary filters, no DELETE. Use SQLite if you need those.
- Not a human-facing tool. The prefix query model is designed for agents that know their own namespace. If you're building a user-facing search UI, this is the wrong tool.
- Not multi-op transactional. Single-operation atomicity only. If your workload requires "write A and B or neither," you need a different system.

---

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — Engine internals, why not SQLite
- [Benchmarks](docs/BENCHMARKS.md) — Real numbers, honest scale limits
- [Durability & crash recovery](docs/CRASH_TESTING.md) — WAL, SIGKILL testing
- [API](docs/API.md) — Full SDK reference
- [.lattice Format Spec](docs/LATTICE_FORMAT_SPEC.md) — File format, recovery script
- [Privacy & Data Collection](docs/PRIVACY_AND_DATA.md) — HWID, what's sent, how to verify

---

## License

- **Python SDK**: MIT
- **Engine binary**: Proprietary
  - Free tier: 25,000 nodes (no key required)
  - Unlimited: License key required

---

**Synrix** — State infrastructure for agents that operate at machine speed.
