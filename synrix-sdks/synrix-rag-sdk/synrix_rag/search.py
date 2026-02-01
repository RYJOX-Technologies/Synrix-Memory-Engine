"""
Search functionality for SYNRIX Local RAG SDK.

Provides semantic search using SYNRIX's vector similarity search.
"""

from typing import List, Dict, Optional, Any
import sys
import os

# Add SYNRIX to path
_synrix_path = os.path.join(os.path.dirname(__file__), '..', '..', 'synrix_unlimited')
if os.path.exists(_synrix_path):
    sys.path.insert(0, _synrix_path)

try:
    from synrix.client import SynrixClient
except ImportError:
    SynrixClient = None


def search_documents(
    query_embedding: List[float],
    collection_name: str = "rag_documents",
    top_k: int = 5,
    metadata_filter: Optional[Dict[str, Any]] = None,
    synrix_client: Optional[Any] = None,
    **kwargs
) -> List[Dict[str, Any]]:
    """
    Search for documents using semantic similarity.
    
    Args:
        query_embedding: The query embedding vector
        collection_name: Name of the SYNRIX collection to search
        top_k: Number of results to return
        metadata_filter: Optional metadata filter dictionary
        synrix_client: Optional SYNRIX client instance (creates new if None)
        **kwargs: Additional arguments for SYNRIX search
        
    Returns:
        List of search results, each containing:
        - id: Document ID
        - text: Document text
        - metadata: Document metadata
        - score: Similarity score
        
    Raises:
        ValueError: If query_embedding is empty or invalid
        RuntimeError: If SYNRIX client cannot be created or search fails
    """
    if not query_embedding:
        raise ValueError("Query embedding cannot be empty")
    
    if SynrixClient is None:
        raise ImportError(
            "SYNRIX not found. Ensure synrix_unlimited is in the parent directory."
        )
    
    # Create client if not provided
    if synrix_client is None:
        try:
            synrix_client = SynrixClient()
        except Exception as e:
            raise RuntimeError(f"Failed to create SYNRIX client: {str(e)}")
    
    try:
        # Perform search using SYNRIX
        results = synrix_client.search_points(
            collection=collection_name,
            vector=query_embedding,
            limit=top_k,
            **kwargs
        )
        
        # Format results
        formatted_results = []
        for result in results:
            metadata = result.get("metadata", {})
            formatted_results.append({
                "id": result.get("id", ""),
                "text": metadata.get("text", ""),  # Text is stored in metadata
                "metadata": metadata,
                "score": result.get("score", 0.0)
            })
        
        return formatted_results
        
    except Exception as e:
        raise RuntimeError(f"Search failed: {str(e)}")


def format_search_results(results: List[Dict[str, Any]]) -> str:
    """
    Format search results as a readable string.
    
    Args:
        results: List of search result dictionaries
        
    Returns:
        Formatted string representation
    """
    if not results:
        return "No results found."
    
    formatted = []
    for i, result in enumerate(results, 1):
        formatted.append(
            f"{i}. [ID: {result['id']}] Score: {result['score']:.4f}\n"
            f"   Text: {result['text'][:200]}...\n"
            f"   Metadata: {result['metadata']}"
        )
    
    return "\n".join(formatted)
