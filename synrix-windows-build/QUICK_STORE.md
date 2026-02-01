# Quick Store Commands for Synrix Lattice

## One-Liner Template (with explicit save)

```bash
python -c "import sys, json; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False); b.add_node('PREFIX:name', 'data', node_type); b.save(); b.checkpoint(); print('Stored'); b.close()"
```

## Node Types
- `1` = TASK/WORK
- `2` = CONSTRAINT
- `3` = ISA (concept)
- `4` = FAILURE
- `5` = PATTERN/LEARNING

## Examples

### Store a Pattern (with explicit save)
```bash
python -c "import sys, json; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False); b.add_node('PATTERN:my_pattern', json.dumps({'title': 'My Pattern', 'steps': ['step1', 'step2']}), 5); b.save(); b.checkpoint(); print('Stored pattern'); b.close()"
```

### Store a Constraint
```bash
python -c "import sys, json; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False); b.add_node('CONSTRAINT:my_constraint', json.dumps({'rule': 'Always do X', 'reason': 'Because Y'}), 2); b.save(); b.checkpoint(); print('Stored constraint'); b.close()"
```

### Store Work/Task
```bash
python -c "import sys, json; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False); b.add_node('WORK:task_name', json.dumps({'description': 'Task description', 'steps': ['step1', 'step2']}), 1); b.save(); b.checkpoint(); print('Stored work'); b.close()"
```

### Store Simple Text (No JSON)
```bash
python -c "import sys; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False); b.add_node('PATTERN:simple', 'Just plain text data', 5); b.save(); b.checkpoint(); print('Stored'); b.close()"
```

## Important Notes

1. **Lattice Location**: The lattice is stored at `lattice/cursor_ai_memory.lattice` (relative to project root). This is fine - NOT a temp folder.

2. **Explicit Save**: Always call `b.save()` and `b.checkpoint()` before `b.close()` to ensure data is persisted.

3. **WAL Safety**: Even if you see "Failed to rename temp file" error, your data is safe in the WAL (Write-Ahead Log). The checkpoint applies WAL entries to the main file.

4. **Windows File Rename**: The rename error on Windows is a known issue with atomic file operations, but data is still persisted via WAL.

## Multi-Line Version (Easier to Edit)

Save this as `quick_store.py`:
```python
import sys, os, json
sys.path.insert(0, 'python-sdk')
from synrix.raw_backend import RawSynrixBackend

b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False)

# Edit these:
name = 'PATTERN:my_pattern'
data = json.dumps({'title': 'My Pattern', 'description': 'What it does'})
node_type = 5  # 1=WORK, 2=CONSTRAINT, 5=PATTERN

b.add_node(name, data, node_type)
b.save()  # Explicit save
b.checkpoint()  # Apply WAL to main file
print(f'Stored: {name}')
b.close()
```

Then just edit and run:
```bash
python quick_store.py
```
