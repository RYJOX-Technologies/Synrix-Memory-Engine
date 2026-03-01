# Synrix: ACID Database for AI Agents

**Microsecond queries. Deterministic learning. Crash-safe.**

[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

## Quick Start (60 seconds)

```bash
git clone https://github.com/RYJOX-Technologies/Synrix-Memory-Engine
cd Synrix-Memory-Engine

# Build tools (crash_test, latency diagnostic)
make build

# See crash recovery in action (THE PROOF)
./tools/crash_recovery_demo.sh

# Measure latency on your hardware
./tools/run_query_latency_diagnostic.sh

# Or: build + run core tests in one step
make test-core
```

## Proof

- **Crash recovery**: Write 500 nodes, crash mid-write (SIGKILL), recover perfectly. Zero data loss.
- **Latency**: Single-digit microsecond queries (192 ns hot, 3.2 μs warm)
- **Learning**: Mahalanobis from PMU feedback

## What Is Synrix?

Synrix is an **ACID-compliant database** for AI agents. It stores knowledge as a **binary lattice** — dense, cache-aligned nodes in memory-mapped files. No embeddings. No vector search. Just prefix-semantic retrieval that scales O(k) with matches, not O(N) with corpus size.

| | Synrix | Mem0 | Qdrant | ChromaDB |
|---|---|---|---|---|
| **Read latency** | 192 ns (hot) / 3.2 μs (warm) | 1.4s p95 | 4 ms p50 | 12 ms p50 |
| **Embedding model** | No | Yes | Yes | Yes |
| **ACID + crash proof** | Yes (Jepsen-style) | No | Partial | No |
| **Runs offline/edge** | Yes | No | Partial | Yes |

## Install (Python SDK)

```bash
pip install synrix
```

```python
from synrix.raw_backend import RawSynrixBackend

db = RawSynrixBackend("my_memory.lattice")
db.add_node("LEARNING_PYTHON_ASYNCIO", "asyncio uses event loops")
results = db.find_by_prefix("LEARNING_PYTHON_", limit=10)
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — How it works
- [Benchmarks](docs/BENCHMARKS.md) — Real numbers
- [ACID Guarantees](docs/ACID.md) — What we prove
- [API](docs/API.md) — How to use it
- [Quick Start](docs/QUICKSTART.md) — 5-minute walkthrough

## Example Outputs

See what the tools produce before running them:

- [Crash recovery proof](examples/crash_recovery_output.txt)
- [Latency diagnostic](examples/latency_diagnostic_output.txt)
- [WAL test results](examples/wal_test_output.txt)

## Demos

```bash
# Local RAG without embeddings
python3 python-sdk/examples/hello_memory.py

# Multi-session agent memory
python3 python-sdk/examples/ai_agent_synrix_demo.py

# O(k) scaling proof
python3 python-sdk/examples/test_scale_nodes.py
```

## Qdrant-Compatible API

Drop-in replacement. Same REST API, same Python client:

```python
from qdrant_client import QdrantClient
client = QdrantClient(url="http://localhost:6334")  # Synrix server
```

## Build From Source

```bash
# Build crash_test and tools
make build

# Run core tests (WAL, crash recovery)
make test-core
```

## Platform Support

| Platform | Status |
|----------|--------|
| Linux x86_64 | Ready |
| Linux ARM64 (Jetson, Pi) | Ready |
| Windows x86_64 | Ready |

## License

- **Python SDK**: MIT
- **Evaluation engine**: Free (25K node limit)
- **Production engine**: Commercial license

---

**Synrix** — ACID database for intelligent agents. Proven durable. Tested under crashes. Ready for production.
