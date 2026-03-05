# Synrix: Durable Database for AI Agents

**Microsecond queries. Deterministic learning. Crash-safe.**

[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

---

## Try Synrix in 5 minutes (Windows + Linux)

**No signup. No license key. Free evaluation engine.**

**1. Install the SDK and engine**
```bash
pip install synrix
```
(If you cloned the repo instead: `pip install -e python-sdk/` from the repo root.)
```bash
synrix install-engine
```

**2. Start the engine** (leave this terminal open)
```bash
synrix run
```

**3. In a second terminal**, run Python:
```bash
python -c "
from synrix import SynrixClient
c = SynrixClient(host='localhost', port=6334)
c.create_collection('demo')
c.add_node('HELLO_SYNRIX', 'Durable memory for AI agents', collection='demo')
print(c.query_prefix('HELLO_', collection='demo'))
"
```

You should see your node returned. That’s it — you’re using Synrix.  
Evaluation mode: free, local-only, 25K node limit. For more, see [License](#license) below.

If `synrix install-engine` fails, download the engine from the [latest release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) and place the `.exe` in `~/.synrix/bin/`. On Windows the default download expects the [Synrix-Memory-Engine](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases/tag/Synrix-Memory-Engine) release asset **`synrix-windows-release.zip`** (the .exe inside is extracted automatically).

---

## Quick Start (Linux: crash recovery + latency)

```bash
# Download latest release from [Releases](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases)
# Extract, then cd into the extracted folder:

# See crash recovery in action (THE PROOF)
./tools/crash_recovery_demo.sh

# Measure latency on your hardware
./tools/run_query_latency_diagnostic.sh
```

## Proof

- **Crash recovery**: Write 500 nodes, crash mid-write (SIGKILL), recover perfectly. Zero data loss.
- **Latency**: Single-digit microsecond queries (192 ns hot, 3.2 μs warm)
- **Learning**: Deterministic learning from execution feedback

## What Is Synrix?

Synrix is a **durable, crash-safe database** for AI agents. It stores knowledge as a **binary lattice** — dense, cache-aligned nodes in memory-mapped files. No embeddings. No vector search. Just prefix-semantic retrieval that scales O(k) with matches, not O(N) with corpus size. Single-operation atomicity, WAL + fsync; no multi-op transactions.

| | Synrix | Mem0 | Qdrant | ChromaDB |
|---|---|---|---|---|
| **Read latency** | 192 ns (hot) / 3.2 μs (warm) | 1.4s p95 | 4 ms p50 | 12 ms p50 |
| **Embedding model** | No | Yes | Yes | Yes |
| **Durable + crash proof** | Yes (Jepsen-style) | No | Partial | No |
| **Runs offline/edge** | Yes | No | Partial | Yes |

## Install (Python SDK)

**Recommended:** Use the [Try Synrix in 5 minutes](#try-synrix-in-5-minutes-windows--linux) flow above (`pip install synrix` → `synrix install-engine` → `synrix run`).

Or download the release for your platform (includes engine binary and Python SDK). Then use the server with `SynrixClient` or the raw backend with `RawSynrixBackend` if you have the engine lib on your path.

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — How it works
- [Benchmarks](docs/BENCHMARKS.md) — Real numbers
- [Durability & crash recovery](docs/ACID.md) — What we prove
- [API](docs/API.md) — How to use it
- [Quick Start](docs/QUICKSTART.md) — 5-minute walkthrough

## Example Outputs

See what the tools produce before running them:

- [Verified crash recovery](examples/verified_crash_recovery_output.txt) — Full run from Jetson
- [Crash recovery (condensed)](examples/crash_recovery_output.txt)
- [Latency diagnostic](examples/latency_diagnostic_output.txt)
- [WAL test results](examples/wal_test_output.txt)

## Examples (Python)

With the engine running (`synrix run` in one terminal) or the engine lib on your path (Linux: `libsynrix.so`; Windows: see [python-sdk README](python-sdk/README.md)):

```bash
# Basic memory operations
python python-sdk/examples/hello_memory.py

# Multi-session agent memory
python python-sdk/examples/ai_agent_synrix_demo.py

# O(k) scaling proof
python python-sdk/examples/test_scale_nodes.py
```
(Use `python3` on Linux if your system uses that.)

## Platform Support

| Platform | Status (this repo) |
|----------|--------------------|
| Windows x86_64 | Ready — built and tested (MSYS2; see [build/windows/WINDOWS_BUILD_GUIDE.md](build/windows/WINDOWS_BUILD_GUIDE.md)) |
| Linux ARM64 (Jetson, Pi) | Ready — ARM64 build available |

## License

- **Python SDK**: MIT (fully open)
- **Engine (native binary)**: Proprietary
  - Free evaluation version (25K node limit)
  - Production licensing TBD

---

**Synrix** — Durable database for intelligent agents. Proven crash recovery. Tested under crashes. Ready for production.
