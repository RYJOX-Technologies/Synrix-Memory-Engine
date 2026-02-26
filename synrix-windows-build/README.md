# SYNRIX

## Deterministic, local-first AI memory

Kill the process.  
Restart it.  
Ask the same question.  
Get the same answer instantly.

No cloud. No re-ingestion. No nondeterministic recall.

**Think SQLite for AI memory.**

[![LangChain Compatible](https://img.shields.io/badge/langchain-compatible-brightgreen)](https://python.langchain.com/)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

**Site:** [ryjoxtechnologies.com](https://www.ryjoxtechnologies.com)

---

## What SYNRIX is

SYNRIX is a **persistent binary memory engine** for AI systems.

- Local-first memory layer  
- Deterministic retrieval  
- **O(k) prefix-based queries** (scales with results, not corpus)  
- Hardware-aligned, mmap-based storage  
- Crash-safe (WAL + recovery)  
- Validated at 50M+ nodes  

**It is not:** an LLM, a chatbot, a SaaS product, or “just another vector DB.”

---

## Why it exists

Most AI memory stacks depend on:

- Cloud vector DBs  
- Embedding pipelines and approximate search  
- Re-ingestion on restart  
- Per-query cloud cost  

SYNRIX removes that. Store once. Query deterministically. Survive restarts. Run locally.

---

## Performance

- **Sub-millisecond** hot-path reads (Windows: ~17 µs lookup; Linux: sub-µs in native builds)  
- **O(k) prefix queries** — scales with result count, not dataset size  
- **~2 µs/node** add throughput (Windows); higher on Linux  
- Memory-mapped storage; no network in the hot path  

Cloud vector DBs: 10–50 ms end-to-end. SYNRIX hot path: microseconds. That’s orders of magnitude faster for local retrieval.

---

## Architecture

```
Your app → Python SDK → SYNRIX engine (DLL / .so) → memory-mapped lattice file
```

No cloud calls. One binary per platform; tier by license key at runtime.

---

## Quick start

**1. Download** the engine from [Releases](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases):

- **Windows:** `synrix-windows.zip` → `libsynrix.dll` + runtimes  
- **Linux:** `synrix-linux-x86_64.tar.gz` (or `synrix-linux-arm64.tar.gz` when available) → `libsynrix.so` + runtimes  

No key = free tier (~25k nodes). Verify with the SHA256 on the release page.

**2. Install the SDK** (from this repo):

```bash
# This repo (synrix-windows-build): use python-sdk
cd python-sdk
pip install -e .
```

If you use the full Synrix-Memory-Engine repo instead: `cd Synrix-Memory-Engine/synrix-sdks/agent-memory-sdk` then `pip install -e .`.

Put the engine in the same folder as your script, or set `SYNRIX_LIB_PATH` (Windows) or `LD_LIBRARY_PATH` / `SYNRIX_LIB_PATH` (Linux) to the folder containing the library.

**3. Use it:**

```python
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("example.lattice", max_nodes=25_000)
backend.add_node("TASK:login", "Implement user login", 5)
for r in backend.find_by_prefix("TASK:"):
    print(r["name"], r["data"])
backend.close()
```

You’re storing and querying AI memory locally. No server required.

---

## Why engineers care

- Deterministic retrieval  
- No per-query billing, no vendor lock-in  
- Local-first; data stays on your machine  
- Memory survives process death  
- Single binary; tier by key  

Infrastructure, not an app.

---

## Use cases

RAG · persistent AI agents · code intelligence · edge AI · structured task memory · pattern storage

---

## Platform & requirements

| Platform    | Status      |
|-------------|-------------|
| Windows x64 | Ready       |
| Linux ARM64 | Ready       |
| Linux x86_64| In progress |
| macOS       | In progress |

**Requirements:** Python 3.8+, engine binary from releases, under 1 GB RAM for ~25k nodes, ~1 GB disk per 1M nodes.

---

## License

- **Python SDK:** MIT  
- **Engine:** Proprietary  

Free up to ~25k nodes without a key. Higher tiers via signed license keys. See [LICENSE](LICENSE).

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| DLL / .so not found | Put the engine next to your script or set `SYNRIX_LIB_PATH` (Windows) or `LD_LIBRARY_PATH` (Linux). |
| Hit node cap | Set `SYNRIX_LICENSE_KEY` or save once: `python -c "import synrix; synrix.save_license_key('YOUR_KEY')"` |
| SDK not found | Install from this repo: `pip install -e .` in `python-sdk` (or in Synrix-Memory-Engine: `synrix-sdks/agent-memory-sdk`). |

---

## Learn more

- [User Journey & flow](USER_JOURNEY_AND_FLOW.md) · [docs/](docs/) · [GitHub issues](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/issues)  
- **Maintainers:** [docs/GITHUB_READY.md](docs/GITHUB_READY.md) before first push; [docs/RELEASE.md](docs/RELEASE.md) to create a release.

---

**SYNRIX** — Fast, local AI memory. Your data. Your machine.
