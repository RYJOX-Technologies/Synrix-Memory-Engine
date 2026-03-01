# Synrix Quick Start

**5 minutes from clone to proof.**

## 1. Get the release

Download the latest [release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) for your platform and extract it (or clone the repo).

## 2. See Crash Recovery (30 seconds)

```bash
./tools/crash_recovery_demo.sh
```

*(Tools are included in the release; on Windows use the provided scripts or run the demo from the extracted folder.)*

You should see:
```
[CRASH-TEST] 💥 CRASHING NOW after node 500...
...
[CRASH-TEST] ✅ ZERO DATA LOSS: All nodes recovered from WAL after crash
```

## 3. Measure Latency (1 minute)

```bash
./tools/run_query_latency_diagnostic.sh
```

Output shows min/max/avg latency for prefix search and O(1) lookup.

## 4. Use Python SDK (2 minutes)

```bash
pip install synrix
```

```python
from synrix.raw_backend import RawSynrixBackend

db = RawSynrixBackend("my_memory.lattice")
db.add_node("LEARNING_PYTHON_ASYNCIO", "asyncio uses event loops")
results = db.find_by_prefix("LEARNING_PYTHON_", limit=10)
print(results)
```

## 5. Run Tests

Use the test scripts included in the release, or see the main [README](../README.md) for SDK and platform details.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `crash_test: command not found` | Use the tools from the [release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) for your platform |
| `Global usage limit reached` | Clear `~/.synrix/license_usage/` (free tier 25K nodes) |
| Build from source | See platform-specific build docs (Windows: `build/windows/`; Linux: `build/linux/build.sh`) |

## Next Steps

- [Architecture](ARCHITECTURE.md) — How it works
- [Benchmarks](BENCHMARKS.md) — Real numbers
- [ACID Guarantees](ACID.md) — What we prove
