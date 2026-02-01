# SYNRIX Drop-in Replacement Guide

## Quick Start: Use SYNRIX with Your LangChain App (5 Minutes)

### Step 1: Start SYNRIX Server

```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
./synrix-server-evaluation --port 6334 --lattice-path ~/.synrix_your_app.lattice
```

### Step 2: Change One Line in Your Code

**Before (Qdrant):**
```python
from langchain_community.vectorstores import Qdrant

vectorstore = Qdrant.from_texts(
    texts=your_documents,
    embedding=your_embeddings,
    url='http://localhost:6333'  # ← Qdrant
)
```

**After (SYNRIX):**
```python
from langchain_community.vectorstores import Qdrant

vectorstore = Qdrant.from_texts(
    texts=your_documents,
    embedding=your_embeddings,
    url='http://localhost:6334'  # ← SYNRIX (changed port)
)
```

**That's it. Same code, different port.**

### Step 3: Run Your App

Your existing LangChain code works immediately. No other changes needed.

---

## Migration Script

Use `migrate_to_synrix.py` to automatically update your code:

```bash
python3 migrate_to_synrix.py your_app.py
```

This will:
- Find Qdrant URLs in your code
- Change port from 6333 → 6334
- Backup your original file
- Show you the changes

---

## What Works

✅ Collection creation  
✅ Document upsert  
✅ Vector search  
✅ All LangChain Qdrant operations  

## What's Different

- **Port**: 6334 (instead of 6333)
- **Performance**: 40-50× faster (no network latency)
- **Cost**: Fixed (no per-query pricing)
- **Privacy**: Data stays local

---

## For Production

The evaluation server (free) has a 100k node limit. For production use with unlimited nodes, contact RYJOX Technologies.

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
# Make sure you're in the right directory
cd NebulOS-Scaffolding/integrations/qdrant_mimic
ls -la synrix-server-evaluation

# If missing, compile it:
make
```

**Collection creation fails?**
- Make sure SYNRIX server is running
- Check server logs for errors
- Verify port 6334 is accessible
