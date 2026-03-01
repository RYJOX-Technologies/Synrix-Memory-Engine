# Synrix Launch Plan — Technical Spec

**Objective:** Position Synrix as a credible alternative to Mem0/Qdrant for AI memory workloads.  
**Target:** r/LocalLLaMA, Hacker News, arXiv  
**Timeline:** 4 phases, ~6 weeks total  
**Handoff:** This document contains everything a developer needs to execute.

---

## Current State (What We Have)

| Asset | Status | Path |
|-------|--------|------|
| C engine (libsynrix.so) | Ships | `build/linux/build.sh` → `build/linux/out/libsynrix.so` |
| Python SDK | Ships (`pip install synrix`) | `python-sdk/` |
| Qdrant-compatible HTTP server | Ships | `integrations/qdrant_mimic/` |
| LangChain integration | Exists | `python-sdk/synrix/langchain/` |
| Existing benchmark | Exists but requires server + embeddings | `python-sdk/examples/benchmark_synrix.py` |
| License system | Ships (25k free / tiered / unlimited) | `src/storage/lattice/synrix_license*` |
| README | Exists, decent | `README.md` |
| Whitepaper | v1.9 | `Synrix_Technical_Whitepaper_v1.9.md` |

## What We Need To Build

| Deliverable | Priority | Phase |
|-------------|----------|-------|
| **Reproducible benchmark vs Mem0 + Qdrant** | P0 | 1 |
| **One-command install that works** | P0 | 1 |
| **3 hero demos** | P0 | 2 |
| **Launch README rewrite** | P1 | 2 |
| **Blog post / arXiv note** | P1 | 3 |
| **Launch posts (Reddit, HN)** | P1 | 4 |
| **Cost calculator** | P2 | 3 |

---

## Phase 1: Benchmark + Install (Week 1–2)

### 1A. Reproducible Benchmark Script

**File:** `benchmarks/bench_synrix_vs_mem0.py`

**What it measures:**

| Metric | How | Notes |
|--------|-----|-------|
| Write latency (p50, p95, p99) | Time `N` individual node adds | N = 1k, 10k, 100k |
| Read latency (p50, p95, p99) | Time `N` prefix lookups | Same N values |
| Bulk retrieval | Time to retrieve `k` matches at corpus size `N` | k = 10, 100, 1000; N = 10k, 100k, 500k |
| Throughput (ops/sec) | Sustained writes over 10s window | Report MB/s and ops/s |
| Memory footprint | RSS before/after adding N nodes | Use `psutil` |
| Cold start | Time from `import synrix` to first successful read | Measure wall clock |

**Comparison targets:**

1. **Synrix (raw_backend)** — Direct libsynrix.so via ctypes. No server, no embedding.
2. **Synrix (HTTP server)** — Via qdrant_mimic server on localhost:6334.
3. **Qdrant** — Via `qdrant-client` against a local Qdrant Docker container.
4. **Mem0** — Via `mem0` Python package in local/self-hosted mode.
5. **ChromaDB** — Via `chromadb` Python package (local persistent mode).

**Implementation requirements:**

```python
# Pseudocode structure

class BenchmarkRunner:
    """Runs identical workloads against multiple backends."""

    def __init__(self, backend: str, n_nodes: int):
        ...

    def bench_write(self) -> dict:
        """Add n_nodes. Return {p50, p95, p99, ops_per_sec, total_ms}."""

    def bench_read(self, k: int) -> dict:
        """Query for k matches. Return {p50, p95, p99, total_ms}."""

    def bench_bulk_retrieve(self, prefix: str) -> dict:
        """Retrieve all nodes matching prefix. Return {count, total_ms}."""

    def bench_memory(self) -> dict:
        """Return {rss_before_mb, rss_after_mb, delta_mb}."""

    def bench_cold_start(self) -> dict:
        """Time from import to first read. Return {ms}."""
```

**Output format:** JSON + markdown table, both written to `benchmarks/results/`.

**Acceptance criteria:**
- Runs with `python3 benchmarks/bench_synrix_vs_mem0.py` — no manual setup beyond `pip install` and Docker.
- Script prints a single markdown comparison table at the end.
- All timings use `time.perf_counter_ns()` (nanosecond precision).
- Includes a `--quick` flag (1k nodes, 3 backends) for fast smoke test.
- Includes `--full` flag (100k nodes, all 5 backends) for publication numbers.
- Each backend runs in isolation (separate process or clean state).
- Script documents hardware at top of output (CPU, RAM, OS, disk).

