# Synrix Memory Engine — API Surface Area Truthfulness Audit

*Which advertised features are real and which are facades?*

---

## 1. Methodology

For every public class and method in the Python SDK, I traced the call chain to identify:
- **Real**: Backed by a distinct C engine operation or genuinely unique logic
- **Facade**: Wraps a real operation with naming conventions but no new functionality
- **Stub**: Declared but not implemented, or implemented with TODOs
- **Broken**: Would likely fail if actually used

---

## 2. Core API — What's Real

### ✅ Real: `RawSynrixBackend` (raw_backend.py)

| Method | C Function Called | Verdict |
|--------|-----------------|---------|
| `add_node(name, data)` | `lattice_add_node()` | ✅ Real — ctypes FFI to C |
| `add_node_binary(name, data)` | `lattice_add_node_binary()` | ✅ Real — binary data variant |
| `add_node_chunked(name, data)` | `lattice_add_node_chunked()` | ✅ Real — large data chunking |
| `get_node(node_id)` | `lattice_get_node_copy()` + `lattice_free_node_copy()` | ✅ Real — proper memory management |
| `find_by_prefix(prefix)` | `lattice_find_nodes_by_name()` | ✅ Real — prefix index query |
| `save()` | `lattice_save()` | ✅ Real — persist to disk |
| `checkpoint()` | `lattice_wal_checkpoint()` | ✅ Real — WAL checkpoint |
| `build_prefix_index()` | `lattice_build_prefix_index()` | ✅ Real — explicit index build |
| `get_node_chunked(parent_id)` | `lattice_get_node_chunked_to_buffer()` | ✅ Real — reassemble chunked data |
| `get_hardware_id()` | `lattice_get_hardware_id()` | ✅ Real — hardware fingerprinting |
| `add_node_auto(data)` | Calls `auto_organizer.classify()` → `add_node()` | 🟡 Real but auto_organizer is just keyword matching |

**Verdict**: The raw backend is the most genuine part of the SDK. ~10 C functions properly wrapped via ctypes.

### ✅ Real: `SynrixClient` (client.py)

| Method | HTTP Endpoint | Verdict |
|--------|--------------|---------|
| `create_collection(name)` | `PUT /collections/{name}` | ✅ Real HTTP call |
| `delete_collection(name)` | `DELETE /collections/{name}` | ✅ Real HTTP call |
| `list_collections()` | `GET /collections` | ✅ Real HTTP call |
| `add_node(name, data)` | `POST /synrix/nodes` | ✅ Real HTTP call |
| `query_prefix(prefix)` | `POST /collections/{col}/query` | ✅ Real HTTP call |
| `upsert_points(collection, points)` | `PUT /collections/{col}/points` | ✅ Real HTTP call |
| `search_points(collection, vector)` | `POST /collections/{col}/points/search` | ⚠️ Real HTTP call, but **what the server does with vectors is unverifiable** |
| `get_point(collection, point_id)` | `GET /collections/{col}/points/{id}` | ✅ Real HTTP call |

**Verdict**: HTTP client works. The critical question is `search_points` — see Section 3.

### ✅ Real: `SynrixDirectClient` (direct_client.py)

Genuine shared memory IPC using POSIX `/dev/shm`. Proper binary protocol with struct offsets. Would work if the server supports it.

### ✅ Real: `SynrixMockClient` (mock.py)

Proper in-memory mock implementation. Clean and useful for testing.

---

## 3. The Qdrant-Compatible Vector Search — Real or Facade?

### What the Code Shows

`SynrixClient.search_points()` sends a properly formatted HTTP request:
```python
data = {"vector": vector, "limit": limit}
response = self._request("POST", f"/collections/{collection}/points/search", data=data)
```

`SynrixVectorStore` (LangChain adapter) calls:
```python
query_vector = self.embedding.embed_query(query)
results = self.client.search_points(collection, vector=query_vector, limit=k)
```

### What We Can't Verify

The server binary (`synrix-server-evaluation`) is proprietary. We don't know:
1. Does it compute actual cosine/euclidean/dot-product similarity?
2. Does it use an index (HNSW, IVF) or brute-force?
3. Does it even store vectors, or just ignore them and return something else?
4. Are the returned `score` values actual similarity scores?

### Evidence Suggesting It Might Not Work

1. The entire rest of the system is prefix-based — no other part of the codebase uses vectors
2. The `create_collection` API accepts `vector_dim` but defaults to 128 "for compatibility, but not enforced by engine"
3. No vector search benchmarks exist anywhere in the repo
4. The LangChain adapter's `_store_meta` stores document text as prefix-indexed nodes (not vectors) as a separate operation — suggesting the vector storage alone isn't sufficient for retrieval
5. `delete()` in the VectorStore returns `False` with a comment "Point deletion is not currently exposed"

### Verdict

**The vector search API exists as HTTP endpoints, but there is zero evidence it performs actual vector similarity computation.** The entire architecture is designed for prefix matching, and vector support appears to be a compatibility shim that accepts vectors but may not meaningfully use them. This cannot be verified without the source code or independent testing.

---

## 4. Agent/Integration Modules — Facades

### `SynrixMemory` (agent_memory.py) — Facade

Every method is `json.dumps()` → `client.add_node()` or `client.query_prefix()` → `json.loads()`:

