# SYNRIX Free Tier - 100k Node Limit

**Version:** 0.1.0  
**Platform:** Windows (x86_64)  
**Node Limit:** 100,000 nodes (hard-coded)

## What is SYNRIX?

SYNRIX is a high-performance, persistent knowledge graph system designed for AI agents. It provides:

- **O(k) Semantic Queries** - Fast prefix-based queries that scale with results, not total data
- **O(1) Lookups** - Instant access by node ID
- **Persistent Storage** - Data survives restarts with crash-safe WAL (Write-Ahead Logging)
- **Tokenless Architecture** - No tokenization overhead, direct semantic reasoning
- **Data-Agnostic** - Store text, binary, or chunked data

## Quick Start

### Installation

**Option 1: Double-click installer (Easiest)**
1. Double-click `installer.bat`
2. Wait for installation
3. Done!

**Option 2: Manual installation**
```bash
pip install -e .
```

**Option 3: Use directly (No install)**
```python
import sys
sys.path.insert(0, r'C:\path\to\synrix_free_tier_100k')

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

### Basic Usage

```python
from synrix.ai_memory import get_ai_memory

# Get memory instance
memory = get_ai_memory()

# Store data
memory.add("TASK:fix_bug", "Fixed null pointer in user.py")
memory.add("LEARNING:python", "List comprehensions are faster than loops")

# Query by prefix
tasks = memory.query("TASK:")
learnings = memory.query("LEARNING:")

# Get specific node
node = memory.get(node_id)

# Count nodes
count = memory.count()  # Returns current node count
```

## Features

- ✅ **100,000 node limit** (hard-coded, enforced at DLL level)
- ✅ **Windows-native** - No shell scripts, direct DLL access
- ✅ **Crash-safe** - WAL ensures no data loss
- ✅ **Fast** - O(k) queries, O(1) lookups
- ✅ **Persistent** - Data stored in `~/.synrix_ai_memory.lattice`

## Node Limits

This free tier package has a **hard-coded 100,000 node limit**. When you reach the limit:

- New nodes cannot be added
- Existing nodes can still be queried and updated
- Delete nodes to free up space
- Upgrade to unlimited version for production use

## Documentation

- **START_HERE.md** - Quick start guide
- **AI_AGENT_GUIDE.md** - Guide for AI agent integration (if included)

## Requirements

- **Python:** 3.8 or higher
- **OS:** Windows 10/11 (x86_64)
- **Dependencies:** None (all included)

## Package Contents

```
synrix_free_tier_100k/
├── synrix/              # Python SDK
│   ├── ai_memory.py     # AI memory interface (recommended)
│   ├── raw_backend.py   # Raw backend (advanced)
│   ├── _native.py       # Windows DLL loader
│   └── *.dll            # Engine DLL + MinGW runtime
├── setup.py             # Package setup
├── installer.bat       # Windows installer
└── START_HERE.md       # Quick start guide
```

## Support

For issues, questions, or to upgrade to unlimited version:
- Visit: synrix.io
- Contact: support@synrix.io

## License

See LICENSE file (if included) or contact for licensing information.

---

**Note:** This is a free tier evaluation package with a 100,000 node limit. For production use or unlimited nodes, please contact us for the full version.
