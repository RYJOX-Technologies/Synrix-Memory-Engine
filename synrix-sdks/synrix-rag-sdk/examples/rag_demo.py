#!/usr/bin/env python3
"""
SYNRIX RAG Demo – showcase document ingestion and semantic search.

Demonstrates:
- Storing documents larger than 512 bytes (uses Synrix chunked storage automatically)
- Semantic search over the collection
- Context retrieval for LLM-style RAG

Run from repo root (with agent-memory-sdk + DLL available):
  pip install -e ../agent-memory-sdk
  pip install -e ..
  python examples/rag_demo.py

Or from this directory:
  pip install -e ..
  python rag_demo.py

Requires: synrix-rag-sdk, synrix (agent-memory-sdk), SYNRIX DLL. Local embeddings use sentence-transformers.
"""

import os
import sys

# Allow running from examples/ or from synrix-rag-sdk/
_script_dir = os.path.dirname(os.path.abspath(__file__))
_sdk_root = os.path.dirname(_script_dir)
if _sdk_root not in sys.path:
    sys.path.insert(0, _sdk_root)

def main():
    # Use a dedicated lattice for the demo (no clash with user data)
    demo_lattice = os.path.join(_script_dir, "demo_rag.lattice")
    if os.path.exists(demo_lattice):
        try:
            os.remove(demo_lattice)
        except Exception:
            pass
    print("SYNRIX RAG Demo")
    print("===============\n")

    # Create Synrix-backed memory (optional: use default if not provided)
    synrix_client = None
    try:
        from synrix.ai_memory import AIMemory
        synrix_client = AIMemory(lattice_path=demo_lattice)
        print("[OK] Using SYNRIX backend (documents persist, chunked storage for large payloads).\n")
    except ImportError as e:
        print("[WARN] synrix not installed. Install agent-memory-sdk and provide SYNRIX DLL for full demo.")
        print("       Using in-memory fallback (data will not persist).\n")
    except Exception as e:
        print(f"[WARN] SYNRIX init failed: {e}")
        print("       Using in-memory fallback.\n")

    # RAG memory with local embeddings (no API key)
    from synrix_rag import RAGMemory

    rag = RAGMemory(
        collection_name="synrix_rag_demo",
        embedding_model="local",  # sentence-transformers, no API key
        synrix_client=synrix_client,
    )

    # Documents deliberately longer than 512 bytes to show chunked storage
    docs = [
        {
            "text": (
                "Synrix is a high-performance memory engine designed for AI agents and robotics. "
                "It uses a fixed 512-byte node architecture for speed, with chunked storage for larger payloads. "
                "This document is intentionally long to demonstrate that RAG works with documents exceeding "
                "the single-node limit: the SDK stores them automatically using chunked storage, so you can "
                "build retrieval-augmented generation without worrying about payload size. "
                "Features include persistent lattices, semantic search, and configurable auto-save."
            ),
            "metadata": {"source": "synrix_overview", "topic": "architecture"},
        },
        {
            "text": (
                "To build RAG with Synrix you install the agent-memory SDK and the RAG SDK. "
                "Place the Synrix DLL in the same directory as the Python package or set SYNRIX_LIB_PATH. "
                "Then create a RAGMemory instance with your collection name and add documents with add_document. "
                "Use search() for semantic retrieval and get_context() to get a formatted context string for your LLM. "
                "Documents and embeddings are stored in the lattice; large documents use chunked storage under the hood."
            ),
            "metadata": {"source": "setup_guide", "topic": "rag"},
        },
        {
            "text": (
                "The Synrix engine supports atomic snapshot saves on Windows with proper file locking: "
                "it flushes and unmaps the memory-mapped file before replacing the lattice file. "
                "Auto-save is disabled by default at init so read-only or evaluation workloads do not trigger writes. "
                "Configure persistence after init with interval_nodes and interval_seconds as needed for your application."
            ),
            "metadata": {"source": "engine_notes", "topic": "persistence"},
        },
    ]

    print("1. Ingesting documents (some >512 bytes → chunked storage)...")
    doc_ids = []
    for i, doc in enumerate(docs):
        doc_id = rag.add_document(text=doc["text"], metadata=doc.get("metadata", {}))
        doc_ids.append(doc_id)
        size = len(doc["text"].encode("utf-8"))
        note = " (chunked)" if size > 511 else ""
        print(f"   Added doc {i+1}: id={doc_id[:8]}... size={size} bytes{note}")
    print()

    print("2. Semantic search: 'How do I set up RAG with Synrix?'")
    results = rag.search("How do I set up RAG with Synrix?", top_k=3)
    for i, r in enumerate(results, 1):
        score = r.get("score", 0)
        text_preview = (r.get("text", "") or "")[:120].replace("\n", " ")
        print(f"   [{i}] score={score:.4f} | {text_preview}...")
    print()

    print("3. Get context for LLM (top 2):")
    context = rag.get_context("How do I set up RAG with Synrix?", top_k=2)
    print(context[:500] + "..." if len(context) > 500 else context)
    print()

    print("4. List documents in collection:")
    listed = rag.list_documents(limit=10)
    print(f"   Total: {len(listed)} document(s)")
    for d in listed:
        print(f"   - {d.get('id', '')[:8]}... | text length {len(d.get('text', ''))}")
    print()

    print("Demo complete. RAG works with documents of any size; Synrix uses chunked storage for payloads >512 bytes.")
    if synrix_client and os.path.exists(demo_lattice):
        print(f"Lattice saved at: {demo_lattice}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
