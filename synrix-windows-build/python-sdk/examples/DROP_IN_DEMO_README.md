# The Drop-In Replacement Demo - Setup Guide

## What This Demo Shows

**THE demo for infrastructure buyers:**
- Show LangChain config pointing to Qdrant
- Change one line (host/port)
- Same app, same behavior, lower latency
- No refactor, no migration

**"This is the entire migration."**

## Prerequisites

### 1. Install LangChain and Qdrant Client

```bash
pip install langchain langchain-community qdrant-client
```

### 2. Build SYNRIX Server (if not already built)

```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
make
```

This creates:
- `synrix-server-evaluation` (evaluation API)
- `synrix_mimic_qdrant` (production server)

### 3. (Optional) Start Qdrant for Side-by-Side Comparison

If you want to show Qdrant vs SYNRIX side-by-side:

```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
./start_qdrant.sh
```

Or manually:
```bash
# Download Qdrant (if needed)
mkdir -p /tmp/qdrant
cd /tmp/qdrant
wget https://github.com/qdrant/qdrant/releases/latest/download/qdrant-$(uname -m)-unknown-linux-musl.tar.gz
tar -xzf qdrant-*.tar.gz
./qdrant
```

## Running the Demo

```bash
cd NebulOS-Scaffolding/python-sdk/examples
python3 vc_demo_drop_in_replacement.py
```

## What the Demo Does

1. **Shows current LangChain code** using Qdrant (port 6333)
2. **Shows the migration** - change `url` from `6333` to `6334`
3. **Runs same code** with SYNRIX (port 6334)
4. **Compares performance** side-by-side

## Key Points to Emphasize

1. **Same LangChain code** - no custom vectorstore needed
2. **Same API** - Qdrant REST API compatible
3. **One line change** - just the URL/port
4. **No refactor** - drop-in replacement
5. **Lower latency** - local execution

## Troubleshooting

### "LangChain not installed"
```bash
pip install langchain langchain-community qdrant-client
```

### "SYNRIX server not found"
```bash
cd NebulOS-Scaffolding/integrations/qdrant_mimic
make
```

### "Qdrant not running"
The demo works without Qdrant - it will just show SYNRIX side only.

### "Connection refused"
Make sure SYNRIX server started successfully. Check logs in the demo output.

## Next Steps

Once this demo works, you can:
1. Add more realistic documents
2. Use real embeddings (OpenAI, etc.)
3. Show more complex queries
4. Add persistence proof (restart server, data still there)
