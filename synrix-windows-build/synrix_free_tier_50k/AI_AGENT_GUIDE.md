# SYNRIX AI Agent Guide

Complete guide for using SYNRIX as persistent memory for AI agents.

## Table of Contents

1. [Why SYNRIX for AI Agents](#why-synrix-for-ai-agents)
2. [Quick Start](#quick-start)
3. [Common Patterns](#common-patterns)
4. [Advanced Usage](#advanced-usage)
5. [Best Practices](#best-practices)
6. [Examples](#examples)

---

## Why SYNRIX for AI Agents

### The Problem

AI agents need persistent memory, but traditional solutions have issues:

- **Databases**: Too slow, too complex, require servers
- **Files**: Unstructured, slow to parse, no semantic search
- **In-Memory**: Lost on restart, limited by RAM
- **Cloud APIs**: Privacy concerns, network latency, costs

### The Solution

SYNRIX provides:
- ✅ **Fast**: O(1) lookups (~131.5 ns), O(k) queries (~10-100 μs)
- ✅ **Persistent**: Survives crashes, reboots, updates
- ✅ **Local**: No cloud dependency, privacy-preserving
- ✅ **Structured**: Organized by prefixes, semantic search
- ✅ **Data-Agnostic**: Text, binary, structured, unstructured

---

## Quick Start

### Installation

```python
import sys
sys.path.insert(0, '/path/to/synrix_free_tier_50k')

from synrix.ai_memory import get_ai_memory
```

### Basic Operations

```python
# Get memory instance
memory = get_ai_memory()

# Store information
memory.add("PROJECT:name", "My Project")
memory.add("FIX:bug_123", "Fixed null pointer in parser")
memory.add("PREF:code_style", "Use type hints, prefer composition")

# Query by prefix
fixes = memory.query("FIX:")
for fix in fixes:
    print(f"{fix['name']}: {fix['data']}")

# Get count
count = memory.count()  # Current node count
```

---

## Common Patterns

### 1. Coding Assistant Memory

```python
from synrix.ai_memory import get_ai_memory

class CodingAssistant:
    def __init__(self):
        self.memory = get_ai_memory()
    
    def remember_fix(self, bug_id, fix_description):
        """Remember a bug fix"""
        self.memory.add(f"FIX:{bug_id}", fix_description)
    
    def remember_preference(self, key, value):
        """Remember user preference"""
        self.memory.add(f"PREF:{key}", value)
    
    def recall_fixes(self):
        """Get all remembered fixes"""
        return self.memory.query("FIX:")
    
    def recall_preferences(self):
        """Get all user preferences"""
        return self.memory.query("PREF:")
```

### 2. Multi-Agent System

```python
# Agent A: Research Agent
research_memory = get_ai_memory("research.lattice")
research_memory.add("RESEARCH:topic_123", research_data)

# Agent B: Analysis Agent (different process)
analysis_memory = get_ai_memory("research.lattice")  # Same file
results = analysis_memory.query("RESEARCH:topic_123")  # Instant access
analysis_memory.add("ANALYSIS:topic_123", analysis_results)

# Agent C: Synthesis Agent
synthesis_memory = get_ai_memory("research.lattice")
context = synthesis_memory.query("RESEARCH:") + synthesis_memory.query("ANALYSIS:")
synthesis_memory.add("SYNTHESIS:topic_123", synthesized_output)
```

### 3. Long-Running Agent

```python
class LongRunningAgent:
    def __init__(self):
        self.memory = get_ai_memory("agent_state.lattice")
    
    def save_checkpoint(self, state_data):
        """Save agent state"""
        timestamp = int(time.time())
        self.memory.add(f"CHECKPOINT:{timestamp}", json.dumps(state_data))
    
    def resume(self):
        """Resume from latest checkpoint"""
        checkpoints = self.memory.query("CHECKPOINT:")
        if not checkpoints:
            return None
        
        latest = max(checkpoints, key=lambda x: int(x['name'].split(':')[1]))
        return json.loads(latest['data'])
```

### 4. Learning Agent

```python
class LearningAgent:
    def __init__(self):
        self.memory = get_ai_memory()
    
    def learn(self, experience):
        """Store experience"""
        timestamp = int(time.time())
        self.memory.add(f"EXPERIENCE:{timestamp}", json.dumps(experience))
    
    def recall_similar(self, query):
        """Find similar experiences"""
        experiences = self.memory.query("EXPERIENCE:")
        # Filter by similarity (implement your similarity logic)
        similar = [e for e in experiences if self._is_similar(e, query)]
        return similar
    
    def update_knowledge(self, pattern, knowledge):
        """Update learned knowledge"""
        self.memory.add(f"KNOWLEDGE:{pattern}", knowledge)
```

---

## Advanced Usage

### Binary Data Storage

```python
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("lattice.lattice")

# Store embeddings
embedding = model.encode("text").tobytes()
backend.add_node_binary("EMBEDDING:doc_123", embedding)

# Store encrypted data
encrypted = encrypt(sensitive_data, key)
backend.add_node_binary("ENCRYPTED:user_data", encrypted)

# Store image thumbnails
thumbnail = resize_image(image, (64, 64)).tobytes()
backend.add_node_binary("IMAGE:thumbnail_123", thumbnail)
```

### Hierarchical Data

```python
# Create parent node
parent_id = memory.add("CATEGORY:animals", "")

# Create child nodes
child1_id = backend.add_node("ANIMAL:dog", "Canis lupus", parent_id=parent_id)
child2_id = backend.add_node("ANIMAL:cat", "Felis catus", parent_id=parent_id)

# Query children
children = backend.get_children(parent_id)
```

### Error Handling

```python
from synrix.raw_backend import FreeTierLimitError

try:
    memory.add("KEY", "VALUE")
except FreeTierLimitError:
    # Handle limit reached
    usage = memory.get_usage_info()
    print(f"Limit reached: {usage['current']}/{usage['limit']}")
    print("Options:")
    print("  1. Delete unused nodes")
    print("  2. Upgrade to Pro tier")
```

---

## Best Practices

### 1. Use Semantic Prefixes

**Good:**
```python
memory.add("FIX:bug_123", "Fixed null pointer")
memory.add("PREF:code_style", "Use type hints")
memory.add("ARCH:microservices", "REST APIs")
```

**Bad:**
```python
memory.add("node1", "data")  # No semantic meaning
memory.add("item", "value")  # Too generic
```

### 2. Organize by Category

Use consistent prefixes:
- `PROJECT:` - Project information
- `FIX:` - Bug fixes
- `PREF:` - User preferences
- `ARCH:` - Architecture decisions
- `PERF:` - Performance metrics
- `TASK:` - Tasks/todos

### 3. Handle Limits Gracefully

```python
def safe_add(memory, name, data):
    """Safely add node, handling limits"""
    try:
        return memory.add(name, data)
    except FreeTierLimitError:
        # Clean up old nodes
        old_nodes = memory.query("TEMP:")
        for node in old_nodes[:100]:  # Delete 100 old temp nodes
            memory.delete(node['id'])
        # Retry
        return memory.add(name, data)
```

### 4. Use Structured Data

```python
import json

# Store structured data as JSON
data = {
    "type": "fix",
    "bug_id": "123",
    "description": "Fixed null pointer",
    "timestamp": 1234567890
}
memory.add("FIX:bug_123", json.dumps(data))

# Retrieve and parse
fixes = memory.query("FIX:")
for fix in fixes:
    data = json.loads(fix['data'])
    print(f"Bug {data['bug_id']}: {data['description']}")
```

---

## Examples

### Example 1: Coding Assistant

```python
from synrix.ai_memory import get_ai_memory

class CodingAssistant:
    def __init__(self):
        self.memory = get_ai_memory()
    
    def start_session(self):
        """Initialize session"""
        # Load project context
        project_info = self.memory.query("PROJECT:")
        fixes = self.memory.query("FIX:")
        prefs = self.memory.query("PREF:")
        
        print(f"Project: {len(project_info)} items")
        print(f"Known fixes: {len(fixes)}")
        print(f"Preferences: {len(prefs)}")
    
    def remember_fix(self, bug_id, description):
        """Remember a bug fix"""
        self.memory.add(f"FIX:{bug_id}", description)
    
    def recall_fix(self, bug_id):
        """Recall a specific fix"""
        fixes = self.memory.query(f"FIX:{bug_id}")
        return fixes[0]['data'] if fixes else None
```

### Example 2: Research Agent

```python
class ResearchAgent:
    def __init__(self):
        self.memory = get_ai_memory("research.lattice")
    
    def store_finding(self, topic, finding):
        """Store research finding"""
        self.memory.add(f"RESEARCH:{topic}", finding)
    
    def synthesize(self, topic):
        """Synthesize all findings on a topic"""
        findings = self.memory.query(f"RESEARCH:{topic}")
        return "\n".join([f['data'] for f in findings])
```

### Example 3: Multi-Agent Workflow

```python
# Agent 1: Data Collection
collector = get_ai_memory("pipeline.lattice")
collector.add("RAW:dataset_1", raw_data)

# Agent 2: Data Processing
processor = get_ai_memory("pipeline.lattice")
raw = processor.query("RAW:dataset_1")
processed = clean_data(raw[0]['data'])
processor.add("PROCESSED:dataset_1", processed)

# Agent 3: Analysis
analyzer = get_ai_memory("pipeline.lattice")
processed = analyzer.query("PROCESSED:dataset_1")
insights = analyze(processed[0]['data'])
analyzer.add("ANALYSIS:dataset_1", insights)
```

---

## Performance Tips

1. **Use Prefixes**: O(k) queries scale with results, not data size
2. **Batch Operations**: Add multiple nodes in sequence (WAL batching)
3. **Limit Queries**: Use `limit` parameter to avoid loading all nodes
4. **Cache Results**: Query results are fast, but cache if needed

---

## Troubleshooting

### "Free Tier Limit Reached"

The 50k node limit has been reached. Options:
1. Delete unused nodes
2. Use multiple lattice files
3. Upgrade to Pro tier

### "DLL Not Found" (Windows)

Ensure all DLLs are in the `synrix/` directory:
- `libsynrix.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

### Slow Queries

- Use specific prefixes (not empty string)
- Use `limit` parameter
- Check node count (may need to delete old nodes)

---

## Next Steps

- See `README.md` for API reference
- Check examples in this guide
- Experiment with your use case
- Upgrade to Pro tier for unlimited nodes

---

**Questions?** See documentation or contact support.
