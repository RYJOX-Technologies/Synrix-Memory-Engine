# SYNRIX RAG SDK

Python SDK for retrieval-augmented generation (RAG) using SYNRIX as the memory backend.

## What this is
This SDK provides RAG primitives (embeddings, search, persistence) backed by SYNRIX.
It requires a SYNRIX engine: use the Agent Memory SDK with the engine DLL for local Windows,
or a running Synrix server for other setups.

For internal distribution we use the free tier (100k node limit) and provide
upgrade instructions when the limit is reached.

## Requirements
- Windows 10+ (or Linux/macOS with Synrix server)
- Python 3.8+
- SYNRIX engine (DLL package or server)
- Optional: `sentence-transformers` / `torch` for local embeddings; or `openai` / `cohere` for cloud embeddings

## Quick start (Windows, local DLL)

1) Install the Agent Memory SDK and provide the SYNRIX DLL (see `../agent-memory-sdk/README.md`).

2) Install this SDK
```bash
cd synrix-rag-sdk
pip install -r requirements.txt
pip install -e .
```

3) Use RAG
```python
from synrix_rag import rag_memory, embeddings

# Configure embeddings (local or API), then use rag_memory with your Synrix backend.
# See synrix_rag/ for modules: rag_memory, search, embeddings, config.
```

## Free tier limit
When the 100k node limit is reached, the backend raises an error with upgrade
instructions. Use a paid tier to remove the limit.

## License
See [LICENSE](LICENSE). SDK code is MIT; engine binaries are proprietary.
