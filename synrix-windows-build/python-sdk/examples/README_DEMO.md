# SYNRIX Demo & Migration Tools

## For VC/Investor Demos

**`vc_demo_drop_in_replacement.py`** - The main demo showing SYNRIX as a drop-in replacement for Qdrant in LangChain applications.

```bash
python3 vc_demo_drop_in_replacement.py
```

This demonstrates:
- One-line migration (port 6333 → 6334)
- Same code, same behavior
- 40-50× faster performance (p50/p95 comparison)
- Fixed cost, data privacy, predictable latency

---

## For Real-World Use

### Option 1: Quick Start Script (Easiest)

```bash
./quick_start_synrix.sh
```

This will:
- Start SYNRIX server on port 6334
- Show you exactly what to change in your code
- Keep the server running until you press Ctrl+C

### Option 2: Manual Migration

1. **Start SYNRIX server:**
   ```bash
   cd NebulOS-Scaffolding/integrations/qdrant_mimic
   ./synrix-server-evaluation --port 6334 --lattice-path ~/.synrix_your_app.lattice
   ```

2. **Change one line in your code:**
   ```python
   # Before
   url='http://localhost:6333'  # Qdrant
   
   # After
   url='http://localhost:6334'  # SYNRIX
   ```

3. **Run your app** - no other changes needed!

### Option 3: Automated Migration

Use the migration helper to automatically update your code:

```bash
python3 migrate_to_synrix.py your_app.py
```

This will:
- Find Qdrant URLs in your code
- Change port 6333 → 6334
- Create a backup of your original file
- Show you the changes

### Option 4: Try the Example

See a working example with real documents:

```bash
python3 example_real_world_migration.py
```

---

## Files Overview

| File | Purpose |
|------|---------|
| `vc_demo_drop_in_replacement.py` | Main VC demo (drop-in replacement) |
| `quick_start_synrix.sh` | One-command server startup + instructions |
| `migrate_to_synrix.py` | Automated code migration tool |
| `example_real_world_migration.py` | Working example with real documents |
| `MIGRATION_GUIDE.md` | Complete migration documentation |

---

## What Works

✅ All LangChain Qdrant operations  
✅ Collection creation  
✅ Document upsert  
✅ Vector similarity search  
✅ Same API, same behavior  

## What's Different

- **Port**: 6334 (instead of 6333)
- **Performance**: 40-50× faster (no network latency)
- **Cost**: Fixed (no per-query pricing)
- **Privacy**: Data stays local

---

## Troubleshooting

**Server won't start?**
```bash
# Check if port is in use
lsof -i :6334

# Kill existing process if needed
pkill -f synrix-server-evaluation
```

**Can't find the server?**
```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
make  # Build the server
```

**Need help?**
- See `MIGRATION_GUIDE.md` for detailed instructions
- Check server logs: `/tmp/synrix_quickstart.log`

---

## For Production

The evaluation server (free) has a 100k node limit. For production use with unlimited nodes, contact RYJOX Technologies.
