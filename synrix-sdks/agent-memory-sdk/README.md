# SYNRIX Agent Memory SDK

Python SDK for **SYNRIX agent memory**: a local, persistent key–value store designed for AI agents and cross-turn context. No server, no embeddings in the engine—store by prefix/key and query by prefix.

---

## For AI assistants (GPT, Claude, Cursor, Gemini)

**What SYNRIX is:** A fast, local, persistent store for agent memory. You **add** items with a **prefix key** (e.g. `USER:name`, `PROJECT:architecture`) and **query by prefix** to get all items under that namespace. Payloads >511 bytes are stored in chunks automatically. One lattice file per app (or per user); no network, no vector DB inside the engine.

**API (use this surface):**

```python
from synrix.ai_memory import get_ai_memory   # or: from synrix.ai_memory import AIMemory

memory = get_ai_memory()   # default lattice: ~/.synrix_ai_memory.lattice
# Or: memory = AIMemory(lattice_path="/path/to/app.lattice")

# Add (key = prefix-style; use colons for namespaces: PROJECT:doc:id)
memory.add("USER:name", "Alice")
memory.add("PROJECT:architecture", "Microservices with REST APIs")

# Query by prefix (returns list of {"id", "name", "data"})
results = memory.query("PROJECT:")       # all keys starting with PROJECT:
results = memory.query("USER:", limit=10)

# Get one node by id (O(1))
node = memory.get(node_id)   # {"id", "name", "data"}

# Count / list
n = memory.count()
all_user = memory.list_all("USER:")

# When done (optional; closes backend)
memory.close()
```

**Key naming:** Use a consistent prefix scheme so query returns a namespace (e.g. `TASK:123:plan:`, `TASK:123:outcomes:`). No embeddings—retrieval is by prefix match only.

**Errors:** On add, if the free-tier node limit (e.g. 25k) is reached, the SDK raises `FreeTierLimitError`. Catch it and inform the user: limit reached; they can delete some nodes to free space or upgrade; no modal popups—handle in conversation.

```python
from synrix.raw_backend import FreeTierLimitError

try:
    memory.add("KEY", "value")
except FreeTierLimitError:
    # Tell user: limit reached; suggest deleting nodes or upgrading
    pass
```

**Setup:** Install with `pip install -e .` from this directory. The engine is a DLL: set `SYNRIX_LIB_PATH` to the directory containing `libsynrix.dll`, or place the DLL in `agent-memory-sdk/synrix/`. No key = free tier (25k nodes); set `SYNRIX_LICENSE_KEY` for higher limits.

---

## What this is

This SDK is the Python client for the SYNRIX engine (DLL). It exposes **agent memory** via `AIMemory` / `get_ai_memory()`. Free tier has a 25k node limit; the SDK raises `FreeTierLimitError` when the limit is reached.

## Requirements

- Windows 10+
- Python 3.8+
- SYNRIX engine: `libsynrix.dll` (and runtime deps: `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `zlib1.dll`)

## Quick start (Windows)

1) Install the SDK
```bash
cd agent-memory-sdk
pip install -e .
```

2) Provide the engine DLL  
**Option A:** Copy into `agent-memory-sdk/synrix/`: `libsynrix.dll` plus the runtime deps above.  
**Option B:** Set the DLL directory:
```powershell
$env:SYNRIX_LIB_PATH = "C:\path\to\directory\containing\libsynrix.dll"
```

Tier is set by `SYNRIX_LICENSE_KEY`. No key = default cap (25k).

3) Use the SDK
```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("PROJECT:architecture", "Microservices with REST APIs")
results = memory.query("PROJECT:")
print(results[:3])
```

## Tier / limit

Node limit is set by your license key (`SYNRIX_LICENSE_KEY`). When the limit is reached, the SDK raises `FreeTierLimitError`; catch it and inform the user (e.g. delete nodes to free space or upgrade).

## Testing

Tests are not included in this repo. With the Synrix DLL, you can run the test suite from the development package (add, query, get, chunked storage, limit enforcement).

## Examples

- **`python -m synrix.examples.agent_memory_demo`** – Public demo: store key/value by prefix, query by prefix. Set `SYNRIX_LIB_PATH` first.
- See `synrix/examples/README.md` for details.
