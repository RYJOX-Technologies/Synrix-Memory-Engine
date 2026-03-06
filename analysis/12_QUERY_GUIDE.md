# Synrix Memory Engine — What Queries Work, What Doesn't, and How to Best Use It

---

## Part 1: What Queries WILL Work

Synrix supports exactly **two query operations**. Everything in the SDK ultimately calls one of these.

### ✅ Query Type 1: Get by ID (O(1))

```python
node = backend.get_node(42)
# Returns: {"id": 42, "name": "USER_PREF_THEME", "data": "dark", "type": 5, ...}
```

**What it does**: Direct array lookup by numeric node ID. Like accessing `array[42]`.

**When to use it**: When you already have the node ID from a previous query or from storing the ID at write time.

**Performance**: ~192 ns (hot cache), ~3 μs (warm) — genuinely fast.

**Limitations**: You must already know the ID. There's no way to discover IDs other than prefix search.

---

### ✅ Query Type 2: Prefix Search (O(k))

```python
results = backend.find_by_prefix("USER_PREF_", limit=100)
# Returns: all nodes whose name starts with "USER_PREF_"
```

**What it does**: Finds all nodes whose `name` field starts with the given string. Returns up to `limit` matching nodes.

**When to use it**: This is the primary query mechanism. Everything depends on how you name your nodes.

**Performance**: Scales with number of matches (k), not total database size (N). Fast for small result sets.

**Limitations**: 
- **Exact prefix only** — no wildcards, no "contains", no regex
- **Name field only** — cannot search by data content
- **No filtering** — cannot combine with type, timestamp, confidence, etc.
- **No ordering** — results come in insertion/index order, not sorted

---

## Part 2: What Queries WON'T Work

### ❌ Semantic / Fuzzy Search

```python
# WILL NOT WORK — no similarity matching
results = backend.find_by_prefix("How do I handle errors in Python?")
# Returns: nothing (no node name starts with that sentence)
```

**Why**: Synrix matches literal string prefixes. It doesn't understand meaning. "errors" won't match `DOC_PYTHON_ERROR_HANDLING` unless the query string is literally `"DOC_PYTHON_ERROR_HANDLING"` or a prefix of it like `"DOC_PYTHON_"`.

---

### ❌ Suffix / Contains / Regex Search

```python
# WILL NOT WORK
results = backend.find_by_prefix("_PYTHON_")     # Not a prefix search
results = backend.find_by_prefix("*PYTHON*")     # No wildcards
results = backend.find_by_prefix("%.PYTHON.%")   # No SQL-style patterns
```

**Why**: Only the beginning of the name is indexed. You cannot search for strings that appear in the middle or end.

---

### ❌ Search by Data Content

```python
# WILL NOT WORK — data field is not indexed
backend.add_node("DOC_1", "Python uses asyncio for concurrency")
results = backend.find_by_prefix("asyncio")  # Returns nothing
# "asyncio" is in the data, not in the name
```

**Why**: Only the `name` field is indexed. The `data` field is stored but not searchable. To find data by content, you must encode the searchable content into the name.

---

### ❌ Filter by Type, Timestamp, Confidence

```python
# WILL NOT WORK — no field filtering
results = backend.find_by_prefix("DOC_", type=5)           # No type filter
results = backend.find_by_prefix("DOC_", after="2024-01-01") # No date filter  
results = backend.find_by_prefix("DOC_", min_confidence=0.8) # No confidence filter
```

**Why**: The prefix index only indexes the name field. Other fields (type, timestamp, confidence) are stored but cannot be used in queries. You must filter in Python after retrieving results.

---

### ❌ Sort Results

```python
# WILL NOT WORK — no ORDER BY
results = backend.find_by_prefix("LOG_", order_by="timestamp")  # No sorting
results = backend.find_by_prefix("LOG_", order_by="name")        # No sorting
```

**Why**: Results come back in whatever order the index returns them. To sort, retrieve all results and sort in Python.

---

### ❌ Count Without Fetching

```python
# WILL NOT WORK — no COUNT operation
count = backend.count_by_prefix("DOC_")  # This method doesn't exist
```

**Workaround**: `len(backend.find_by_prefix("DOC_", limit=50000))` — fetches all results to count them.

---

### ❌ Delete / Update

```python
# WILL NOT WORK — no delete or update operations
backend.delete_node(42)                          # Doesn't exist
backend.update_node(42, data="new value")        # Doesn't exist
```

**Why**: The lattice is append-only. To "update," you add a new node with the same name. To "delete," there's no mechanism — the node stays forever (until the file is recreated).

**Workaround for updates**: Add a new node with the same name. When querying, take the last result (most recently added). But this means old values accumulate as dead weight.

---

### ❌ Range Queries

```python
# WILL NOT WORK
results = backend.find_by_prefix("LOG_", timestamp_min=1000, timestamp_max=2000)
results = backend.find_by_prefix("LOG_2024-01", ...)  # Only if name encodes the date
```

**Why**: No range predicates on any field. The only range-like behavior is prefix matching, which works on the name string.

