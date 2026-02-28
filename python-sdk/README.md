# SYNRIX Python SDK

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python 3.8+](https://img.shields.io/badge/python-3.8+-blue.svg)](https://www.python.org/downloads/)

**Python SDK for SYNRIX - A local-first semantic memory system for AI applications**

SYNRIX provides persistent semantic memory for AI systems, enabling them to remember, reason, and learn over time. This SDK is the Python client library that connects to the SYNRIX engine.

**⚠️ Important:** SYNRIX is a **semantic memory system**, not a traditional knowledge graph. SYNRIX does **not** support arbitrary graph traversal, edge queries, or RDF-style relations. It uses prefix-based semantic queries optimized for agent memory workloads.

> **📖 Documentation:**
> - **Why not a knowledge graph?** [`SYNRIX_VS_KNOWLEDGE_GRAPH.md`](../docs/technical/SYNRIX_VS_KNOWLEDGE_GRAPH.md) - Complete comparison
> - **How SYNRIX achieves KG benefits:** [`SYNRIX_ARCHITECTURAL_INNOVATIONS.md`](../docs/technical/SYNRIX_ARCHITECTURAL_INNOVATIONS.md) - Deep dive into architectural innovations

---

## 🚀 Quick Start

### 1. Install the SDK

```bash
pip install synrix
```

Or install from source:
```bash
git clone https://github.com/synrix/synrix-python-sdk
cd synrix-python-sdk
pip install -e .
```

### 2. Install the Engine

**⚠️ Important:** The SDK is just the client library. You need **both the SDK and the engine** to use SYNRIX.

**Recommended (one command):**
```bash
synrix install-engine
```

This installs the evaluation engine to `~/.synrix/bin/` for your platform.

**Quick start (Linux) – direct engine (libsynrix.so):**  
Same `pip install -e .` as above. Provide the engine by either copying `libsynrix.so` (and any runtime deps) into the `synrix/` package directory, or setting **`SYNRIX_LIB_PATH`** or **`LD_LIBRARY_PATH`** to the directory containing `libsynrix.so`. Build from repo root: `./build/linux/build.sh`, then e.g. `export LD_LIBRARY_PATH=/path/to/build/linux/out:$LD_LIBRARY_PATH`.

**Manual download (optional):**
```bash
# Linux x86_64
wget https://releases.synrix.dev/synrix-server-evaluation-0.1.0-linux-x86_64
chmod +x synrix-server-evaluation-0.1.0-linux-x86_64
```

**Run the engine in evaluation mode (free, local-only):**
```bash
./synrix-server-evaluation-0.1.0-linux-x86_64 --port 6334
```

**Free Evaluation Engine:**
- ✅ Single node, local-only
- ✅ Hard limits (25k nodes free tier; see docs/TIER_AND_LICENSE_SPEC.md)
- ✅ Perfect for development and evaluation
- ✅ No signup required
- ⚠️ **Note:** Evaluation mode enforces limits to prevent misuse and is not representative of production-scale performance.

### 3. Initialize (Optional)

```python
import synrix
synrix.init()
```

If the engine is missing, it will prompt:

```
SYNRIX engine not found.
Run: synrix install-engine
```

### 4. Use the SDK

```python
from synrix import SynrixClient

# Connect to the engine
client = SynrixClient(host="localhost", port=6334)

# Create a collection
client.create_collection("knowledge_base")

# Add knowledge nodes
node_id = client.add_node(
    "ISA_ADD", 
    "Addition operation",
    collection="knowledge_base"
)

# Query by prefix (O(k) semantic search)
results = client.query_prefix("ISA_", collection="knowledge_base")
print(f"Found {len(results)} nodes")
```

**That's it!** You now have a working SYNRIX semantic memory system.

---

## 📖 What is SYNRIX?

SYNRIX is a **local-first semantic memory system** designed for AI applications. It provides:

- **Persistent Semantic Memory** - AI systems can remember what they've learned
- **O(k) Semantic Queries** - Prefix-based search that scales with results, not data
- **Local-First Architecture** - Everything runs on your machine, zero vendor lock-in
- **Local, bounded latency** - End-to-end typically ~1ms on local hardware (cold reads depend on storage)
- **Deterministic Behavior** - Same query = same result, always

**Think of it as:**
- The **long-term memory** for AI agents
- A **semantic index** that scales with results, not data
- A **local-first** alternative to cloud vector databases

**⚠️ What SYNRIX is NOT:**
- ❌ Not a general-purpose knowledge graph (no graph traversal, no edge queries, no SPARQL/RDF)
- ❌ Not a vector database (no native similarity search; embeddings can be stored but aren't queryable via similarity)
- ❌ Not a traditional database (no SQL, no flexible schemas)
- ✅ It's a **semantic memory system** optimized for structured agent memory

> **📖 Why not a knowledge graph?** See [`docs/technical/SYNRIX_VS_KNOWLEDGE_GRAPH.md`](../docs/technical/SYNRIX_VS_KNOWLEDGE_GRAPH.md) for a detailed comparison.

### Semantic Memory System vs Vector Database

**SYNRIX is NOT a full replacement for vector databases.** They solve different problems:

| Feature | SYNRIX | Vector DBs |
|---------|--------|-----------|
| **Query Type** | Semantic prefix queries | Similarity search |
| **Best For** | Structured, semantic data | Unstructured, fuzzy matching |
| **Performance** | O(k) where k = results | O(n) or O(log n) |
| **Latency** | Local, bounded (end-to-end typically ~1ms on local hardware) | 50ms+ |
| **Location** | Local | Cloud |
| **Vendor Lock-in** | None | Yes |
| **Structure** | Semantic, hierarchical | Flat, unstructured |

**Example:**
If you have 1 million nodes but only 100 match `ISA_*`, a SYNRIX query only scans those 100. A vector DB would need to compare against all 1 million.

### When to Use SYNRIX vs Vector Databases

**Use SYNRIX when:**
- ✅ You have **structured, semantic data** (code patterns, learning outcomes, constraints)
- ✅ You need **prefix-based queries** (find all `ISA_*` nodes, all `PATTERN_*` nodes)
- ✅ You need **deterministic results** (same query = same result)
- ✅ You need **local-first** architecture (no cloud dependency)
- ✅ You need **bounded local latency** (real-time agent memory)
- ✅ You have **hierarchical data** (namespaces, categories, domains)

**Use Vector Databases when:**
- ✅ You need **fuzzy similarity search** (find documents similar to a query)
- ✅ You have **unstructured text** (documents, embeddings, natural language)
- ✅ You need **semantic similarity** (cosine similarity, nearest neighbors)
- ✅ You're doing **RAG with embeddings** (retrieve similar document chunks)
- ✅ You need **approximate matching** (typos, variations, paraphrasing)

**Use Both (Hybrid Approach):**
- Use **SYNRIX** for structured agent memory (patterns, constraints, learnings)
- Use **Vector DB** for document retrieval (RAG, similarity search)
- Best of both worlds: fast structured queries + fuzzy document search

**Key Insight:** SYNRIX excels at **structured semantic memory**, while vector DBs excel at **unstructured similarity search**. They complement each other.

---

## 📦 SDK vs Engine

**This repository contains the Python SDK (client library) - fully open source (MIT License).**

- ✅ **SDK**: MIT License (open source, fully auditable)
- ✅ **Examples**: MIT License (open source)
- ✅ **Documentation**: Open source

**The engine is distributed separately:**
- **Evaluation Engine**: Free local evaluation (dev/testing)
- **Production Engine**: Commercial license (separate distribution)

**Why this matters:**
- The SDK is a **contract** that defines what operations are legal
- You can **audit** the SDK code to understand the system
- The SDK gets you past code review and procurement gates
- The engine provides the actual storage and query capabilities

**You need BOTH:**
1. **SDK** (this repo) - The Python client library
2. **Engine** (separate download) - The actual semantic memory server

---

## 🎯 Use Cases

> **📖 For detailed use case guidance, see [`USE_CASES.md`](USE_CASES.md)**

### AI Agent Memory (Primary Use Case)
Store what the agent learns and recall patterns using **namespace prefixes**:

```python
# Agents use namespace prefixes to store anything
client.add_node("AGENT_123:LEARNING_PATTERN:error_handling", "Use try/except blocks")
client.add_node("AGENT_123:TEMP_cache", "cached data")
client.add_node("AGENT_123:USER_DATA:profile", "user profile")

# Query by namespace (finds all agent nodes)
results = client.query_prefix("AGENT_123:", limit=100)

# Query specific prefix in namespace
results = client.query_prefix("AGENT_123:LEARNING_PATTERN:", limit=10)
```

**Why namespace prefixes?**
- Agents can store **anything** (TEMP_, RANDOM_, arbitrary prefixes)
- Prefix explosion **mitigated** (prefix growth is contained via namespace discipline)
- O(k) performance **maintained** (query by namespace prefix)

**Supported namespace prefixes:**
- `AGENT_*:` - For AI agents
- `USER_*:` - For user data
- `SESSION_*:` - For session data
- `TENANT_*:` - For multi-tenant systems

### Code Pattern Storage
Store discovered code patterns with semantic prefixes:

```python
# System nodes use strict semantic prefixes
client.add_node("ISA_ADD", "Addition operation")
client.add_node("PATTERN_LOOP", "For loop pattern")
client.add_node("CONSTRAINT_NO_REGEX", "No regex constraint")

# Query by prefix
results = client.query_prefix("ISA_", limit=100)
results = client.query_prefix("PATTERN_", limit=100)
```

### Learning Systems
Accumulate knowledge over time:

```python
# Session 1: Learn a pattern
client.add_node("LEARNING_PATTERN:python:missing_colon", "Add ':' after if/for/while")

# Session 2: Recall the pattern
results = client.query_prefix("LEARNING_PATTERN:python:", limit=10)
```

### Structured Knowledge Base
Store hierarchical, semantic knowledge:

```python
# Domain-organized knowledge
client.add_node("DOMAIN_MATH:algebra:quadratic", "ax² + bx + c = 0")
client.add_node("DOMAIN_MATH:geometry:circle", "πr²")

# Query by domain
results = client.query_prefix("DOMAIN_MATH:", limit=100)
results = client.query_prefix("DOMAIN_MATH:algebra:", limit=10)
```

### ⚠️ NOT Recommended: Unstructured Document Search

**For RAG/document retrieval, use a vector database instead:**

```python
# ❌ Don't use SYNRIX for this
client.add_node("DOC:article1", "Long unstructured article text...")
results = client.query_prefix("DOC:")  # Not useful for similarity search

# ✅ Use vector database for this
from langchain.vectorstores import Qdrant
vectorstore = Qdrant.from_documents(documents, embeddings)
results = vectorstore.similarity_search("query", k=5)  # Fuzzy matching
```

**Why?** SYNRIX uses **prefix queries** (exact matches), not **similarity search** (fuzzy matching). For documents, you need similarity search.

---

## 📚 Examples

### Beginner-Friendly

**Start here:** [`examples/first_knowledge_graph.py`](examples/first_knowledge_graph.py)
- Perfect for complete beginners
- Interactive, step-by-step tutorial
- No server needed (uses mock client)
- Explains concepts as you go

**Quick API demo:** [`examples/quickstart.py`](examples/quickstart.py)
- Shows the API in action
- ~30 seconds to run
- Uses mock client (no server required)

**Full example:** [`examples/hello_memory.py`](examples/hello_memory.py)
- Complete workflow
- Error handling
- Real-world usage patterns
- Requires engine running

See [`examples/README.md`](examples/README.md) for all examples.

---

## 🔧 Installation

**Requirements:** Python 3.8+. **Linux (raw_backend / direct engine):** `libsynrix.so` (build with `./build/linux/build.sh` from repo root); optional runtime deps (e.g. OpenSSL .so) if the engine uses them.

### From PyPI (when available)
```bash
pip install synrix
```

### From Source
```bash
git clone https://github.com/synrix/synrix-python-sdk
cd synrix-python-sdk
pip install -e .
```

### Development Installation
```bash
git clone https://github.com/synrix/synrix-python-sdk
cd synrix-python-sdk
pip install -e ".[dev]"
```

---

## 💻 Usage

### Basic Operations

```python
from synrix import SynrixClient

client = SynrixClient(host="localhost", port=6334)

# Create a collection
client.create_collection("my_collection")

# Add a node
node_id = client.add_node(
    "ISA_ADD",
    "Addition operation",
    collection="my_collection"
)

# Query by prefix
results = client.query_prefix("ISA_", collection="my_collection")

# Get node by ID
node = client.get_node(node_id, collection="my_collection")

# Update a node
client.update_node(node_id, "Updated content", collection="my_collection")

# Delete a node
client.delete_node(node_id, collection="my_collection")
```

### Using Mock Client (No Server Required)

For testing or examples, use the mock client:

```python
from synrix import SynrixMockClient

client = SynrixMockClient()

# Same API, but no server needed
client.create_collection("test")
client.add_node("TEST_NODE", "test data", collection="test")
results = client.query_prefix("TEST_", collection="test")
```

---

## 🐛 Troubleshooting

### Library not found on Linux (raw_backend / libsynrix.so)
Set **`LD_LIBRARY_PATH`** or **`SYNRIX_LIB_PATH`** to the directory containing `libsynrix.so` before running Python. Example: `export LD_LIBRARY_PATH=/path/to/build/linux/out:$LD_LIBRARY_PATH`.

### "Connection refused" Error

**Problem:** The SDK can't connect to the engine.

**Solution:**
1. Make sure the engine is running:
   ```bash
   ./synrix-server-evaluation-0.1.0-linux-x86_64 --port 6334
   ```
2. Check the host and port match:
   ```python
   client = SynrixClient(host="localhost", port=6334)
   ```
3. Verify the engine is listening:
   ```bash
   curl http://localhost:6334/health
   ```

### "Collection not found" Error

**Problem:** Trying to use a collection that doesn't exist.

**Solution:**
```python
# Create the collection first
client.create_collection("my_collection")
```

### Engine Not Starting

**Problem:** The engine binary won't run.

**Solution:**
1. Make sure it's executable: `chmod +x synrix-server-0.1.0-linux-x86_64`
2. Check system compatibility (Linux x86_64)
3. Check for missing dependencies (usually none required)

---

## 📖 API Reference

### SynrixClient

```python
client = SynrixClient(host="localhost", port=6334, timeout=30)
```

**Methods:**
- `create_collection(name: str) -> None`
- `add_node(name: str, data: str, collection: str) -> int`
- `get_node(node_id: int, collection: str) -> Dict`
- `update_node(node_id: int, data: str, collection: str) -> None`
- `delete_node(node_id: int, collection: str) -> None`
- `query_prefix(prefix: str, collection: str, limit: int = 100) -> List[Dict]`
- `close() -> None`

See the [full API documentation](https://github.com/synrix/synrix-python-sdk#api-reference) for details.

---

## 🤝 Contributing

Contributions are welcome! This is an open-source SDK (MIT License).

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines.

---

## 📄 License

This SDK is licensed under the **MIT License** - see [`LICENSE`](LICENSE) for details.

**Note:** The SYNRIX engine is distributed separately under a commercial license. The SDK (this repository) is fully open source.

---

## 🔗 Links

- **Engine Download**: https://releases.synrix.dev
- **Documentation**: https://github.com/synrix/synrix-python-sdk#readme
- **Issues**: https://github.com/synrix/synrix-python-sdk/issues
- **Releases**: https://github.com/synrix/synrix-python-sdk/releases

---

## ❓ FAQ

### Do I need the engine to use the SDK?

**Yes.** The SDK is just the client library. You need the engine running to actually store and query data.

### Is the engine open source?

The engine is **source-available** with a commercial license. The SDK (this repo) is fully open source (MIT).

### Can I use the SDK without the engine?

You can use `SynrixMockClient()` for testing and examples, but for real usage, you need the engine.

### Where do I download the engine?

Download from: https://releases.synrix.dev

### Is there a free version of the engine?

Yes! The evaluation engine (`synrix-server-evaluation-*`) is free for local development and testing.

### What's the difference between evaluation and production?

- **Evaluation**: Free, local-only, hard limits (25k nodes free tier)
- **Production**: Commercial license, no limits, support & SLA
- ⚠️ **Note:** Evaluation mode enforces limits to prevent misuse and is not representative of production-scale performance.

### Is SYNRIX a general-purpose database?

**No.** SYNRIX is optimized for structured semantic memory and agent workloads. If you need:
- **Flexible schemas** → Use a traditional database (PostgreSQL, MongoDB)
- **Ad-hoc queries** → Use a traditional database
- **Document similarity search** → Use a vector database (Pinecone, Qdrant)
- **Graph traversal** → Use a graph database (Neo4j, ArangoDB)

SYNRIX is specifically designed for **agent memory** and **structured semantic data** with prefix-based queries.

---

## 🙏 Acknowledgments

SYNRIX is built with a focus on:
- **Mechanical sympathy** - Works with hardware, not against it
- **Semantic over syntax** - Meaning over structure
- **Zero vendor lock-in** - Your data, your machine
- **Deterministic behavior** - Predictable, reliable results

---

**Questions?** Open an issue or check the [documentation](https://github.com/synrix/synrix-python-sdk#readme).
