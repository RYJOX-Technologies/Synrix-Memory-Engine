# Synrix

**AI memory that runs locally. No embeddings. No cloud. One binary.**

192 ns reads. O(k) retrieval. Qdrant-compatible. Runs on a $200 Jetson or a $5K server.

[![LangChain Compatible](https://img.shields.io/badge/langchain-compatible-brightgreen)](https://python.langchain.com/)
[![OpenAI API](https://img.shields.io/badge/api-openai--compatible-blue)](https://platform.openai.com/)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

**Single entry point:** The Python SDK and all demos live in **`python-sdk/`**. Use that directory for installation and examples.

## Install

```bash
pip install synrix
```

Or from source: `cd python-sdk && pip install -e .`

## First Query (3 lines)

```python
from synrix.raw_backend import RawSynrixBackend

db = RawSynrixBackend("my_memory.lattice")
db.add_node("LEARNING_PYTHON_ASYNCIO", "asyncio uses event loops for concurrent I/O")
results = db.find_by_prefix("LEARNING_PYTHON_", limit=10)
```

No server. No Docker. No API key. No embedding model.

## Learn More

Want to understand the "why" behind these numbers? Read our analysis:

**[Why Agent Memory Speed Matters: Sub-Millisecond Retrieval Enables Adaptive Reasoning](https://medium.com/@ryjoxtechnologies/why-agent-memory-speed-matters-sub-millisecond-retrieval-enables-adaptive-reasoning-40714bdbc48a)**

This explains how sub-millisecond memory enables adaptive reasoning in agents, validated on both desktop (Ryzen 7700X: 0.16ms) and edge hardware (Jetson Orin Nano: 0.60ms).

## Why Synrix

| | Synrix | Mem0 | Qdrant | ChromaDB |
|---|---|---|---|---|
| **Read latency** | 192 ns (hot) / 3.2 us (warm) | 1.4s p95 (end-to-end) | 4 ms p50 | 12 ms p50 |
| **Embedding model needed** | No | Yes | Yes | Yes |
| **Cost per query** | $0 | Embedding API + cloud | Embedding API | Embedding API |
| **Runs offline / edge** | Yes (one .so file) | No (cloud-first) | Partial (Docker) | Yes (local) |
| **Retrieval model** | Prefix-semantic O(k) | Vector similarity | HNSW ANN | HNSW ANN |
| **Qdrant API compatible** | Yes | No | Native | No |

**Caveats:** Mem0/Qdrant latency includes embedding + retrieval (different workload). Synrix uses prefix lookup, not fuzzy similarity — different retrieval semantics.

## Demos

All demos are in **`python-sdk/examples/`**. Run after `pip install -e python-sdk/` with `SYNRIX_LIB_PATH` (or `LD_LIBRARY_PATH` on Linux) set to the directory containing the engine DLL/.so. No server required (raw backend).

```bash
# Core demos
python python-sdk/examples/hello_memory.py
python python-sdk/examples/ai_agent_synrix_demo.py
python python-sdk/examples/test_scale_nodes.py
python python-sdk/examples/benchmark_synrix.py
python python-sdk/examples/reasoning_chain_benchmark.py

# Robotics (synrix.robotics)
python python-sdk/examples/robotics_quick_demo.py
```

RAG examples (`rag_demo.py`, `rag_simple_demo.py`, `rag_demo_kb.py`) require the **synrix_rag** package (e.g. from synrix-rag-sdk).

## How It Works

```
Your App → Python SDK → libsynrix.so → Memory-Mapped Lattice File
```

Synrix stores knowledge as a **Binary Lattice** — a dense array of fixed-size nodes (1216 bytes, cache-aligned) in memory-mapped files. Nodes use **semantic prefix naming** (`ISA_ADD`, `LEARNING_PYTHON_ASYNCIO`, `DOC_K8S_PODS_1`) instead of vector embeddings.

Retrieval uses a **dynamic prefix index**: query cost scales with the number of matches (k), not the total corpus size (N). At 500K nodes, 1000 matches take ~0.022 ms.

No pointer chasing. No embedding model. No ANN index. Just arithmetic addressing and prefix lookup.


## Benchmarks

```bash
# Quick (1K nodes, Synrix + ChromaDB)
python3 python-sdk/examples/benchmark_synrix.py

# Full (100K nodes, all backends)
python3 python-sdk/examples/benchmark_synrix.py --full
```

Run it yourself. All numbers are reproducible. Results written to `python-sdk/benchmark_results/latest.json`.

## Already Using Qdrant?

Synrix is a drop-in replacement. Same REST API, same Python client:

```python
from qdrant_client import QdrantClient

# Before: client = QdrantClient(url="http://qdrant-server:6333")
# After:
client = QdrantClient(url="http://localhost:6334")  # Synrix server
```

Start the Qdrant-compatible server:

```bash
# Evaluation (free, 25K nodes)
./synrix-server-evaluation --port 6334

# Production (requires license)
./synrix-server --port 6334 --production
```

API compatibility: collections, points upsert, search, health. Covers most RAG and agent workloads.

## Use Cases

- **RAG** — Store documents with semantic prefixes, retrieve by topic without embeddings
- **Agent memory** — Cross-session recall of user preferences, conversation history, learned patterns
- **Code intelligence** — Store code patterns, ISA definitions, build artifacts with semantic naming
- **Edge AI** — Full memory engine on Jetson, Raspberry Pi, or any ARM64/x86_64 device

## Platform Support

| Platform | Status |
|----------|--------|
| Linux x86_64 | Ready |
| Linux ARM64 (Jetson, Pi) | Ready |
| Windows x86_64 | Ready |
| macOS | In progress |

## Build From Source

```bash
# Linux — produces build/linux/out/libsynrix.so
./build/linux/build.sh

# Use with SDK
export LD_LIBRARY_PATH="$(pwd)/build/linux/out:$LD_LIBRARY_PATH"
pip install -e python-sdk/
```

## Performance (Validated on Jetson Orin Nano)

| Metric | Value |
|--------|-------|
| Hot-read latency | 192 ns |
| Warm-read average | 3.2 us |
| Durable write | ~28 us |
| Sustained ingestion | 512 MB/s |
| Max validated scale | 500K nodes (O(k) confirmed) |
| Max supported | 50M nodes (47.68 GB) |

## License

- **Python SDK**: MIT
- **Evaluation engine**: Free (25K node limit)
- **Production engine**: Commercial license (tiered: 25K → 1M → 10M → 50M → unlimited)

No signup required to try Synrix. The evaluation engine has the same API and performance characteristics as production.

## Links
- [GitHub Releases](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases)
- [Medium Article: Why Agent Memory Speed Matters](https://medium.com/@ryjoxtechnologies/why-agent-memory-speed-matters-sub-millisecond-retrieval-enables-adaptive-reasoning-40714bdbc48a) – Analysis of sub-millisecond retrieval and adaptive reasoning
---

**Synrix** — Fast, local AI memory. Your data, your machine, your rules.
