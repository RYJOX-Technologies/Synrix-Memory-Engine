"""
Embeddings integration for SYNRIX Local RAG SDK.

Supports local models (sentence-transformers), OpenAI, and Cohere embeddings.
Local models are the default - no API key required!
"""

import os
from typing import List, Optional, Union
import numpy as np

# Local embedding models (default, no API key needed)
try:
    from sentence_transformers import SentenceTransformer
    _SENTENCE_TRANSFORMERS_AVAILABLE = True
except ImportError:
    SentenceTransformer = None
    _SENTENCE_TRANSFORMERS_AVAILABLE = False

# Cloud embedding APIs (optional, requires API key)
try:
    from openai import OpenAI
except ImportError:
    OpenAI = None

try:
    import cohere
except ImportError:
    cohere = None

# Global cache for local models (avoid reloading)
_LOCAL_MODEL_CACHE = {}


def generate_embedding(
    text: str,
    model: str = "local",
    api_key: Optional[str] = None,
    **kwargs
) -> List[float]:
    """
    Generate an embedding for a single text.
    
    Args:
        text: The text to embed
        model: Embedding model to use:
            - "local" or "sentence-transformers" (default, no API key needed)
            - "openai" (requires API key)
            - "cohere" (requires API key)
            - Custom model name for sentence-transformers (e.g., "all-MiniLM-L6-v2")
        api_key: API key for cloud services (not needed for local models)
        **kwargs: Additional arguments for the embedding model
        
    Returns:
        List of floats representing the embedding vector
        
    Raises:
        ValueError: If model is not supported or API key is missing (for cloud models)
        ImportError: If required library is not installed
    """
    if not text or not text.strip():
        raise ValueError("Text cannot be empty")
    
    # Local models (default, no API key needed)
    if model in ("local", "sentence-transformers") or (
        model not in ("openai", "cohere") and _SENTENCE_TRANSFORMERS_AVAILABLE
    ):
        model_name = kwargs.get("model_name", "all-MiniLM-L6-v2")
        if model not in ("local", "sentence-transformers", "openai", "cohere"):
            # Custom model name provided
            model_name = model
        return _generate_local_embedding(text, model_name=model_name, **kwargs)
    elif model == "openai":
        return _generate_openai_embedding(text, api_key, **kwargs)
    elif model == "cohere":
        return _generate_cohere_embedding(text, api_key, **kwargs)
    else:
        raise ValueError(
            f"Unsupported model: {model}. Use 'local' (default), 'openai', or 'cohere'. "
            f"For custom sentence-transformers models, pass the model name directly."
        )


def batch_embeddings(
    texts: List[str],
    model: str = "local",
    api_key: Optional[str] = None,
    **kwargs
) -> List[List[float]]:
    """
    Generate embeddings for multiple texts in batch.
    
    Args:
        texts: List of texts to embed
        model: Embedding model to use:
            - "local" or "sentence-transformers" (default, no API key needed)
            - "openai" (requires API key)
            - "cohere" (requires API key)
            - Custom model name for sentence-transformers
        api_key: API key for cloud services (not needed for local models)
        **kwargs: Additional arguments for the embedding model
        
    Returns:
        List of embedding vectors
    """
    if not texts:
        return []
    
    # Local models (default, no API key needed)
    if model in ("local", "sentence-transformers") or (
        model not in ("openai", "cohere") and _SENTENCE_TRANSFORMERS_AVAILABLE
    ):
        model_name = kwargs.get("model_name", "all-MiniLM-L6-v2")
        if model not in ("local", "sentence-transformers", "openai", "cohere"):
            # Custom model name provided
            model_name = model
        return _batch_local_embeddings(texts, model_name=model_name, **kwargs)
    elif model == "openai":
        return _batch_openai_embeddings(texts, api_key, **kwargs)
    elif model == "cohere":
        return _batch_cohere_embeddings(texts, api_key, **kwargs)
    else:
        raise ValueError(
            f"Unsupported model: {model}. Use 'local' (default), 'openai', or 'cohere'."
        )


def _generate_local_embedding(
    text: str,
    model_name: str = "all-MiniLM-L6-v2",
    **kwargs
) -> List[float]:
    """
    Generate embedding using local sentence-transformers model (no API key needed).
    
    Args:
        text: Text to embed
        model_name: Sentence-transformers model name (default: "all-MiniLM-L6-v2")
        **kwargs: Additional arguments (device, normalize_embeddings, etc.)
        
    Returns:
        List of floats representing the embedding vector
    """
    if not _SENTENCE_TRANSFORMERS_AVAILABLE:
        raise ImportError(
            "sentence-transformers not installed. Install with: pip install sentence-transformers"
        )
    
    # Cache model instances to avoid reloading
    if model_name not in _LOCAL_MODEL_CACHE:
        device = kwargs.get("device", "cpu")
        _LOCAL_MODEL_CACHE[model_name] = SentenceTransformer(model_name, device=device)
    
    model = _LOCAL_MODEL_CACHE[model_name]
    
    try:
        # Generate embedding
        embedding = model.encode(
            text,
            normalize_embeddings=kwargs.get("normalize_embeddings", False),
            show_progress_bar=kwargs.get("show_progress_bar", False),
        )
        return embedding.tolist()
    except Exception as e:
        raise RuntimeError(f"Failed to generate local embedding: {str(e)}")


