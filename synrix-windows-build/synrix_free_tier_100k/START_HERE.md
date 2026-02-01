# START HERE - SYNRIX Free Tier 100k

## Installation (30 seconds)

### Option A: Double-Click Installer (Easiest!)

1. **Double-click `installer.bat`** (or `install_synrix.exe` if available)
2. Wait for installation to complete
3. **Done!** Use SYNRIX from any Python script:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

### Option B: Quick Test (No Install)

1. Extract this package anywhere (e.g., `C:\synrix_free_tier_100k`)

2. In your Python script, add one line:

```python
import sys
sys.path.insert(0, r'C:\synrix_free_tier_100k')  # Change to your path

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

**Done!** That's it.

### Option C: Install as Package (Manual)

```bash
cd C:\synrix_free_tier_100k
pip install -e .
```

Then use it anywhere:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Quick Test

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:install", "SYNRIX works!")
print(f"Found {len(memory.query('TEST:'))} nodes")
```

## What You Get

- ✅ **100,000 nodes** (free tier - hard-coded limit)
- ✅ **Fast**: O(1) lookups, O(k) queries
- ✅ **Persistent**: Survives restarts
- ✅ **No setup**: Just extract and use
- ✅ **No dependencies**: Everything included

## Next Steps

- `QUICK_START.md` - 5-minute guide (if included)
- `AI_AGENT_GUIDE.md` - Comprehensive examples (if included)
- `README.md` - Full documentation (if included)

## Requirements

- Python 3.8+ (that's it!)

---

**That's it!** No scripts, no config, no hassle. Just extract and use.
