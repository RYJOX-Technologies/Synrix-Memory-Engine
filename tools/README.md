# Synrix Tools

Proof tools for crash recovery, latency, and ACID validation.

## Quick Start

Download the [latest release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases) and extract it. Then:

```bash
./tools/crash_recovery_demo.sh
./tools/run_query_latency_diagnostic.sh
```

## Tools

### crash_recovery_demo.sh (Most Important)

**The proof.** Write 500 nodes, crash mid-write (SIGKILL), recover perfectly.

```bash
./tools/crash_recovery_demo.sh
```

Output: `✅ ZERO DATA LOSS: All nodes recovered from WAL after crash`

### crash_test

Low-level crash injection. Run from repo root.

```bash
# Crash at node 500
./tools/crash_test 1 || true

# Verify recovery
./tools/crash_test 10
```

Modes: `1`=power loss, `2`=process kill, `3`=multiple crashes. `10`/`20`/`30`=verify.

### run_query_latency_diagnostic.sh

Per-query nanosecond latency. Min/max/avg, distribution.

```bash
./tools/run_query_latency_diagnostic.sh [lattice_path] [iterations]
# Default: /tmp/query_latency_diagnostic.lattice, 1000 iters
```

### run_extended_p99_benchmark.sh

Full P99 benchmark: O(1) lookup + O(k) prefix search.

```bash
./tools/run_extended_p99_benchmark.sh [iterations] [dataset_size]
# Default: 100k iters, 1M nodes
```

### wal_test.c

WAL test suite. Requires lattice engine sources; build via the platform build (see [Release process](docs/RELEASE.md)). Or run the prebuilt tools from the release.

## Build

The tools are included in the [release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases). To build from source you need the lattice engine sources and platform build (Windows: `build/windows/`; Linux: `build/linux/build.sh`).

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `crash_test: command not found` | Use the tools from the release for your platform |
| `Global usage limit reached` | Clear `~/.synrix/license_usage/` |
| `No prefix matches` | Lattice empty; diagnostic creates minimal one if path doesn't exist |