**Data payloads:**
- Synrix: Prefix-named nodes (`BENCH_CATEGORY_N` where CATEGORY is one of 20 semantic prefixes, N is sequential). 512-byte text payload per node.
- Vector DBs (Qdrant, Chroma, Mem0): Same text payloads, embedded with `all-MiniLM-L6-v2` (384-dim). Embedding time measured separately and reported.

**Critical: fair comparison notes (include in output):**
- Synrix does NOT compute embeddings; vector DBs do. Report embedding time separately.
- Synrix uses prefix lookup (exact semantic match); vector DBs use ANN similarity. Different retrieval semantics — note this.
- "End-to-end" column should include embedding for vector DBs.

---

### 1B. One-Command Install

**Goal:** `pip install synrix` on Linux x86_64 and aarch64 — and it works immediately, no server needed.

**Current gap:** `pip install synrix` installs the SDK but the user still needs `libsynrix.so` and must set `LD_LIBRARY_PATH`. That is a dealbreaker for first impressions.

**Tasks:**

1. **Build manylinux wheels with bundled .so**
   - File: `build/linux/build_wheel.sh`
   - Use `auditwheel` to bundle `libsynrix.so` + deps into the wheel.
   - Platform tags: `manylinux_2_17_x86_64`, `manylinux_2_17_aarch64`.
   - The wheel's `synrix/` package should contain `libsynrix.so` and `raw_backend.py` should look for it there first.

2. **Fallback: auto-download on import**
   - `python-sdk/synrix/_download_binary.py` already exists.
   - Ensure it downloads the correct platform binary from GitHub Releases on first import if the .so isn't bundled.
   - Print one clear line: `Synrix: downloading engine for linux-x86_64...` — not a wall of text.

3. **Verify: zero-config usage**
   - After `pip install synrix`, the following must work with no environment variables, no server, no Docker:

   ```python
   from synrix.raw_backend import RawSynrixBackend
   s = RawSynrixBackend()
   s.add_node("LEARNING_HELLO", "Hello World")
   result = s.find_nodes("LEARNING_")
   print(result)  # Should show the node
   ```

