# SYNRIX Usage Guide - How to Use After Installation

## After Installation

Once you've run `installer.bat`, SYNRIX is installed and ready to use **from any Python script**.

## Direct Usage (No Setup Required)

### Step 1: Import

```python
from synrix.ai_memory import get_ai_memory
```

### Step 2: Get Memory Instance

```python
memory = get_ai_memory()
```

**That's it!** No configuration, no paths, no setup. It automatically:
- Creates a memory file at `~/.synrix_ai_memory.lattice` (in your home directory)
- Loads existing data if it exists
- Works immediately

### Step 3: Use It

```python
# Store data
memory.add("PROJECT:name", "My Project")
memory.add("USER:preference", "Dark mode")

# Query by prefix
results = memory.query("PROJECT:")
for r in results:
    print(f"{r['name']}: {r['data']}")

# Get total count
count = memory.count()
print(f"Total nodes: {count}")
```

## Complete Example

Create a file `my_script.py`:

```python
from synrix.ai_memory import get_ai_memory

# Get memory (auto-creates if needed)
memory = get_ai_memory()

# Store some data
memory.add("TASK:fix_bug_123", "Fixed null pointer in user.py")
memory.add("TASK:add_feature", "Added dark mode toggle")
memory.add("NOTE:important", "Remember to update docs")

# Query all tasks
tasks = memory.query("TASK:")
print(f"Found {len(tasks)} tasks:")
for task in tasks:
    print(f"  - {task['name']}: {task['data']}")

# Get specific node by ID
if tasks:
    node_id = tasks[0]['id']
    node = memory.get(node_id)
    print(f"\nFirst task: {node['data']}")

# Get total count
print(f"\nTotal nodes in memory: {memory.count()}")
```

Run it:
```bash
python my_script.py
```

**Output:**
```
Found 2 tasks:
  - TASK:fix_bug_123: Fixed null pointer in user.py
  - TASK:add_feature: Added dark mode toggle

First task: Fixed null pointer in user.py

Total nodes in memory: 3
```

## Where Data is Stored

By default, data is stored in:
- **Windows**: `C:\Users\YourName\.synrix_ai_memory.lattice`
- **Linux/Mac**: `~/.synrix_ai_memory.lattice`

This file:
- [OK] Persists across restarts
- [OK] Grows automatically
- [OK] Is portable (you can copy it)
- [OK] Is safe (atomic writes, crash-safe)

## Custom Lattice Path

If you want to use a different file:

```python
from synrix.ai_memory import get_ai_memory

# Use custom path
memory = get_ai_memory(lattice_path="C:/my_project/memory.lattice")
memory.add("TEST", "Data")
```

## API Reference

### `get_ai_memory(lattice_path=None)`
Get a memory instance. Creates the lattice file if it doesn't exist.

### `memory.add(name, data)`
Store a memory entry.
- `name`: Key/name (e.g., "PROJECT:name")
- `data`: Value/data
- Returns: `True` if successful

### `memory.query(prefix, limit=100)`
Query by prefix (semantic search).
- `prefix`: Prefix to search for (e.g., "PROJECT:")
- `limit`: Max results (default: 100)
- Returns: List of nodes matching the prefix

### `memory.get(node_id)`
Get a specific node by ID (O(1) lookup).
- `node_id`: Node ID
- Returns: Node dict or `None`

### `memory.count()`
Get total number of nodes.
- Returns: Integer count

## Real-World Examples

### Example 1: Project Notes

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

# Store project notes
memory.add("PROJECT:synrix:status", "In development")
memory.add("PROJECT:synrix:priority", "High")
memory.add("PROJECT:synrix:deadline", "2024-12-31")

# Retrieve all project notes
notes = memory.query("PROJECT:synrix:")
for note in notes:
    print(f"{note['name']}: {note['data']}")
```

### Example 2: Code Patterns

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

# Store code patterns
memory.add("PATTERN:async_handler", "Use asyncio.create_task()")
memory.add("PATTERN:error_handling", "Always use try/except")

# Find patterns
patterns = memory.query("PATTERN:")
for p in patterns:
    print(f"Pattern: {p['name']} = {p['data']}")
```

### Example 3: User Preferences

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

# Store preferences
memory.add("USER:theme", "dark")
memory.add("USER:language", "python")
memory.add("USER:editor", "vscode")

# Get preferences
prefs = memory.query("USER:")
prefs_dict = {p['name'].split(':')[1]: p['data'] for p in prefs}
print(f"Theme: {prefs_dict.get('theme')}")
```

## Key Points

1. **No server needed** - Works directly, no background process
2. **No configuration** - Just import and use
3. **Persistent** - Data survives restarts automatically
4. **Fast** - O(1) lookups, O(k) queries
5. **Unlimited** - No node limits in this version

## Troubleshooting

### "ModuleNotFoundError: No module named 'synrix'"
- Make sure you ran `installer.bat` or `pip install -e .`
- Check that Python can find the package: `python -c "import synrix"`

### "DLL not found"
- Make sure `libsynrix.dll` is in the `synrix/` package directory
- Check that MinGW runtime DLLs are present

### Data not persisting
- Check that you have write permissions in your home directory
- Verify the `.lattice` file is being created

---

**That's it!** After installation, just `import` and use. No setup, no configuration, no hassle.
