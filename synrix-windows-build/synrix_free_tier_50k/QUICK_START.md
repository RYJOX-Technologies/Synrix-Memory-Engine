# Quick Start Guide

Get started with SYNRIX in 5 minutes!

## 1. Install

```python
import sys
sys.path.insert(0, '/path/to/synrix_free_tier_50k')
```

## 2. Basic Usage

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

## 3. Common Patterns

### Coding Assistant

```python
memory = get_ai_memory()

# Remember fixes
memory.add("FIX:bug_123", "Fixed null pointer in parser")

# Remember preferences
memory.add("PREF:code_style", "Use type hints")

# Recall later
fixes = memory.query("FIX:")
prefs = memory.query("PREF:")
```

### Multi-Agent System

```python
# All agents use same lattice file
memory = get_ai_memory("shared.lattice")

# Agent 1: Research
memory.add("RESEARCH:topic_123", research_data)

# Agent 2: Analysis (reads Agent 1's data)
results = memory.query("RESEARCH:topic_123")
memory.add("ANALYSIS:topic_123", analysis_results)
```

## 4. Next Steps

- Read `README.md` for full API reference
- See `AI_AGENT_GUIDE.md` for comprehensive examples
- Check `INSTALL.md` for installation options

## That's It!

You're ready to use SYNRIX for AI agent memory. Happy coding! 🚀
