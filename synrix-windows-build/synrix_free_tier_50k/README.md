# SYNRIX Free Tier - AI Agent Memory System

**Version:** Free Tier (50,000 node limit)  
**Platform:** Windows (with Linux/macOS support)  
**License:** See LICENSE file

## Quick Start

### Installation

1. **Copy the package** to your project directory
2. **Add to Python path:**

```python
import sys
sys.path.insert(0, '/path/to/synrix_free_tier_50k')
```

Or install as a package:

```bash
cd synrix_free_tier_50k
pip install -e .
```

### Basic Usage

```python
from synrix.ai_memory import get_ai_memory

# Get AI memory instance
memory = get_ai_memory()

# Store information
memory.add("PROJECT:architecture", "Microservices with REST APIs")
memory.add("FIX:bug_123", "Fixed null pointer exception in parser")

# Query by prefix
results = memory.query("PROJECT:")
for r in results:
    print(f"{r['name']}: {r['data']}")

# Get node count
count = memory.count()  # Returns current node count
```

## Features

- ✅ **50,000 node limit** (free tier)
- ✅ **O(1) lookups** (~131.5 ns)
- ✅ **O(k) queries** (~10-100 μs)
- ✅ **Persistent storage** (survives restarts)
- ✅ **Windows-native** (no scripts, no environment variables)
- ✅ **Binary data support** (embeddings, images, encrypted data)
- ✅ **WAL (Write-Ahead Log)** for crash recovery

## For AI Agents

SYNRIX is designed specifically for AI agents that need persistent, fast memory.

### Why SYNRIX for AI Agents?

1. **Persistent Memory**: Survives across sessions, crashes, reboots
2. **Fast Access**: O(1) lookups, O(k) queries (scales with results, not data size)
3. **Data-Agnostic**: Store text, binary, structured, or unstructured data
4. **Privacy-Preserving**: Local-only storage, no cloud dependency
5. **Cross-Platform**: Works on Windows, Linux, macOS

### Common Use Cases

- **Coding Assistants**: Remember project context, fixes, preferences
- **Multi-Agent Systems**: Shared memory across agents
- **Long-Running Agents**: Persistent state across restarts
- **Learning Agents**: Accumulate knowledge over time
- **Specialized Agents**: Domain-specific knowledge bases

See `AI_AGENT_GUIDE.md` for comprehensive examples.

## API Reference

### AIMemory Class

```python
from synrix.ai_memory import AIMemory, get_ai_memory

# Get instance (recommended)
memory = get_ai_memory()

# Or create custom instance
memory = AIMemory(lattice_path="custom.lattice")
```

#### Methods

**`add(name: str, data: str) -> Optional[int]`**
- Add a memory to the lattice
- Returns node ID if successful, None otherwise
- Example: `memory.add("FIX:bug_123", "Fixed null pointer")`

**`get(node_id: int) -> Optional[Dict]`**
- Get a memory by node ID (O(1) lookup)
- Returns: `{"id": int, "name": str, "data": str}`

**`query(prefix: str, limit: int = 100) -> List[Dict]`**
- Query memories by prefix (O(k) semantic search)
- Returns list of matching nodes
- Example: `memory.query("FIX:")` finds all fixes

**`count() -> int`**
- Count all memories in the lattice
- Returns total node count

**`list_all(prefix: str = "") -> List[Dict]`**
- List all memories (optionally filtered by prefix)
- Returns list of all nodes

### RawSynrixBackend (Advanced)

For direct access to the C backend:

```python
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("lattice.lattice", evaluation_mode=True)
node_id = backend.add_node("TASK:write_function", "code here")
node = backend.get_node(node_id)  # O(1) lookup
results = backend.find_by_prefix("TASK:")  # O(k) query
```

## Performance

- **O(1) Lookups**: ~131.5 ns per lookup
- **O(k) Queries**: ~10-100 μs (scales with results, not data size)
- **Write Performance**: ~292 ns per node addition
- **Throughput**: 3.4M nodes/sec (with WAL disabled)

## Limits

- **Free Tier**: 50,000 nodes maximum
- **Node Size**: 512 bytes per node (data field)
- **Binary Data**: 510 bytes per node (with 2-byte length header)
- **Chunked Storage**: Unlimited (for larger data)

## Error Handling

```python
from synrix.raw_backend import FreeTierLimitError

try:
    memory.add("KEY", "VALUE")
except FreeTierLimitError:
    print("Free tier limit reached (50k nodes)")
    print("Options: Delete nodes, upgrade to Pro tier")
```

## File Structure

```
synrix_free_tier_50k/
├── synrix/              # Python package
│   ├── __init__.py
│   ├── _native.py       # Windows-native DLL loader
│   ├── raw_backend.py   # Direct C backend interface
│   ├── ai_memory.py     # AI agent memory interface
│   ├── libsynrix.dll    # Main library (Windows)
│   ├── libgcc_s_seh-1.dll
│   ├── libstdc++-6.dll
│   └── libwinpthread-1.dll
├── README.md            # This file
├── AI_AGENT_GUIDE.md    # Comprehensive AI agent guide
└── LICENSE              # License file
```

## Requirements

- **Python**: 3.8+ (for `os.add_dll_directory()` on Windows)
- **Platform**: Windows 10+, Linux, macOS
- **No external dependencies**: Self-contained

## Troubleshooting

### DLL Not Found (Windows)

The package includes all necessary DLLs. If you see "DLL not found":
1. Ensure all DLLs are in the `synrix/` directory
2. Check that Python 3.8+ is installed (required for `os.add_dll_directory()`)

### Free Tier Limit Reached

If you hit the 50k node limit:
1. Delete unused nodes to free space
2. Upgrade to Pro tier for unlimited nodes
3. Use multiple lattice files for different purposes

## Support

- **Documentation**: See `AI_AGENT_GUIDE.md`
- **Issues**: Report on GitHub
- **Upgrade**: Visit synrix.io for Pro tier

## License

See LICENSE file for details.
