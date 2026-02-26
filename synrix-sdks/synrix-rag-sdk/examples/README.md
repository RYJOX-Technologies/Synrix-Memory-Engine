# SYNRIX RAG Examples

## Public demo

- **`rag_simple_demo.py`** – Minimal RAG: ingest 2 documents, run semantic search, get context for an LLM. Uses local embeddings. Safe to ship as the official RAG demo.

  ```bash
  pip install -e ../agent-memory-sdk && pip install -e ..
  set SYNRIX_LIB_PATH=<path to dir with libsynrix.dll>
  python examples/rag_simple_demo.py
  ```

---

## Knowledge-base demo (`rag_demo_kb.py`) – internal

**What people use RAG for:** Ingest a corpus (support articles, docs, wiki), then answer questions by retrieving relevant chunks.

This script uses a **built-in set of ~30 support/kb-style documents**. Many exceed 512 bytes so the C engine uses chunked storage (one call per doc from Python). No external files or downloads.

```bash
cd synrix-rag-sdk
pip install -e . && pip install -e ../agent-memory-sdk
pip install -r requirements.txt
python examples/rag_demo_kb.py
```

You’ll see ingest time, how many docs used chunked storage, and sample semantic queries (e.g. “How do I reset my password?”, “API rate limits”, “Refund policy”) returning the right doc. **Offline:** The first run downloads the embedding model (sentence-transformers/all-MiniLM-L6-v2); after that the demo runs without network.

---

## Small RAG Demo (`rag_demo.py`)

End-to-end demo: ingest documents (including ones larger than 512 bytes), run semantic search, and get context for an LLM. Shows that RAG works with Synrix’s fixed node size by using chunked storage automatically.

### Prerequisites

- Python 3.8+
- **Agent Memory SDK** (Synrix Python client) installed, with the Synrix engine DLL available  
  - From repo: `pip install -e ../agent-memory-sdk`  
  - Or install from PyPI if published; set `SYNRIX_LIB_PATH` to the directory or path of `libsynrix.dll` (Windows).
- **RAG SDK** installed: `pip install -e ..` (from `synrix-rag-sdk` root).
- Local embeddings: `sentence-transformers` (and optionally `torch`). Install with:  
  `pip install -r ../requirements.txt`

### Run

From **synrix-rag-sdk** directory:

```bash
cd synrix-rag-sdk
pip install -e .
pip install -e ../agent-memory-sdk   # if using local SDK tree
pip install -r requirements.txt
python examples/rag_demo.py
```

From **examples** directory:

```bash
cd synrix-rag-sdk/examples
pip install -e ..
python rag_demo.py
```

The demo uses a local lattice file `examples/demo_rag.lattice` (created and optionally removed each run). It does not touch your default Synrix lattice.

### What it does

1. **Ingest** – Adds 3 documents; at least one is longer than 512 bytes so the engine uses chunked storage.
2. **Search** – Runs a semantic query and prints top results with scores.
3. **Context** – Calls `get_context()` to produce a single string suitable for LLM context.
4. **List** – Lists documents in the collection to confirm storage.

If the Synrix DLL is not available, the script falls back to in-memory storage and still runs (data does not persist), so you can see the API flow.
