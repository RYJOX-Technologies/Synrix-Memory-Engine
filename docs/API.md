# Synrix API

## Python SDK

```python
from synrix.raw_backend import RawSynrixBackend

# Initialize
db = RawSynrixBackend("my_memory.lattice")

# Add node
db.add_node("LEARNING_PYTHON_ASYNCIO", "asyncio uses event loops", node_type=5)

# Query by prefix
results = db.find_by_prefix("LEARNING_PYTHON_", limit=10)
for r in results:
    print(r["name"], r["data"])

# Get single node by ID
node = db.get_node(node_id)
```

## Node Types

| Type | ID | Use |
|------|-----|-----|
| PRIMITIVE | 1 | Basic data |
| KERNEL | 2 | System |
| PATTERN | 3 | Code patterns |
| LEARNING | 5 | Learned knowledge |

## C API (Core)

```c
// Initialize
persistent_lattice_t lattice;
lattice_init(&lattice, "path.lattice", 100000, 0);
lattice_load(&lattice);

// Add node
uint64_t id = lattice_add_node(&lattice, LATTICE_NODE_LEARNING, "LEARNING_X", "data", 0);

// Query by prefix
uint64_t ids[64];
uint32_t n = lattice_find_nodes_by_name(&lattice, "LEARNING_", ids, 64);

// Get node
lattice_node_t node;
lattice_get_node_data(&lattice, ids[0], &node);

// Cleanup
lattice_cleanup(&lattice);
```

## WAL (Durable Writes)

```c
lattice_enable_wal(&lattice);
uint64_t id = lattice_add_node_with_wal(&lattice, type, name, data, 0);
lattice_wal_checkpoint(&lattice);  // Flush to disk
```

## Qdrant-Compatible REST API

```
POST /collections/{name}/points
POST /collections/{name}/points/search
GET  /collections/{name}
```

Start server: `./synrix-server-evaluation --port 6334`

## Common Patterns

- **RAG**: Store docs with `DOC_` prefix, retrieve by topic
- **Agent memory**: `LEARNING_` for learned facts, `EPISODIC_` for events
- **Code intelligence**: `ISA_`, `PATTERN_` for assembly/code patterns
