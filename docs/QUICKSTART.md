# Synrix Quick Start

**5 minutes from clone to proof.**

## 1. Clone

```bash
git clone https://github.com/RYJOX-Technologies/Synrix-Memory-Engine
cd Synrix-Memory-Engine
```

## 2. See Crash Recovery (30 seconds)

```bash
make build
./tools/crash_recovery_demo.sh
```

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

```bash
make test-core
```

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `crash_test: command not found` | Run `make build` first |
| `Global usage limit reached` | Clear `~/.synrix/license_usage/` (free tier 25K nodes) |
| Build fails | Ensure gcc, std=c11. See [Build](../README.md#build-from-source) |

## Next Steps

- [Architecture](ARCHITECTURE.md) — How it works
- [Benchmarks](BENCHMARKS.md) — Real numbers
- [ACID Guarantees](ACID.md) — What we prove
