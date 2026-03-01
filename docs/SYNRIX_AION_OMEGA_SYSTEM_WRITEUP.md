# Synrix & Aion Omega: System Overview

**High-level overview of the Synrix memory engine and Aion Omega stack.**  
**January 2025**

---

## Executive Summary

**Synrix** is a persistent Binary Lattice—a high-performance memory engine for AI systems. It provides O(k) semantic retrieval (query cost scales with result count, not corpus size), memory-mapped storage that exceeds RAM, and ACID durability. It runs locally, is Qdrant-compatible, and serves as the memory substrate for RAG, agents, and code intelligence.

**Aion Omega** is the broader applied-research system built on Synrix. It generates executable assembly through knowledge-graph–driven reasoning, empirical hardware discovery, and iterative refinement. It is **not** an LLM: it is a memory engine, reasoning system, and code-generation framework that discovers ISA capabilities from real compilation, tests hypotheses on real hardware, and learns from execution feedback.

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
│  Synrix Engine: HTTP server or libsynrix                         │
├─────────────────────────────────────────────────────────────────┤
│  Reasoning / synthesis layer                                     │
├─────────────────────────────────────────────────────────────────┤
│  Persistent Lattice (Binary Lattice storage + prefix index)      │
├─────────────────────────────────────────────────────────────────┤
│  Storage: .lattice files, WAL, memory-mapped I/O                │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 Core Principles

- **KG-driven architecture:** Knowledge graph as the source of truth for reasoning and synthesis.
- **Assembly-first:** Primary output is assembly; C is used for tooling and tests.
- **Tokenless system:** Semantic reasoning; no regex-based processing.
- **Stability → accuracy → speed:** Priority order for all operations.
- **Mechanical sympathy:** Rigid, cache-friendly layout; O(1) addressing; O(k) semantic queries.

---

## 2. Synrix: The Binary Lattice

### 2.1 What It Is

Synrix is a **Binary Lattice**—a dense, fixed-size array of nodes stored in memory-mapped files. It is not a traditional graph database: it avoids pointer chasing and variable-length storage in favor of rigid, predictable layout.

**Key properties:**
- **Fixed-size nodes**, cache-aligned for CPU and OS paging.
- **Storage:** Memory-mapped; lattice can exceed RAM (multiples of RAM supported).
- **Lookup:** O(1) by ID; O(k) semantic query via prefix index (k = matches, not total nodes).
- **Durability:** WAL, ACID, checkpointed operations.

### 2.2 Node Types (High Level)

Nodes are typed (e.g. PRIMITIVE, KERNEL, PATTERN, LEARNING, INTERFACE, COMPONENT, SYSTEM) for different roles in knowledge storage and code generation. The public API and SDK document the types available for use.

### 2.3 Semantic Prefix Indexing

Nodes use **semantic naming** (e.g. `ISA_ADD`, `LEARNING_PYTHON_ASYNCIO`). A prefix index enables O(k) retrieval: query cost depends on the number of matches, not corpus size. At large scale (e.g. 500k nodes), thousands of matches are returned in sub-millisecond time.

### 2.4 Performance (Validated)

- Sub-microsecond hot reads; single-digit microsecond warm reads.
- Durable writes in tens of microseconds.
- Sustained ingestion at hundreds of MB/s.
- Validated at 500k nodes with linear scaling; larger scales supported.

---

## 3. Aion Omega: Code Generation & Reasoning

Aion Omega is the applied-research layer on top of Synrix. It uses the lattice for knowledge-driven code generation, hardware discovery, and deterministic learning from execution. Implementation details and validation results are documented in internal technical whitepapers.

---

## 4. Integrations

### 4.1 Qdrant Mimic

Drop-in Qdrant replacement. Implements Qdrant REST API (collections, points upsert/search, health) using Synrix lattice under the hood. Use the Synrix server when you need Qdrant-compatible endpoints.

### 4.2 Python SDK

- **client.py** – HTTP client for Synrix server.
- **raw_backend.py** – Direct C library access (`libsynrix.so` / `libsynrix.dll`); sub-millisecond; supports evaluation and licensed tiers.
- **langchain/** – SynrixVectorStore, SynrixPrefixRetriever, SynrixLangGraphMemory.

### 4.3 Other Integrations

LangChain (drop-in Qdrant replacement), Redis shadow, and OpenAI-compatible API are available where documented.

---

## 5. License & Tiers

Synrix ships with an evaluation tier (node limit as stated in release notes) and commercial tiers. License is applied at engine init; key can be provided via environment, config file, or next to the binary. See the main README and release documentation for usage.

---

## 6. Build & Deployment

Prebuilt engine and tools are distributed via [GitHub Releases](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases). For build-from-source and internal deployment, see internal build and release documentation.

---

## 7. Documentation Reference

| Doc | Purpose |
|-----|---------|
| README.md | Main entry; quick start, platform support |
| docs/ARCHITECTURE.md | Binary Lattice design, high level |
| docs/BENCHMARKS.md | Performance numbers |
| docs/API.md | How to use the SDK and API |

---

## 8. Summary

**Synrix** = persistent Binary Lattice memory engine. O(k) semantic retrieval, memory-mapped storage, ACID durability, Qdrant-compatible. One binary per platform; tiered licensing.

**Aion Omega** = applied-research code-generation system on top of Synrix. KG-driven synthesis, hardware discovery, iterative refinement, deterministic learning. Not an LLM.

**Stack:** Lattice (storage) → reasoning/synthesis layer → assembly output. Integrations: Qdrant mimic (server), Python SDK (HTTP + raw), LangChain.

**Target:** Jetson Orin Nano (ARM64); validated at 500k nodes; edge-executable; local-first.
