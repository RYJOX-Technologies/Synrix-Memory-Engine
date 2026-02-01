# SYNRIX - Unlimited Version

SYNRIX is a high-performance, persistent knowledge graph system designed for AI agents. This is the **unlimited version** with no node limits.

## Features

- **Unlimited nodes** - Store as many nodes as you need
- **O(1) lookups** - Instant access by node ID
- **O(k) queries** - Prefix-based semantic search scales with results, not data size
- **Persistent** - Data survives restarts via `.lattice` files
- **Fast** - Optimized for high-throughput operations
- **Data-agnostic** - Store text, binary, or chunked data

## Quick Start

### Installation

**Option 1: Double-click installer**
```
Double-click installer.bat
```

**Option 2: Manual install**
```bash
pip install -e .
```

**Option 3: Use directly**
```python
import sys
sys.path.insert(0, r'C:\synrix_unlimited')
from synrix.ai_memory import get_ai_memory
```

### Basic Usage

```python
from synrix.ai_memory import get_ai_memory

# Get memory instance
memory = get_ai_memory()

# Store information
memory.add("PROJECT:name", "My Project")
memory.add("FIX:bug_123", "Fixed null pointer")

# Query by prefix
results = memory.query("FIX:")
for r in results:
    print(f"{r['name']}: {r['data']}")

# Get count
count = memory.count()
print(f"Total nodes: {count}")
```

## API Reference

### AI Memory Interface

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

# Add a node
memory.add(name: str, data: str) -> bool

# Query by prefix
memory.query(prefix: str) -> List[Dict]

# Get node count
memory.count() -> int

# Get node by ID
memory.get(node_id: int) -> Optional[Dict]
```

### Raw Backend (Advanced)

```python
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend(
    lattice_path="data.lattice",
    max_nodes=100000,  # RAM cache size
    evaluation_mode=False  # Unlimited mode
)

# Add node
node_id = backend.add_node("name", "data", node_type=3)

# Get node
node = backend.get_node(node_id)

# Query by prefix
results = backend.find_by_prefix("prefix", limit=100)
```

## Performance

- **Lookups**: O(1) - Instant access by ID
- **Queries**: O(k) - Scales with number of results, not total data
- **Writes**: Batched for performance (checkpoint every 12.5k-50k entries)
- **Memory**: Configurable RAM cache (default: 100k nodes)

## File Structure

```
synrix_unlimited/
├── synrix/              # Python SDK
│   ├── __init__.py
│   ├── ai_memory.py     # AI memory interface
│   ├── raw_backend.py   # Raw backend
│   ├── libsynrix.dll    # Windows DLL (unlimited)
│   └── *.dll            # MinGW runtime DLLs
├── setup.py             # Package setup
├── installer.bat        # Windows installer
└── README.md           # This file
```

## Requirements

- **Python 3.8+**
- **Windows** (this package includes Windows DLLs)
- **No other dependencies** - Everything is included

## Differences from Free Tier

This unlimited version:
- [OK] **No node limits** - Store unlimited nodes
- [OK] **No evaluation mode** - Full production features
- [OK] **Full performance** - All optimizations enabled

## Support

For issues or questions, see the documentation files or contact the SYNRIX team.

---

**SYNRIX Unlimited** - Production-ready knowledge graph for AI agents.