**Partial workaround**: If you encode timestamps into names (`LOG_20240115_12:00:00_event`), then `find_by_prefix("LOG_20240115_")` returns all logs from January 15. But this is crude — you can't do "between January 10 and January 20" without querying each day separately.

---

### ❌ Join / Cross-Reference

```python
# WILL NOT WORK
# "Find all sensors for robots that had failures"
# This requires joining ROBOT:*:SENSOR: with ROBOT:*:FAILURE:
```

**Why**: No join operations. Each prefix query is independent. To correlate data across prefixes, you must query multiple prefixes and merge results in Python.

---

### ❌ Aggregation

```python
# WILL NOT WORK
# "Average confidence across all LEARNING_ nodes"
# "Count of nodes per type"
# "Most recent node by timestamp"
```

**Why**: No aggregation functions. Fetch all results, aggregate in Python.

---

## Part 3: How to Best Use Synrix

If you choose to use Synrix, here's how to get the most out of its narrow capabilities.

### Rule 1: Design Your Prefix Scheme First

Your entire query strategy depends on how you name nodes. **Invest time in prefix design before writing any code.**

```
Good prefix scheme:
  AGENT:{agent_id}:PREF:{preference_name}
  AGENT:{agent_id}:CONV:{session_id}:{timestamp}
  AGENT:{agent_id}:FAIL:{error_type}:{timestamp}
  DOC:{category}:{subcategory}:{doc_id}

Bad prefix scheme:
  preference_dark_mode        ← Can't query "all preferences"
  agent_123_conversation_5    ← Delimiter ambiguity
  my data about python        ← Not prefix-searchable
```

### Rule 2: Put Queryable Information in the Name, Not the Data

```python
# BAD — "asyncio" is in data, unsearchable
backend.add_node("DOC_1", "Python asyncio tutorial")

# GOOD — queryable by topic
backend.add_node("DOC_PYTHON_ASYNCIO_TUTORIAL", "Python asyncio uses event loops...")

# GOOD — queryable by both category and topic  
backend.add_node("DOC_PYTHON_ASYNCIO_1", "asyncio uses event loops for concurrency")
# Query: find_by_prefix("DOC_PYTHON_ASYNCIO_")   → all asyncio docs
# Query: find_by_prefix("DOC_PYTHON_")            → all Python docs
# Query: find_by_prefix("DOC_")                   → all docs
```

### Rule 3: Use Hierarchical Prefixes for Drill-Down Queries

```python
# Store with hierarchical structure
backend.add_node("ROBOT:arm01:SENSOR:lidar:1709000001", data)
backend.add_node("ROBOT:arm01:SENSOR:lidar:1709000002", data)
backend.add_node("ROBOT:arm01:SENSOR:camera:1709000001", data)
backend.add_node("ROBOT:arm02:SENSOR:lidar:1709000001", data)

# Query at different levels of specificity:
find_by_prefix("ROBOT:")                          # All robots
find_by_prefix("ROBOT:arm01:")                    # All arm01 data
find_by_prefix("ROBOT:arm01:SENSOR:")             # All arm01 sensors
find_by_prefix("ROBOT:arm01:SENSOR:lidar:")       # All arm01 lidar readings
```

### Rule 4: Use "latest" Pointers for Current State

Since there's no sorting or "most recent" query:

```python
# Store timestamped history AND a "latest" pointer
backend.add_node("STATE:pose:1709000001", '{"x":1.0, "y":2.0}')
backend.add_node("STATE:pose:1709000002", '{"x":1.5, "y":2.3}')
backend.add_node("STATE:pose:latest",     '{"x":1.5, "y":2.3}')  # Always overwrite

# Fast "what's the current state?" query:
results = find_by_prefix("STATE:pose:latest")  # 1 result, instant

# Historical query (returns all, sort in Python):
results = find_by_prefix("STATE:pose:")  # All poses including "latest"
```

### Rule 5: Keep Data Under 512 Bytes

Each node's `data` field is limited to 512 bytes. Plan for this:

```python
# BAD — will be silently truncated at 511 bytes
backend.add_node("DOC_1", very_long_document_text)

# GOOD — store a summary or reference
backend.add_node("DOC_1", json.dumps({
    "title": "Asyncio Tutorial",
    "summary": "Event loops, coroutines, tasks",  
    "file_path": "/docs/asyncio.md"  # Reference to full content
}))

# ALTERNATIVE — use chunked storage for larger data
backend.add_node_chunked("LARGE_DOC_1", large_data_bytes)
# But retrieval is slower: backend.get_node_chunked(parent_id)
```

### Rule 6: Build a Keyword-to-Prefix Map for Search

If you need to answer natural language queries, you must build the mapping yourself:

```python
QUERY_MAP = {
    "async": "DOC_PYTHON_ASYNCIO",
    "await": "DOC_PYTHON_ASYNCIO",
    "type hints": "DOC_PYTHON_TYPING",
    "ownership": "DOC_RUST_OWNERSHIP",
    "kubernetes": "DOC_K8S_",
    "docker": "DOC_DEVOPS_DOCKER",
}

def query_to_prefixes(user_query):
    """Map user query to Synrix prefixes."""
    words = user_query.lower().split()
    prefixes = []
    for word in words:
        if word in QUERY_MAP:
            prefixes.append(QUERY_MAP[word])
    return prefixes or ["DOC_"]  # Fallback: search all docs

# Usage:
for prefix in query_to_prefixes("How does async work?"):
    results = backend.find_by_prefix(prefix)
```