4. **Update `pyproject.toml`**
   - Bump version to `0.2.0`.
   - Update description: `"Local AI memory engine — 7,000x faster than cloud memory, zero API costs"`
   - Update keywords: add `"local-first"`, `"edge-ai"`, `"no-embeddings"`.
   - Remove `"embeddings"` from keywords (we don't use them in core).

**Acceptance criteria:**
- Fresh virtualenv: `pip install synrix && python3 -c "from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend(); b.add_node('TEST_X','hi'); print(b.find_nodes('TEST_'))"` prints a result.
- Works on Ubuntu 22.04 x86_64 and Jetson (aarch64) without gcc, without Docker.
- If .so is missing and download fails, error message says exactly what to do (one sentence, one URL).

---

## Phase 2: Hero Demos + README (Week 2–3)

### 2A. Three Hero Demo Scripts

Each demo must be runnable with `python3 demos/demo_X.py` after `pip install synrix`. No server. No API keys. No Docker. Under 100 lines each.

#### Demo 1: `demos/demo_local_rag.py` — Local RAG Without Embeddings

**What it does:**
1. Ingests 50 short documents (inline, no external files) into Synrix with prefix naming: `DOC_PYTHON_ASYNCIO_1`, `DOC_KUBERNETES_PODS_3`, etc.
2. User types a natural language query.
3. Script extracts keywords, maps to prefixes, retrieves matching nodes.
4. Prints retrieved context.

**Point proven:** RAG retrieval without an embedding model, without a vector DB, without an API key. Instant.

**Implementation notes:**
- Include a small keyword → prefix mapping (15–20 topics).
- Show latency of retrieval in the output.
- Print: `"Retrieved 5 documents in 0.018ms — no embeddings, no API calls."`

#### Demo 2: `demos/demo_agent_memory.py` — Multi-Session Agent Memory

**What it does:**
1. Simulates 3 "sessions" with a user.
2. Session 1: User says preferences (likes Python, works at Acme Corp, prefers dark mode).
3. Session 2: Agent recalls preferences from Synrix without being told again.
4. Session 3: User updates a preference; agent recalls updated version.

**Point proven:** Cross-session memory without Mem0, without embeddings, without cloud.

**Implementation notes:**
- Store as `USER_PREF_LANGUAGE`, `USER_PREF_COMPANY`, `USER_PREF_THEME`.
- Show that recall is instant (print latency).
- Print: `"Recalled 3 user preferences in 0.005ms — stored locally, no cloud sync."`

#### Demo 3: `demos/demo_edge_scale.py` — 100K Nodes on Edge Hardware

**What it does:**
1. Adds 100K nodes to Synrix with varied prefixes.
2. Queries at 1k, 10k, 50k, 100k to show O(k) scaling.
3. Prints a latency table showing query time scales with matches, not corpus.
4. Reports memory usage.

**Point proven:** Scales to 100K on any hardware; query cost is O(k).

**Implementation notes:**
- Use 20 semantic prefixes, ~5K nodes each.
- Print table:
  ```
  Corpus: 100,000 nodes
  Query "ISA_" → 5,000 matches:  0.11 ms
  Query "LEARN_" → 5,000 matches: 0.10 ms
  Corpus: 10,000 nodes (same query)
  Query "ISA_" → 500 matches:    0.01 ms
  → Query time scales with matches, not corpus size.
  ```
- Report RSS delta (expect ~120 MB for 100K nodes at 1216 bytes/node).

**Acceptance criteria (all 3 demos):**
- Each runs in < 30 seconds on a Raspberry Pi 4 / Jetson Nano.
- Each prints clear, quotable output suitable for a blog post.
- No external dependencies beyond `synrix`.
- Exit code 0 on success.

---

### 2B. README Rewrite

**File:** `README.md` (replace current)

**Structure (strict order):**

1. **One-liner** — `"AI memory that runs locally. 7,000x faster storage than Mem0. No embeddings. No cloud. One binary."`
2. **Install** — `pip install synrix` — three lines to first working query.
3. **Why Synrix** — Three bullets:
   - 192 ns reads (7,000× faster than Mem0's 1.4s end-to-end)
   - No embedding model needed — prefix-semantic retrieval
   - One .so file — runs on Jetson, Pi, laptop, server
4. **Quick comparison table** (5 rows: Synrix, Mem0, Qdrant, ChromaDB, pgvector — latency, cost, edge support, embedding required)
5. **Demos** — Links to the 3 hero demos with one-line descriptions.
6. **How it works** — 5-sentence explanation + ASCII diagram.
7. **Benchmarks** — Link to `benchmarks/` with summary table.
8. **Qdrant compatibility** — "Already using Qdrant? Synrix is a drop-in replacement." + 3-line migration.
9. **License** — SDK MIT; engine tiered (25k free).
10. **Links** — Docs, whitepaper, Discord/community (when ready).

**Rules:**
- No jargon in the first 3 sections. No "Binary Lattice", no "mechanical sympathy", no "O(k)" until section 6.
- Every claim has a number or a link to reproducible proof.
- README must fit on one screen (no scroll) for sections 1–4.

---

## Phase 3: Content (Week 3–5)

### 3A. Blog Post / arXiv Note

**Title:** `"Embedding-Free Memory for AI: O(k) Semantic Retrieval Without Vector Search"`

**Format:** 6–8 pages. arXiv cs.AI or cs.DB. Alternatively, a long-form blog post on the project site.

**Sections:**

1. **Abstract** — Prefix-semantic memory; O(k) retrieval; validated at 500K nodes; no embeddings.
2. **Introduction** — The embedding tax: cost, latency, model dependency. Why prefix semantics work for structured knowledge.
3. **Architecture** — Binary Lattice, prefix index, memory-mapped storage. Keep it concise; link to whitepaper for details.
4. **Evaluation**
   - Microbenchmarks: read/write latency vs Qdrant, ChromaDB (from Phase 1 benchmark).
   - Scaling: O(k) evidence at 10K, 100K, 500K nodes.
   - Memory footprint: Synrix vs Qdrant at 100K nodes.
   - Cold start: Synrix vs Qdrant vs ChromaDB.
   - *Not claimed:* accuracy on LOCOMO/DMR (different retrieval semantics; acknowledge this).
5. **Limitations** — No fuzzy similarity (prefix is exact); requires naming discipline; not a chat memory system by itself.
6. **Related work** — Mem0, MemGPT, Zep, MAGMA, AriGraph, Qdrant Edge. Cite properly.
7. **Conclusion** — Embedding-free memory is viable for structured AI knowledge. Open-source SDK, reproducible benchmarks.

**Acceptance criteria:**
- All numbers come from the Phase 1 benchmark script (reproducible).
- Limitations section is honest (not marketing).
- If arXiv: compile with LaTeX; if blog: markdown with embedded charts.

### 3B. Cost Calculator

**File:** `tools/cost_calculator.py` (also a static page if we have a site)

**Inputs:** queries per day, average payload size, embedding model (e.g., OpenAI ada-002 at $0.0001/1K tokens).

**Outputs:**

| | Mem0 (cloud) | Qdrant + embeddings | Synrix |
|---|---|---|---|
| Embedding cost/month | $X | $X | $0 |
| Infra cost/month | $Y (Mem0 pricing) | $Y (server) | $0 (local) |
| Total/month | $Z | $Z | **$0** |
| Total/year | ... | ... | **$0** |

**Notes:**
- Use real Mem0 pricing (from their site) and real OpenAI embedding pricing.
- Include a "self-hosted" row for Mem0/Qdrant that shows just the embedding cost.
- Be fair: Synrix doesn't do similarity search; if the user needs similarity, they still need an embedding model on top.

---

## Phase 4: Launch (Week 5–6)

### 4A. r/LocalLLaMA Post

**Subreddit:** r/LocalLLaMA (~850K members, obsessed with local-first AI)

**Title options (pick the one with strongest hook):**
- `"I built an AI memory engine that's 7,000x faster than Mem0 and runs on a Jetson. No embeddings. No cloud. Open source SDK."`
- `"Why does AI memory need embeddings? I built a memory engine without them — 192ns reads, runs offline, one binary."`

**Post structure:**
1. Problem statement (2 sentences)
2. What Synrix is (3 sentences — no jargon)
3. Benchmark table (from Phase 1)
4. One demo GIF or terminal recording (from Phase 2)
5. Link to GitHub
6. Honest "what it doesn't do" (fuzzy similarity, chat memory)
7. "AMA in comments"

**Timing:** Post Tuesday or Wednesday, 9–11 AM EST (peak r/LocalLLaMA activity).

### 4B. Hacker News Post

**Title:** `"Show HN: Synrix – AI memory engine, 192ns reads, no embeddings, one .so file"`

**Rules:**
- HN title must be factual, no hype.
- Post body: link to GitHub. Let the README speak.
- Be in comments within 5 minutes to answer questions.
- Prepare answers for: "How is this different from SQLite?", "What about fuzzy search?", "Benchmarks are unfair (no embeddings)."

### 4C. Prepared FAQ for Comments

Draft answers for predictable pushback:

| Question | Answer |
|----------|--------|
| "You're comparing apples to oranges — Mem0 includes LLM calls" | "Correct. We report storage latency separately and end-to-end separately. Even factoring in embedding overhead, Synrix prefix retrieval completes before the embedding call starts." |
| "No embeddings means no fuzzy search" | "Right. Synrix does exact prefix-semantic retrieval. If you need fuzzy similarity, add an embedding layer on top — Synrix still handles the storage faster. Or use the Qdrant-compatible API with LSH vectors." |
| "Why not just use SQLite with LIKE queries?" | "SQLite LIKE is O(N) full scan. Synrix prefix index is O(k) where k = matches. At 500K nodes, that's the difference between 5ms and 0.02ms." |
| "This won't work for chat agents" | "It's not designed to replace Mem0 for chat. It's designed for structured knowledge, RAG, and code intelligence where naming is systematic. Different tool for a different job — but in that job, it's orders of magnitude faster." |
| "Where's the accuracy benchmark?" | "We haven't run LOCOMO or DMR because those test LLM recall, not storage retrieval. We benchmark what we actually are: a storage engine. Accuracy depends on the retrieval strategy and LLM on top." |

---

## Repo Hygiene Checklist (Before Any Public Launch)

Must be done before any post goes live:

- [ ] `README.md` rewritten per 2B spec
- [ ] `pip install synrix` works on clean Ubuntu 22.04 x86_64 (verified in Docker)
- [ ] `pip install synrix` works on clean aarch64 (verified on Jetson or Pi)
- [ ] All 3 demos run without errors (`demos/demo_local_rag.py`, `demo_agent_memory.py`, `demo_edge_scale.py`)
- [ ] Benchmark script runs with `--quick` in < 2 minutes
- [ ] GitHub repo has: LICENSE (MIT for SDK), CONTRIBUTING.md, .gitignore
- [ ] No debug prints (`printf("DEBUG:...`)` in production code
- [ ] No hardcoded local paths (`/mnt/nvme/aion-omega/...`) in any shipped file
- [ ] No private keys, PEM files, or credentials in repo
- [ ] GitHub Actions CI: build .so + run demos + run `--quick` benchmark (Linux x86_64)
- [ ] Releases page has at least one tagged release with pre-built .so for x86_64 and aarch64

---

## Success Metrics

| Metric | Target (30 days post-launch) |
|--------|------------------------------|
| GitHub stars | 500+ |
| r/LocalLLaMA post upvotes | 200+ |
| HN front page | Yes (top 30) |
| pip install downloads | 1,000+ |
| Issues/PRs from external contributors | 5+ |
| Blog/tweet mentions | 10+ |

These are stretch targets. The real goal is **credibility**: developers who see the project believe the numbers and try it.

---

## Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| Benchmark numbers challenged as unfair | High | Acknowledge embedding difference in every comparison; include "end-to-end" column |
| `pip install` fails on someone's machine | High | Test in clean Docker images (Ubuntu 20.04, 22.04, 24.04; Fedora; Arch); provide fallback instructions |
| "But I need fuzzy search" response | Medium | Show how to add embedding layer on top; point to Qdrant-compatible API |
| Mem0 team responds / dismisses | Low | Stay technical; don't engage in drama; let benchmarks speak |
| Nobody cares | Medium | Post timing matters; have 3 friends upvote early; cross-post to r/MachineLearning, r/SelfHosted |

---

## Assignment Summary

| Task | Owner | Depends On | Est. Days |
|------|-------|------------|-----------|
| 1A. Benchmark script | Dev 1 | — | 3–4 |
| 1B. Wheel build + install | Dev 2 | — | 2–3 |
| 2A. Demo 1 (local RAG) | Dev 1 | 1B | 1 |
| 2A. Demo 2 (agent memory) | Dev 1 | 1B | 1 |
| 2A. Demo 3 (edge scale) | Dev 1 | 1B | 1 |
| 2B. README rewrite | Writer | 1A, 2A | 1 |
| 3A. Blog/arXiv paper | Writer + Dev 1 | 1A | 3–5 |
| 3B. Cost calculator | Dev 2 | — | 1 |
| Repo hygiene checklist | Dev 2 | 1B, 2A | 1–2 |
| 4A/4B. Launch posts | Founder | All above | 1 |

**Critical path:** 1B (install) → 2A (demos) → 2B (README) → 4A (launch).  
**Parallel track:** 1A (benchmark) → 3A (paper) can run alongside.

---

## Appendix: File Structure After Completion

```
NebulOS-Scaffolding/
├── README.md                          # Rewritten (Phase 2B)
├── benchmarks/
│   ├── bench_synrix_vs_mem0.py        # Phase 1A
│   ├── results/                       # Auto-generated benchmark output
│   │   ├── latest.json
│   │   └── latest.md
│   └── README.md                      # How to reproduce
├── demos/
│   ├── demo_local_rag.py              # Phase 2A
│   ├── demo_agent_memory.py           # Phase 2A
│   └── demo_edge_scale.py             # Phase 2A
├── tools/
│   └── cost_calculator.py             # Phase 3B
├── build/
│   └── linux/
│       ├── build.sh                   # Existing
│       └── build_wheel.sh             # Phase 1B (new)
├── python-sdk/                        # Existing
├── docs/
│   ├── LAUNCH_PLAN_TECHNICAL_SPEC.md  # This document
│   ├── SYNRIX_VS_REDDIT_ARXIV_MEMORY_PROJECTS.md
│   └── ...
└── .github/
    └── workflows/
        └── ci.yml                     # Phase repo hygiene
```
