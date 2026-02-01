"""
SYNRIX Local RAG SDK

A complete RAG (Retrieval-Augmented Generation) SDK for local document storage
and semantic search using SYNRIX.

Features:
- Document storage with embeddings
- Semantic search
- Context retrieval for LLMs
- Persistent local storage
- Unlimited nodes (with paid tiers)
- Embedding caching
- Collection management

Usage:
    from synrix_rag import RAGMemory
    
    rag = RAGMemory(collection_name="my_docs")
    rag.add_document(text="Your document", metadata={})
    results = rag.search("your query")
    context = rag.get_context("your query")
"""

from .rag_memory import RAGMemory
from .embeddings import generate_embedding, batch_embeddings
from .config import get_api_key, create_config_file, load_config
from .licensing import LicenseManager, PricingTier, get_license_manager

__version__ = "1.0.0"

# Check if SYNRIX is available
try:
    from synrix.ai_memory import get_ai_memory
    _SYNRIX_AVAILABLE = True
except ImportError:
    _SYNRIX_AVAILABLE = False
    import warnings
    warnings.warn(
        "SYNRIX not found. Please install SYNRIX. "
        "See https://ryjoxtechnologies.com for installation instructions. "
        "SDK will use in-memory fallback (data will not persist).",
        UserWarning
    )
__all__ = [
    "RAGMemory",
    "generate_embedding",
    "batch_embeddings",
    "get_api_key",
    "create_config_file",
    "load_config",
    "LicenseManager",
    "PricingTier",
    "get_license_manager",
]