| Method | Real operation | New logic? |
|--------|---------------|:----------:|
| `write(key, value)` | `client.add_node(key, json.dumps(value))` | ❌ JSON wrapper |
| `read(pattern)` | `client.query_prefix(prefix)` → `json.loads()` | ❌ JSON wrapper |
| `get_last_attempts(task_type)` | `client.query_prefix(f"task:{task_type}:")` | ❌ Prefix convention |
| `get_failed_attempts(task_type)` | Same query + `"fail" in value` string check | ❌ Fragile string matching |
| `get_successful_patterns(task_type)` | Same query + `"success" in value` string check | ❌ Fragile string matching |
| `get_node_by_id(node_id)` | `client.get_node_by_id(node_id)` | ❌ Direct passthrough |

### `SynrixAgentBackend` (agent_backend.py) — Duplicate Facade

Does exactly the same thing as `SynrixMemory` with different method names:

| agent_backend method | Equivalent agent_memory method |
|---------------------|-------------------------------|
| `write(key, value)` | `write(key, value)` |
| `read(key)` | `read(key)` |
| `query_prefix(prefix)` | `read(pattern)` |
| `get_task_memory(task_type)` | `get_task_memory_summary(task_type)` |

### `AgentMemoryLayer` (agent_memory_layer.py) — Triple Facade

Does the same thing again:

| agent_memory_layer method | Maps to |
|--------------------------|---------|
| `remember(key, value)` | `backend.add_node(key, json.dumps(value))` |
| `recall(key)` | `backend.find_by_prefix(key)` → `json.loads()` |
| `search(prefix)` | `backend.find_by_prefix(prefix)` → `json.loads()` |

### `CursorIntegration` (cursor_integration.py) — Wrapper of wrapper

```python
def remember(self, key, value):
    backend = _get_backend()  # Returns SynrixAgentBackend
    backend.write(key, value)  # Which calls client.add_node()
```

Three levels of indirection: `cursor_integration.remember()` → `agent_backend.write()` → `client.add_node()`

### `DevinSynrixIntegration` (devin_integration.py) — Stub

| Feature | Status |
|---------|--------|
| `check_past_errors()` | Facade: `memory.get_task_memory_summary()` |
| `apply_fixes()` | 🔴 Stub: Naive string replacements (`code.replace("/ 0", "/ 1")`) |
| `store_result()` | Facade: `memory.write()` |
| `DevinAISynrixWrapper.execute_with_memory()` | 🔴 Stub: `# TODO: Replace with DevinAI API call` |
| `_execute_code()` | 🔴 Dangerous: Runs arbitrary Python via `subprocess.run()` |

The "Devin integration" doesn't integrate with Devin. It's aspirational code that would execute arbitrary Python files on the user's machine.

### `AssistantMemory` (assistant_memory.py) — Facade

Same pattern: `json.dumps()` → `memory.write()`, `memory.query_prefix()` → `json.loads()`

Added "feature": `_classify_query()` checks if query contains words like "write", "fix", "explain" — a 15-line keyword matcher presented as query classification.

### `RoboticsNexus` (robotics.py) — Facade

Every method is `json.dumps(data)` → `memory.add(f"ROBOT:{id}:TYPE:{key}")`. The naming convention is the only contribution.

---

## 5. Feature Truthfulness Matrix

| Advertised Feature | Real? | What Actually Happens |
|-------------------|:-----:|----------------------|
| Add/query/get nodes | ✅ | Genuine C engine operations via ctypes/HTTP |
| Prefix-based retrieval | ✅ | Real prefix index in C engine |
| WAL crash recovery | ✅ | Real WAL + fsync (tested in C) |
| Shared memory IPC | ✅ | Real POSIX shm implementation |
| Mock client for testing | ✅ | Real in-memory mock |
| LangChain VectorStore | ⚠️ | HTTP calls work, vector similarity unverifiable |
| Qdrant-compatible search | ⚠️ | Endpoints exist, computation unknown |
| Agent memory | ❌ Facade | `json.dumps()` + `add_node()` with prefix convention |
| Cursor integration | ❌ Facade | Wrapper of wrapper of `add_node()` |
| Devin integration | ❌ Stub | TODOs, no actual Devin API calls |
| Robotics memory | ❌ Facade | `json.dumps()` + naming convention |
| Auto-organizer | ❌ Facade | Keyword matching, not NLP |
| Failure tracking | ❌ Facade | `"fail" in value` string check |
| Context restoration | ❌ Facade | `find_by_prefix("CONSTRAINT:")` + `("PATTERN:")` |
| Episodic memory | ❌ Facade | Prefix query with `json.loads()` |
| Semantic aging | ❌ Not found | Mentioned in early commit, no code exists |

---

## 6. The Real API (What Actually Exists)

If you strip away all facades, the genuine API surface is:

```python
# The REAL Synrix API (everything else is a wrapper)

# Via ctypes (fastest):
backend = RawSynrixBackend("file.lattice")
node_id = backend.add_node("NAME", "data")        # Write
results = backend.find_by_prefix("PREFIX_")        # Query
node = backend.get_node(node_id)                   # Read
backend.save()                                      # Persist
backend.checkpoint()                                # WAL checkpoint

# Via HTTP (most common):
client = SynrixClient(host="localhost", port=6334)
client.create_collection("col")                     # Setup
client.add_node("NAME", "data", collection="col")  # Write
results = client.query_prefix("PREFIX_", collection="col")  # Query

# Via shared memory (niche):
direct = SynrixDirectClient()
direct.add_node("NAME", "data", collection="col")
results = direct.query_prefix("PREFIX_", collection="col")
```

**That's the entire real API.** Everything advertised beyond this is `json.dumps()` + prefix naming conventions.