This is exactly what `hello_memory.py` does. It's manual, it doesn't scale to unknown queries, but it works for bounded domains.

### Rule 7: Filter and Sort in Python After Retrieval

```python
# Synrix can only give you "all nodes matching a prefix"
# Everything else is Python post-processing

results = backend.find_by_prefix("LOG_ERROR_", limit=1000, raw=False)

# Filter by timestamp (in Python)
recent = [r for r in results 
          if json.loads(r["data"]).get("timestamp", 0) > cutoff_time]

# Sort by timestamp (in Python)
recent.sort(key=lambda r: json.loads(r["data"]).get("timestamp", 0), reverse=True)

# Get top 10 most recent
top_10 = recent[:10]

# Count by type (in Python)  
type_counts = {}
for r in results:
    t = r.get("type", 0)
    type_counts[t] = type_counts.get(t, 0) + 1
```

### Rule 8: Use the Raw Backend, Not the HTTP Client

If performance matters, bypass the HTTP server:

```python
# SLOW (~500μs per query via HTTP)
client = SynrixClient(host="localhost", port=6334)
results = client.query_prefix("PREFIX_")

# FAST (~5-50μs per query via ctypes)
backend = RawSynrixBackend("file.lattice")
results = backend.find_by_prefix("PREFIX_", raw=True)  # raw=True skips string decode
```

The raw backend is 10-100x faster because it skips HTTP, JSON, and TCP overhead. The HTTP client exists for the Qdrant-compatible API, but for agent memory workloads, use raw.

### Rule 9: Save Explicitly and Periodically

```python
# Writes go to memory (and WAL if enabled). Periodically save:
for i, item in enumerate(items):
    backend.add_node(f"ITEM_{i}", data)
    if i % 1000 == 0:
        backend.save()  # Persist to disk

backend.save()       # Final save
backend.checkpoint()  # Flush WAL
```

### Rule 10: Plan for the 25K Limit

The free tier limits you to 25,000 nodes. At ~1KB each, that's 25MB. Plan accordingly:

```python
# Check remaining capacity
usage = backend.get_usage_info()
print(f"Used: {usage['current']}/{usage['limit']} ({usage['percentage']}%)")

# Be judicious — don't store every sensor reading
# Store summaries instead of raw data
# Use "latest" pointers instead of timestamped history
```

---

## Part 4: Decision Flowchart

```
Do you need to search by meaning/similarity?
  YES → Don't use Synrix. Use a vector database.
  NO ↓

Can you organize ALL your data into predictable prefix namespaces?
  NO → Don't use Synrix. Use SQLite with full-text search.
  YES ↓

Is your data under 512 bytes per record?
  NO → Don't use Synrix (or use chunking, which is slow). Use SQLite.
  YES ↓

Do you need more than 25K records?
  YES → Don't use Synrix free tier. Use SQLite (unlimited, free).
  NO ↓

Do you need to filter/sort/aggregate within query results?
  YES → Use Synrix for retrieval, but plan to post-process in Python.
  NO ↓

Do you need transactions (atomic multi-record writes)?
  YES → Don't use Synrix. Use SQLite.
  NO ↓

Are you running on an edge device (Jetson, Pi) with extreme latency requirements?
  YES → Synrix's mmap raw backend may genuinely help. Use it.
  NO ↓

Do you need an open-source, auditable storage engine?
  YES → Use SQLite. Synrix's engine is proprietary.
  NO → Synrix will work for your use case. Follow the rules above.
```

---

## Part 5: Quick Reference Card

| I want to... | Synrix can? | How |
|--------------|:-----------:|-----|
| Store a key-value pair | ✅ | `add_node("KEY", "value")` |
| Get by known ID | ✅ | `get_node(42)` |
| Find all items in a category | ✅ | `find_by_prefix("CATEGORY_")` |
| Find by exact name | ✅ | `find_by_prefix("EXACT_NAME")` then filter |
| Find by partial name (middle) | ❌ | Not supported |
| Find by data content | ❌ | Put searchable info in the name instead |
| Find similar content | ❌ | Use a vector database |
| Sort results | ❌ | Sort in Python after retrieval |
| Filter by field | ❌ | Filter in Python after retrieval |
| Count items | ❌ | `len(find_by_prefix(...))` |
| Delete a record | ❌ | Not supported |
| Update a record | ❌ | Add new node with same name, take latest |
| Range query (dates, numbers) | ❌ | Encode range values in prefix, query per-bucket |
| Join two datasets | ❌ | Query each prefix, merge in Python |
| Store > 512 bytes | ⚠️ | Use `add_node_chunked()` (slower) |
| Survive a crash | ✅ | WAL enabled by default |
| Work offline | ✅ | Fully local, no network needed |
