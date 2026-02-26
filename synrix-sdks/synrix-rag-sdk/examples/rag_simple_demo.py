#!/usr/bin/env python3
"""
SYNRIX RAG – public demo

Minimal RAG: ingest a few documents, run a semantic search, get context for an LLM.
Uses local embeddings (no API key). Synrix stores documents and vectors; large
documents use chunked storage automatically.

Run (with agent-memory-sdk + synrix-rag-sdk + SYNRIX_LIB_PATH set):
  pip install -e ../agent-memory-sdk && pip install -e ..
  python examples/rag_simple_demo.py
"""

import os
import sys

_script_dir = os.path.dirname(os.path.abspath(__file__))
_sdk_root = os.path.dirname(_script_dir)
if _sdk_root not in sys.path:
    sys.path.insert(0, _sdk_root)


def main():
    demo_lattice = os.path.join(_script_dir, "demo_rag_simple.lattice")
    if os.path.exists(demo_lattice):
        try:
            os.remove(demo_lattice)
        except Exception:
            pass

    print("SYNRIX RAG Simple Demo")
    print("=====================\n")

    synrix_client = None
    try:
        from synrix.ai_memory import AIMemory
        synrix_client = AIMemory(lattice_path=demo_lattice)
        print("[OK] SYNRIX backend ready.\n")
    except Exception as e:
        print("[WARN] SYNRIX init failed:", e)
        print("       Set SYNRIX_LIB_PATH. Using in-memory fallback.\n")

    try:
        from synrix_rag import RAGMemory
    except ImportError as e:
        print("ERROR: synrix_rag not installed or not on PYTHONPATH.")
        print("       From synrix-rag-sdk root run: pip install -e .")
        print("       Then: python examples/rag_simple_demo.py")
        return 1

    rag = RAGMemory(
        collection_name="demo",
        embedding_model="local",
        synrix_client=synrix_client,
    )

    # Two short documents
    rag.add_document(text="Synrix is a local memory engine for AI agents. It stores data by prefix and supports fast prefix queries.", metadata={"source": "intro"})
    rag.add_document(text="To use RAG with Synrix you add documents with add_document and retrieve with search or get_context for your LLM.", metadata={"source": "usage"})
    print("1. Ingested 2 documents.\n")

    print("2. Search: 'How do I use Synrix for RAG?'")
    results = rag.search("How do I use Synrix for RAG?", top_k=2)
    for i, r in enumerate(results, 1):
        print("   [{}] {}...".format(i, (r.get("text") or "")[:80]))
    print()

    print("3. Get context for LLM (top 1):")
    ctx = rag.get_context("How do I use Synrix for RAG?", top_k=1)
    print("   " + (ctx[:200] + "..." if len(ctx) > 200 else ctx))
    print("\nDone.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
