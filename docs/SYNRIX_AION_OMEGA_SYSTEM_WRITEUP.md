# Synrix & Aion Omega: Full System Write-Up

**Deep dive into the NebulOS-Scaffolding / Aion Omega codebase**  
**January 2025**

---

## Executive Summary

**Synrix** is a persistent Binary Lattice—a high-performance memory engine for AI systems. It provides O(k) semantic retrieval (query cost scales with result count, not corpus size), memory-mapped storage that exceeds RAM, and ACID durability. It runs locally, is Qdrant-compatible, and serves as the memory substrate for RAG, agents, and code intelligence.

**Aion Omega** is the broader applied-research system built on Synrix. It generates executable assembly code through knowledge-graph–driven reasoning, empirical hardware discovery, and iterative refinement. It is **not** an LLM: it is a memory engine, reasoning system, and code-generation framework that discovers ISA capabilities from real compilation, tests hypotheses on real hardware, and learns deterministically from execution feedback.

Together they form a local-first, edge-executable AI memory and code-generation stack optimized for ARM64 (Jetson Orin Nano) and validated at 500k+ nodes.

---

## 1. Architecture Overview

### 1.1 Layered Model

```
┌─────────────────────────────────────────────────────────────────┐
│  Applications (RAG, agents, code gen, robotics)                 │
├─────────────────────────────────────────────────────────────────┤
│  Integrations: Qdrant mimic, LangChain, Redis shadow, OpenAI API │
├─────────────────────────────────────────────────────────────────┤
│  Python SDK: client (HTTP), raw_backend (direct .so/.dll)       │
├─────────────────────────────────────────────────────────────────┤
│  Synrix Engine: HTTP server (qdrant_http_server) or libsynrix.so │
├─────────────────────────────────────────────────────────────────┤
│  KG-Driven Synthesizer (thin layer; assembly output)              │
├─────────────────────────────────────────────────────────────────┤
│  Persistent Lattice (Binary Lattice storage + prefix index)      │
├─────────────────────────────────────────────────────────────────┤
│  Storage: .lattice files, WAL, memory-mapped I/O                  │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 Core Principles (from project rules)

- **KG-driven architecture:** Knowledge graph is the single source of truth; synthesizer is a thin layer that uses KG intelligence.
- **Assembly-first:** Primary output is assembly; no C in the synthesis/run hot path. C wrapper is test infrastructure only.
- **Tokenless system:** No regex-based processing; semantic reasoning only.
- **Stability → accuracy → speed:** Priority order for all operations.
- **Mechanical sympathy:** Rigid structure aligned with CPU cache lines and OS pages; O(1) arithmetic addressing; O(k) semantic queries.

---

## 2. Synrix: The Binary Lattice

### 2.1 What It Is

Synrix is a **Binary Lattice**—a dense, fixed-size array of nodes stored in memory-mapped files. It is not a traditional graph database: it avoids pointer chasing and variable-length storage in favor of rigid, predictable layout.

**Key properties:**
- **Node size:** 1216 bytes, 64-byte cache-aligned.
- **Storage:** Memory-mapped; lattice can exceed RAM by 20–30×.
- **Lookup:** O(1) via direct offset; O(k) semantic query via prefix index (k = matches, not total nodes).
- **Durability:** WAL, ACID, checkpointed operations.

### 2.2 Node Types

| Type | Enum | Purpose |
|------|------|---------|
| PRIMITIVE | 1 | Atomic operations |
| KERNEL | 2 | GPU kernels |
| PATTERN | 3 | Code patterns |
| PERFORMANCE | 4 | Performance metrics |
| LEARNING | 5 | Learning outcomes (contains `pattern_sequence`—assembly) |
| ANTI_PATTERN | 6 | Failures/constraints to avoid |
| SIDECAR_* | 7–10 | Sidecar system nodes |
| INTERFACE | 11 | Tier-3 machine-verifiable contracts |
| OBJECTIVE | 12 | Vector metrics + policy gates |
| COMPONENT | 13 | Composition of LEARNING + interfaces |
| SYSTEM | 14 | Tier-4 composition |

### 2.3 Semantic Prefix Indexing

Nodes use **autonomous semantic naming** (e.g. `ISA_ADD`, `LEARNING_ADD_PATTERN`, `PATTERN_MUL`). The dynamic prefix index enables O(k) retrieval: query cost depends on the number of matches, not corpus size. At 500k nodes, 1000 matches take ~0.022 ms.

**Valid prefixes:** `ISA_`, `PATTERN_`, `FUNC_`, `LEARNING_`, `TASK_`, `QDRANT_COLLECTION:`, `DOMAIN_`, `INTERFACE_`, `OBJECTIVE_`, `COMPONENT_`, `SYSTEM_`, etc. Garbage prefixes (`TEMP_`, `DEBUG_`, etc.) are rejected.

### 2.4 Performance (Jetson Orin Nano, validated)

- **192 ns** minimum hot-read latency
- **~3.2 μs** warm-read average
- **~28 μs** durable writes
- **512 MB/s** sustained ingestion
- **50M nodes** (47.68 GB) supported; 500k nodes validated with linear scaling

---

## 3. Aion Omega: Code Generation & Reasoning

### 3.1 What It Is

Aion Omega is an applied-research system that generates executable assembly through:

1. **Empirical discovery:** ISA capabilities from real compilation (133 instructions discovered).
2. **Hypothesis testing:** Generate → execute on real hardware → measure (ARM PMU).
3. **Iterative refinement:** 5–10 validated iterations in the time an LLM generates once.
4. **Deterministic learning:** Anti-patterns stored with reasons; operand constraints learned from execution.

**Critical clarification:** Aion Omega is **not** an LLM. It cannot generate natural language or hold conversations. It generates assembly through semantic reasoning over the lattice.

### 3.2 KG-Driven Synthesizer

The synthesizer (`src/kg_driven_synthesizer.c`) is a **thin layer**:

1. Parse request semantically.
2. Query lattice for relevant concepts (LEARNING, PATTERN, PRIMITIVE).
3. Use concept metadata (`pattern_sequence`, `generation_strategy`, `required_concepts`) to compose structure.
4. Render to **assembly** (primary output).

**No hardcoded logic:** No templates, no if/else for request types. All intelligence comes from KG query results. The `pattern_sequence` field in LEARNING nodes is the actual assembly.

### 3.3 Assembly-First Pipeline

- **Primary output:** Assembly (ARM64).
- **Execution:** In-process ARM64 encode + run (no gcc in hot path); fallback to pipe-to-gcc when needed.
- **Discovery:** Assembly instruction chains discovered from hardware; not C.
- **Learning:** Successful patterns and anti-patterns stored in lattice; cross-session persistence.

### 3.4 Validated Results (v7.0)

- 133 hardware instructions discovered autonomously
- 520+ operand patterns learned from execution
- Best kernel: 1.53× faster than hand-optimized baseline
- Best kernel IPC: 2.82 (112% better than baseline 1.33)
- ~75% success rate: generated kernels beat hand-optimized baseline
- 500k node validation confirms O(k) performance

---

## 4. Integrations

### 4.1 Qdrant Mimic (`integrations/qdrant_mimic/`)

**Purpose:** Drop-in Qdrant replacement. Implements Qdrant REST API (collections, points upsert/search, health) while using Synrix lattice + KG under the hood.

**Components:**
- `qdrant_http_server.c` – HTTP server (port 6334); evaluation vs production modes; license integration.
- `qdrant_mimic.c` – Core mimic logic; LSH index for vectors; lattice add/search.
- `synrix_shared_memory.c` – Zero-copy shared memory for low-latency access.

**Relationship:** Synrix is the product; qdrant_mimic is the integration that runs Synrix as a Qdrant-compatible server.

### 4.2 Python SDK (`python-sdk/synrix/`)

- **client.py** – HTTP client for Synrix server.
- **raw_backend.py** – Direct C library access (`libsynrix.so` / `libsynrix.dll`); sub-millisecond; applies license from env / `~/.synrix/license.json` when .so has symbols.
- **agent_backend.py** – Agent memory layer.
- **langchain/** – SynrixVectorStore, SynrixPrefixRetriever, SynrixLangGraphMemory.

### 4.3 Other Integrations

- **redis_shadow** – Transparent Redis proxy; silent graph building from Redis ops.
- **langchain** – Drop-in Qdrant replacement for LangChain.
- **openai_compatible** – OpenAI-compatible API (`/v1/embeddings`, `/v1/chat/completions`).

---

## 5. License & Tier System

### 5.1 Key Source Order (Linux)

1. Explicit key argument
2. `SYNRIX_LICENSE_KEY` env
3. `~/.synrix/license.json` (`{"license_b64":"<base64>"}`)
4. `license_key` file next to loaded .so (dladdr)

### 5.2 Tier Limits (single source: `synrix_license_linux.c`)

| Tier | node_limit |
|------|------------|
| 25k | 25,000 |
| 1m | 1,000,000 |
| 10m | 10,000,000 |
| 50m | 50,000,000 |
| unlimited | 0 |

### 5.3 Payload Format

- **V1:** 176 bytes (112 payload + 64 Ed25519). Magic `SYNRIXLI`, ver 1, tier 0..4, CRC32C, HWID = SHA256(machine-id).
- **Legacy:** 70/78/86-byte blobs with smaller payloads + 64-byte Ed25519.

### 5.4 Enforcement

- `lattice_apply_license()` sets `license_node_limit` on lattice.
- `license_global_add_one()` tracks usage per machine; cap 0 = unlimited.
- Server, SDK (when .so has symbols), and CLI all apply license at init/startup.

---

## 6. Build & Deployment

### 6.1 Linux

- **Engine:** `./build/linux/build.sh` → `build/linux/out/libsynrix.so` (lattice + exact_name_index + license_utils + synrix_license).
- **Release tarball:** `./build/linux/create-release-tarball.sh` → `/tmp/synrix-linux-{arch}.tar.gz` (lib + bundled .so deps).
- **Server:** Built from `integrations/qdrant_mimic`; produces `synrix-server` (production) and `synrix-server-evaluation` (25k free tier).

### 6.2 Windows

- **Engine:** `build/windows/` (CMake); produces `libsynrix.dll`.
- **SDK:** Same Python package; loads DLL via ctypes.

### 6.3 CLI

- **synrix_cli** (from `python-sdk`): `write`, `read`, `search`, `stats`, `init`, `daemon`, `license status` (Linux).
- Built with `make -f Makefile.synrix_cli`; links lattice + license on Linux.

---

## 7. Discovery & Lattice Growth

### 7.1 Paths

1. **Tier3 path:** `seed_tier3_contracts` → `validate_phase4` / `validate_phase5` (INTERFACE, OBJECTIVE, LEARNING, COMPONENT, SYSTEM).
2. **Code-gen path:** `seed_code_gen_lattice` → `code_gen_cli` (LEARNING patterns + synthesis from natural language).
3. **Hierarchical discovery:** CPU identity, features, ISA probing, microarch, structural topology, adaptive reasoning → ISA_*, LEARNING nodes.

### 7.2 Scripts

- `scripts/gen_license_and_test.py` – Generate v1 key, write `~/.synrix/license.json`, optional tests.
- `scripts/test_linux_release.py` – Pre-release: build, SDK import, add/find, usage file, FreeTierLimitError.
- `scripts/test_license_sdk_lib.py` – SDK + libsynrix.so only; add nodes until limit.
- `integrations/qdrant_mimic/test_license_tiers` – C engine only; no server.

---

## 8. Directory Structure (Key Paths)

```
NebulOS-Scaffolding/
├── src/
│   ├── storage/lattice/          # Persistent lattice, WAL, prefix index, license
│   ├── kg_driven_synthesizer.c/h  # Thin synthesizer; assembly output
│   ├── language/                  # Pattern consolidation
│   └── discovery/                 # Hardware discovery (if present)
├── python-sdk/
│   ├── synrix/                    # SDK package (client, raw_backend, langchain)
│   └── src/cli/                   # synrix_cli (write, read, license status, etc.)
├── integrations/
│   ├── qdrant_mimic/              # Qdrant-compatible server, code_gen_cli, tests
│   ├── redis_shadow/
│   └── langchain/
├── build/
│   ├── linux/                     # build.sh, create-release-tarball.sh
│   └── windows/
├── docs/                          # Technical docs, planning, lock, alignment
├── scripts/                       # Keygen, tests, discovery
└── releases/                      # Versioned artifacts
```

---

## 9. Documentation Reference

| Doc | Purpose |
|-----|---------|
| `README.md` | Main entry; quick start, platform support |
| `Synrix_Technical_Whitepaper_v1.9.md` | Binary Lattice architecture, performance |
| `AION_OMEGA_TECHNICAL_WHITEPAPER_v7.0.md` | Code gen, reasoning, validation |
| `docs/SYNRIX_LICENSE_LOCK.md` | Locked license/SDK surface |
| `docs/WINDOWS_LINUX_ALIGNMENT.md` | Cross-platform alignment |
| `docs/CHANGES_FOR_WINDOWS_DEVS.md` | Handoff for Windows team |
| `docs/planning/DISCOVERY_AND_GROW_LATTICE.md` | Discovery and growth paths |

---

## 10. Summary

**Synrix** = persistent Binary Lattice memory engine. O(k) semantic retrieval, memory-mapped storage, ACID durability, Qdrant-compatible. One binary per platform; tier by key.

**Aion Omega** = applied-research code-generation system on top of Synrix. KG-driven assembly synthesis, empirical hardware discovery, iterative refinement, deterministic learning. Not an LLM.

**Stack:** Lattice (storage) → KG synthesizer (thin layer) → assembly output → in-process run or gcc fallback. Integrations: Qdrant mimic (server), Python SDK (HTTP + raw), LangChain, Redis shadow.

**Target:** Jetson Orin Nano (ARM64); validated at 500k nodes; edge-executable; local-first.
