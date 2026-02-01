# SYNRIX Python SDK

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python 3.8+](https://img.shields.io/badge/python-3.8+-blue.svg)](https://www.python.org/downloads/)

**Python SDK for SYNRIX - A local-first knowledge graph system for AI applications**

SYNRIX provides persistent semantic memory for AI systems, enabling them to remember, reason, and learn over time. This SDK is the Python client library that connects to the SYNRIX engine.

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

### 2. Download and Run the Engine

**⚠️ Important:** The SDK is just the client library. You need **both the SDK and the engine** to use SYNRIX.

**Download the engine binary:**
```bash
# Linux x86_64
wget https://releases.synrix.dev/synrix-server-0.1.0-linux-x86_64
chmod +x synrix-server-0.1.0-linux-x86_64

# Or download from: https://releases.synrix.dev
```

**Run the engine in evaluation mode (free, local-only):**
```bash
./synrix-server-0.1.0-linux-x86_64 --dev --port 6334
```

**Free Evaluation Engine:**
- ✅ Single node, local-only
- ✅ Hard limits (100k nodes, 1GB memory)
- ✅ Perfect for development and evaluation
- ✅ No signup required

### 3. Use the SDK

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

**That's it!** You now have a working SYNRIX knowledge graph.

---

## 📖 What is SYNRIX?

SYNRIX is a **local-first knowledge graph system** designed for AI applications. It provides:

- **Persistent Semantic Memory** - AI systems can remember what they've learned
- **O(k) Semantic Queries** - Prefix-based search that scales with results, not data
- **Local-First Architecture** - Everything runs on your machine, zero vendor lock-in
- **Sub-millisecond Latency** - Fast local access vs 50ms+ for cloud vector DBs
- **Deterministic Behavior** - Same query = same result, always

**Think of it as:**
- The **long-term memory** for AI agents
- A **semantic index** that scales with results, not data
- A **local-first** alternative to cloud vector databases

### Knowledge Graph vs Vector Database

| Feature | SYNRIX | Vector DBs |
|---------|--------|-----------|
| **Query Type** | Semantic prefix | Similarity search |
| **Performance** | O(k) where k = results | O(n) or O(log n) |
| **Latency** | Sub-10µs | 50ms+ |
| **Location** | Local | Cloud |
| **Vendor Lock-in** | None | Yes |
| **Structure** | Semantic, hierarchical | Flat, unstructured |

**Example:**
If you have 1 million nodes but only 100 match `ISA_*`, a SYNRIX query only scans those 100. A vector DB would need to compare against all 1 million.

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
2. **Engine** (separate download) - The actual knowledge graph server

---

## 🎯 Use Cases

### AI Agent Memory
Store what the agent learns and recall patterns:
```python
client.add_node("LEARNING_PATTERN:error_handling", "Use try/except blocks")
results = client.query_prefix("LEARNING_PATTERN:")
```

### RAG Knowledge Base
Store document chunks with semantic names:
```python
client.add_node("DOC:python:tutorial", document_content)
results = client.query_prefix("DOC:python:")
```

### Code Pattern Storage
Store discovered code patterns:
```python
client.add_node("PATTERN:python:async", "Use async/await for I/O")
results = client.query_prefix("PATTERN:python:")
```

### Learning Systems
Accumulate knowledge over time:
```python
# Session 1: Learn a pattern
client.add_node("ERROR:python:missing_colon", "Add ':' after if/for/while")

# Session 2: Recall the pattern
results = client.query_prefix("ERROR:python:")
```

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

### "Connection refused" Error

**Problem:** The SDK can't connect to the engine.

**Solution:**
1. Make sure the engine is running:
   ```bash
   ./synrix-server-0.1.0-linux-x86_64 --dev --port 6334
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

Yes! The evaluation engine (`--dev` flag) is free for local development and testing.

### What's the difference between evaluation and production?

- **Evaluation**: Free, local-only, hard limits (100k nodes, 1GB memory)
- **Production**: Commercial license, no limits, support & SLA

---

## 🙏 Acknowledgments

SYNRIX is built with a focus on:
- **Mechanical sympathy** - Works with hardware, not against it
- **Semantic over syntax** - Meaning over structure
- **Zero vendor lock-in** - Your data, your machine
- **Deterministic behavior** - Predictable, reliable results

---

**Questions?** Open an issue or check the [documentation](https://github.com/synrix/synrix-python-sdk#readme).
