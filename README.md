# Synrix Memory Engine

Persistent local memory for AI systems.

Kill the process.
Restart it.
Ask the same question.
Get the same answer instantly.

No cloud.
No re-ingestion.
No nondeterministic recall.

Think SQLite for AI and autonomous memory.

[![LangChain Compatible](https://img.shields.io/badge/langchain-compatible-brightgreen)](https://python.langchain.com/)
[![OpenAI API](https://img.shields.io/badge/api-openai--compatible-blue)](https://platform.openai.com/)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

**Official site:** https://www.ryjoxtechnologies.com

---

## Why Synrix?

Most AI memory stacks rely on:

- Cloud-hosted vector databases (latency, cost, vendor lock-in)
- Approximate global search that degrades as datasets grow
- Heavy indexing and tuning pipelines

Synrix runs locally and uses deterministic retrieval:

- Queries scale with results (O(k)), not total dataset size
- Microsecond-scale local lookups
- No network variance
- No cold starts
- No per-query cloud latency

**Architecture**

```
Your App → Python SDK → Synrix Engine (DLL) → Local Storage
```

- **Python SDK:** MIT licensed
- **Engine:** local binary
- No network calls required

---

## Quick Start (2–3 minutes)

### 1. Download

Download the Windows package from [Releases](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases):

- **`synrix-windows.zip`**

Unzip it. You get the engine DLL and runtimes (e.g. `libsynrix.dll`, OpenSSL DLLs). The same binary is used for all tiers.

If you do not set a license key, Synrix runs with a free default cap (~25k nodes), so you can try it immediately.

### 2. Install the Python SDK

The SDK lives in this repo. Clone the repo, then install one of the SDKs (e.g. agent-memory or robotics):

```powershell
git clone https://github.com/RYJOX-Technologies/Synrix-Memory-Engine.git
cd Synrix-Memory-Engine\synrix-sdks\agent-memory-sdk
pip install -e .
```

Put the unzipped engine DLLs in the same folder as your script, or in the SDK's `synrix/` folder, or set `SYNRIX_LIB_PATH` to the folder containing `libsynrix.dll`.

### 3. Use It

```python
from synrix.raw_backend import RawSynrixBackend

# Open or create a memory store (25k default)
backend = RawSynrixBackend("example.lattice", max_nodes=25_000)

# Store data
backend.add_node("TASK:login", "Implement user login", 5)

# Query by prefix
results = backend.find_by_prefix("TASK:")
for r in results:
    print(r["name"], r["data"])

backend.close()
```

That's it. You're storing and querying AI memory locally.

---

## What Makes This Different?

**O(k) queries** — Many vector databases rely on approximate global search that degrades as datasets grow. Synrix queries scale with the number of matching results, not the total dataset size.

**Local-first** — Everything runs on your machine. No cloud dependency. No data leaves your environment.

**Single binary** — Same engine for everyone. Node limits are set by a signed license key at runtime.

---

## What Synrix Is

- A persistent local memory engine
- A deterministic retrieval substrate
- A low-level primitive you embed in your stack
- Hardware-aligned, mmap-based storage

## What Synrix Is Not

- Not an LLM
- Not an agent framework
- Not a cloud vector database
- Not a SaaS platform

---

## Who Synrix Is For

- ML engineers building agents
- Infra teams replacing vector DBs
- Robotics / edge AI systems
- Code intelligence systems
- AI-native startups running local-first stacks

If you're building AI systems that need memory, Synrix sits underneath your model.

---

## Use Cases

- Retrieval Augmented Generation (RAG)
- AI agent memory
- Structured task storage
- Local AI state management
- Low-latency recall pipelines

---

## Requirements

- **Windows x64**
- **Python 3.8+**
- **Engine:** `libsynrix.dll` (included in `synrix-windows.zip`) plus the runtime DLLs in that zip
- **RAM:** <1GB for ~25k nodes (scales with node count)
- **Disk:** ~1GB per 1M nodes

---

## Platform Support

| Platform    | Status        |
|------------|----------------|
| Windows x64| Ready          |
| Linux ARM64| In progress    |
| Linux x86_64 | In progress  |
| macOS      | In progress    |

---

## License

- **Python SDK:** MIT
- **Engine:** Proprietary

The engine runs freely up to ~25k nodes without a key. Higher limits (1M / 10M / 50M / unlimited) are enabled via signed license keys.

See [LICENSE](LICENSE) for details.

---

## Troubleshooting

**DLL not found or process exits on init?**  
Ensure `libsynrix.dll`, `libcrypto-3-x64.dll`, and `libssl-3-x64.dll` are in the same folder as your script or on PATH.

**Hit the node cap?**  
Set `SYNRIX_LICENSE_KEY` to a higher-tier key.

**Python cannot find the SDK?**  
Install from this repo (`synrix-sdks/agent-memory-sdk` or `synrix-sdks/robotics-sdk`) with `pip install -e .` or ensure `synrix` is on `sys.path`.

---

## Getting Help

- Open a [GitHub issue](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/issues)
- See this repo's `synrix-sdks/` for SDK source and READMEs

---

**Synrix Memory Engine** — Fast, local AI memory. Your data. Your machine.
