# WAL (Write-Ahead Log) Usage Guide

## How WAL Works in SYNRIX

**You're correct!** Everything should write to WAL first, then get pushed to the main file. Here's how it works:

### Architecture

1. **WAL is enabled by default** in the Python SDK (as of latest update)
2. **`lattice_add_node` automatically writes to WAL** if WAL is enabled (line 1982 in `persistent_lattice.c`)
3. **Nodes are stored in both memory AND WAL** when added
4. **`save()`** writes current memory state to main file
5. **`checkpoint()`** applies WAL entries to main file, then saves

### Flow

```
add_node() 
  → Adds to memory (lattice->nodes[])
  → Writes to WAL (if WAL enabled) ← YOU WERE RIGHT!
  → Returns node_id

save()
  → Writes current memory state to main file
  → Does NOT apply WAL entries

checkpoint()
  → Replays WAL entries to memory (lattice_recover_from_wal)
  → Saves memory to main file (lattice_save)
  → Marks WAL entries as applied (wal_checkpoint)
```

### Correct Usage Pattern

**For Python SDK (WAL enabled by default):**

```python
from synrix.raw_backend import RawSynrixBackend

b = RawSynrixBackend('lattice.lattice', max_nodes=1000000, evaluation_mode=False)

# Add nodes (automatically writes to WAL buffer)
for i in range(100):
    b.add_node(f'TEST:node_{i}', f'data_{i}', 5)

# Option 1: Manual flush (flushes WAL buffer to disk, doesn't apply to main file)
b.flush()  # Forces immediate flush of WAL buffer

# Option 2: Just save (writes memory to file, WAL entries remain uncheckpointed)
b.save()

# Option 3: Checkpoint (flushes WAL, applies entries to memory, then saves)
b.checkpoint()

# Option 4: Flush + Checkpoint (for maximum durability)
b.flush()   # Ensure WAL is on disk
b.checkpoint()  # Apply to main file

b.close()
```

### When to Use `flush()`, `save()`, and `checkpoint()`

- **`flush()`**: Forces immediate flush of WAL buffer to disk. Use when you need durability guarantees without applying entries to main file. Fast operation.
- **`save()`**: Fast, writes current memory state to main file. Use for frequent saves. WAL entries remain in buffer.
- **`checkpoint()`**: Slower, flushes WAL buffer, applies entries to memory, then saves to main file. Use for full durability guarantees.

**Best Practice**: 
- Use `flush()` when you need immediate durability (e.g., before critical operations)
- Use `save()` regularly for fast persistence
- Use `checkpoint()` periodically (or before closing) for full durability

### WAL Benefits

1. **Durability**: All writes go to WAL first (fast sequential write)
2. **Recovery**: WAL entries can be replayed after crash
3. **Performance**: Batched writes (checkpoint every 12.5k-50k entries)
4. **Safety**: No data loss even if main file write fails

### Current Status

✅ **WAL is now enabled by default** in Python SDK (as of latest update)
✅ **`lattice_add_node` writes to WAL automatically** (if WAL enabled)
✅ **`flush()` manually flushes WAL buffer** (for immediate durability)
✅ **`checkpoint()` flushes WAL, applies entries, then saves** (for full durability)

### Troubleshooting

**Issue**: Nodes not persisting after reload
- **Check**: Is WAL enabled? (Look for "[LATTICE-WAL] ✅ WAL enabled" in logs)
- **Fix**: Call `checkpoint()` after adding nodes, or ensure `save()` is called

**Issue**: WAL shows "0 entries replayed"
- **Check**: Are nodes being added? (WAL should have entries)
- **Fix**: Ensure WAL is enabled before adding nodes

**Issue**: Nodes in memory but not in file
- **Check**: Did you call `save()` or `checkpoint()`?
- **Fix**: Call `checkpoint()` to apply WAL entries and save

### Technical Details

- **WAL Location**: `{lattice_path}.wal` (e.g., `lattice.lattice.wal`)
- **WAL Format**: Binary format with operation type, node ID, and packed data
- **Checkpoint Interval**: 
  - Free tier: Every 12.5k entries (half of 25k limit)
  - Production: Every 50k entries
- **Auto-checkpoint**: Triggered when entry count reaches interval

### Code Reference

- **WAL Enable**: `lattice_enable_wal()` in `persistent_lattice.c` (line ~5290)
- **WAL Write**: `lattice_add_node()` writes to WAL (line 1982-2027)
- **WAL Flush**: `lattice_wal_flush()` (line ~5355) - manually flush buffer to disk
- **WAL Checkpoint**: `lattice_wal_checkpoint()` (line 5438) - flush, apply, save
- **WAL Recovery**: `lattice_recover_from_wal()` (line 5466)
