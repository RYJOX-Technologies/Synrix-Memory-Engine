# Drop-In Replacement Demo - What You Need

## ✅ What's Already Done

1. ✅ Demo script created (`vc_demo_drop_in_replacement.py`)
2. ✅ SYNRIX Qdrant-compatible server exists (`synrix-server-evaluation`)
3. ✅ LangChain integration exists (uses Qdrant REST API)

## 🔧 What You Need to Do

### 1. Verify SYNRIX Server is Built

```bash
cd /mnt/nvme/aion-omega/NebulOS-Scaffolding/integrations/qdrant_mimic
ls -la synrix-server-evaluation
```

If it doesn't exist:
```bash
make
```

### 2. Install LangChain (if not already installed)

```bash
pip install langchain langchain-community qdrant-client
```

### 3. (Optional) Install Qdrant for Side-by-Side Comparison

If you want to show Qdrant vs SYNRIX:

```bash
cd /mnt/nvme/aion-omega/NebulOS-Scaffolding/integrations/qdrant_mimic
./start_qdrant.sh
```

Or download manually:
```bash
mkdir -p /tmp/qdrant
cd /tmp/qdrant
# For ARM64 (Jetson):
wget https://github.com/qdrant/qdrant/releases/latest/download/qdrant-aarch64-unknown-linux-musl.tar.gz
tar -xzf qdrant-*.tar.gz
./qdrant  # Runs on port 6333
```

### 4. Test the Demo

```bash
cd /mnt/nvme/aion-omega/NebulOS-Scaffolding/python-sdk/examples
python3 vc_demo_drop_in_replacement.py
```

## 🎯 What the Demo Shows

1. **Current LangChain code** using Qdrant:
   ```python
   vectorstore = Qdrant.from_texts(
       texts=documents,
       embedding=embeddings,
       url="http://localhost:6333"  # Qdrant
   )
   ```

2. **The migration** (one line change):
   ```python
   vectorstore = Qdrant.from_texts(
       texts=documents,
       embedding=embeddings,
       url="http://localhost:6334"  # SYNRIX ← Changed this
   )
   ```

3. **Same code, same behavior, lower latency**

## 🚨 Potential Issues & Fixes

### Issue: "SYNRIX server not found"
**Fix:** Build the server:
```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
make
```

### Issue: "LangChain not installed"
**Fix:** Install dependencies:
```bash
pip install langchain langchain-community qdrant-client
```

### Issue: "Connection refused" on port 6334
**Fix:** Check if server started. The demo should start it automatically, but if it fails:
```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
./synrix-server-evaluation --port 6334 --lattice-path ~/.synrix_demo.lattice
```

### Issue: Qdrant not running (port 6333)
**Status:** This is OK! The demo works without Qdrant - it will just show SYNRIX side only.

## 📊 Expected Output

The demo should show:
1. Current LangChain code (Qdrant)
2. The one-line migration
3. Same code running with SYNRIX
4. Performance comparison (if Qdrant is running)

## 🎬 For VC Meeting

**Key talking points:**
1. "This is your current LangChain app" (show Qdrant code)
2. "Change one line" (show URL change)
3. "Same app, same behavior" (run it)
4. "Lower latency" (show performance)
5. **"This is the entire migration."**

**Infra buyers love this because:**
- Zero refactor risk
- Same codebase
- Drop-in replacement
- Easy to test
- Easy to rollback
