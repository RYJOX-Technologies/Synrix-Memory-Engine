# Synrix Quick Start

**5 minutes from clone to proof.**

## Easiest path (Windows + Linux): pip + engine + Python

No download required. Works the same on Windows and Linux.

```bash
pip install synrix
```
(From the repo: `pip install -e python-sdk/` from the repo root.)
```bash
synrix install-engine
synrix run
```

Leave that terminal open. In a **second terminal**:

```bash
python -c "
from synrix import SynrixClient
c = SynrixClient(host='localhost', port=6334)
c.create_collection('demo')
c.add_node('HELLO_SYNRIX', 'Durable memory for AI agents', collection='demo')
print(c.query_prefix('HELLO_', collection='demo'))
"
```

You should see your node printed. Free evaluation (25K nodes), no signup.

---

## Linux: Crash recovery + latency (from release)

Download the latest [release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) for your platform and extract it.

### See Crash Recovery (30 seconds)

```bash
./tools/crash_recovery_demo.sh
```

You should see:
```
[CRASH-TEST] 💥 CRASHING NOW after node 500...
...
[CRASH-TEST] ✅ ZERO DATA LOSS: All nodes recovered from WAL after crash
```

### Measure Latency (1 minute)

```bash
./tools/run_query_latency_diagnostic.sh
```

Output shows min/max/avg latency for prefix search and O(1) lookup.

## Raw backend (direct lib)

If you have `libsynrix.so` (Linux) or the engine lib on your path:

```python
from synrix.raw_backend import RawSynrixBackend

db = RawSynrixBackend("my_memory.lattice")
db.add_node("LEARNING_PYTHON_ASYNCIO", "asyncio uses event loops")
results = db.find_by_prefix("LEARNING_PYTHON_", limit=10)
print(results)
```

## Run Tests

Use the test scripts included in the release, or see the main [README](../README.md) for SDK and platform details.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `Connection refused` when using SynrixClient | Start the engine first: `synrix run` (in another terminal) |
| `Engine not found` | Run `synrix install-engine` |
| `crash_test: command not found` | Use the tools from the [release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) for your platform (Linux/macOS) |
| `Global usage limit reached` | Clear `~/.synrix/license_usage/` (free tier 25K nodes) |
| Build from source | See platform-specific build docs (Windows: `build/windows/`; Linux: `build/linux/build.sh`) |

## Next Steps

- [Architecture](ARCHITECTURE.md) — How it works
- [Benchmarks](BENCHMARKS.md) — Real numbers
- [Durability & crash recovery](ACID.md) — What we prove