def _batch_local_embeddings(
    texts: List[str],
    model_name: str = "all-MiniLM-L6-v2",
    **kwargs
) -> List[List[float]]:
    """
    Generate embeddings for multiple texts using local sentence-transformers model.
    
    Args:
        texts: List of texts to embed
        model_name: Sentence-transformers model name (default: "all-MiniLM-L6-v2")
        **kwargs: Additional arguments (device, normalize_embeddings, batch_size, etc.)
        
    Returns:
        List of embedding vectors
    """
    if not _SENTENCE_TRANSFORMERS_AVAILABLE:
        raise ImportError(
            "sentence-transformers not installed. Install with: pip install sentence-transformers"
        )
    
    # Cache model instances to avoid reloading
    if model_name not in _LOCAL_MODEL_CACHE:
        device = kwargs.get("device", "cpu")
        _LOCAL_MODEL_CACHE[model_name] = SentenceTransformer(model_name, device=device)
    
    model = _LOCAL_MODEL_CACHE[model_name]
    
    try:
        # Generate embeddings in batch
        embeddings = model.encode(
            texts,
            normalize_embeddings=kwargs.get("normalize_embeddings", False),
            show_progress_bar=kwargs.get("show_progress_bar", False),
            batch_size=kwargs.get("batch_size", 32),
        )
        return [emb.tolist() for emb in embeddings]
    except Exception as e:
        raise RuntimeError(f"Failed to generate local embeddings: {str(e)}")


def _generate_openai_embedding(
    text: str,
    api_key: Optional[str] = None,
    model_name: str = "text-embedding-3-small",
    **kwargs
) -> List[float]:
    """Generate embedding using OpenAI API."""
    if OpenAI is None:
        raise ImportError(
            "OpenAI library not installed. Install with: pip install openai"
        )
    
    api_key = api_key or os.getenv("OPENAI_API_KEY")
    if not api_key:
        raise ValueError(
            "OpenAI API key not provided. Set OPENAI_API_KEY environment variable "
            "or pass api_key parameter."
        )
    
    client = OpenAI(api_key=api_key)
    
    try:
        response = client.embeddings.create(
            model=model_name,
            input=text,
            **kwargs
        )
        return response.data[0].embedding
    except Exception as e:
        raise RuntimeError(f"Failed to generate OpenAI embedding: {str(e)}")


def _batch_openai_embeddings(
    texts: List[str],
    api_key: Optional[str] = None,
    model_name: str = "text-embedding-3-small",
    **kwargs
) -> List[List[float]]:
    """Generate embeddings for multiple texts using OpenAI API."""
    if OpenAI is None:
        raise ImportError(
            "OpenAI library not installed. Install with: pip install openai"
        )
    
    api_key = api_key or os.getenv("OPENAI_API_KEY")
    if not api_key:
        raise ValueError(
            "OpenAI API key not provided. Set OPENAI_API_KEY environment variable "
            "or pass api_key parameter."
        )
    
    client = OpenAI(api_key=api_key)
    
    try:
        response = client.embeddings.create(
            model=model_name,
            input=texts,
            **kwargs
        )
        return [item.embedding for item in response.data]
    except Exception as e:
        raise RuntimeError(f"Failed to generate OpenAI embeddings: {str(e)}")


def _generate_cohere_embedding(
    text: str,
    api_key: Optional[str] = None,
    model_name: str = "embed-english-v3.0",
    input_type: str = "search_document",
    **kwargs
) -> List[float]:
    """Generate embedding using Cohere API."""
    if cohere is None:
        raise ImportError(
            "Cohere library not installed. Install with: pip install cohere"
        )
    
    api_key = api_key or os.getenv("COHERE_API_KEY")
    if not api_key:
        raise ValueError(
            "Cohere API key not provided. Set COHERE_API_KEY environment variable "
            "or pass api_key parameter."
        )
    
    client = cohere.Client(api_key=api_key)
    
    try:
        response = client.embed(
            texts=[text],
            model=model_name,
            input_type=input_type,
            **kwargs
        )
        return response.embeddings[0]
    except Exception as e:
        raise RuntimeError(f"Failed to generate Cohere embedding: {str(e)}")


def _batch_cohere_embeddings(
    texts: List[str],
    api_key: Optional[str] = None,
    model_name: str = "embed-english-v3.0",
    input_type: str = "search_document",
    **kwargs
) -> List[List[float]]:
    """Generate embeddings for multiple texts using Cohere API."""
    if cohere is None:
        raise ImportError(
            "Cohere library not installed. Install with: pip install cohere"
        )
    
    api_key = api_key or os.getenv("COHERE_API_KEY")
    if not api_key:
        raise ValueError(
            "Cohere API key not provided. Set COHERE_API_KEY environment variable "
            "or pass api_key parameter."
        )
    
    client = cohere.Client(api_key=api_key)
    
    try:
        response = client.embed(
            texts=texts,
            model=model_name,
            input_type=input_type,
            **kwargs
        )
        return response.embeddings
    except Exception as e:
        raise RuntimeError(f"Failed to generate Cohere embeddings: {str(e)}")
