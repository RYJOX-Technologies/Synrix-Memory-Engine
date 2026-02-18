"""
RAGMemory class for SYNRIX Local RAG SDK.

Main class for storing documents with embeddings and performing semantic search.
"""

import sys
import os
from typing import List, Dict, Optional, Any, Union
import uuid
import json
import hashlib

# Add SYNRIX to path - auto-detect customer installation
_synrix_path = None

def _find_synrix_installation():
    """
    Find SYNRIX installation on customer's system.
    
    Checks in order:
    1. SYNRIX_PATH environment variable (customer can set this)
    2. Common installation locations
    3. Relative paths (for development)
    4. Installed as Python package
    """
    possible_paths = []
    
    # 1. Environment variable (highest priority - customer can set this)
    env_path = os.getenv('SYNRIX_PATH')
    if env_path:
        possible_paths.append(env_path)
    
    # 2. Common installation locations
    home_dir = os.path.expanduser('~')
    possible_paths.extend([
        # Windows common locations
        r'C:\Program Files\Synrix',
        r'C:\Program Files (x86)\Synrix',
        os.path.join(home_dir, 'Synrix'),
        os.path.join(home_dir, 'synrix'),
        # Linux/Mac common locations
        '/usr/local/synrix',
        '/opt/synrix',
        os.path.join(home_dir, '.local', 'synrix'),
        os.path.join(home_dir, 'synrix'),
        # Development/workspace locations (for testing)
        os.path.join(os.path.dirname(__file__), '..', '..', 'synrix_unlimited'),
        os.path.join(os.path.dirname(__file__), '..', '..', '..', 'synrix_unlimited'),
        os.path.join(os.path.dirname(__file__), '..', '..', 'synrix'),
    ])
    
    # Check each path
    for path in possible_paths:
        if not path:
            continue
        full_path = os.path.abspath(path)
        synrix_dir = os.path.join(full_path, 'synrix')
        # Check if synrix directory exists OR if it's the synrix package itself
        if os.path.exists(full_path):
            # Check for synrix subdirectory
            if os.path.exists(synrix_dir) and os.path.exists(os.path.join(synrix_dir, '__init__.py')):
                return full_path
            # Or check if it's the synrix package itself
            if os.path.exists(os.path.join(full_path, '__init__.py')):
                # Check if it has ai_memory module
                if os.path.exists(os.path.join(full_path, 'ai_memory.py')):
                    return full_path
    
    # 3. Try importing synrix as installed package
    try:
        import synrix
        synrix_path = os.path.dirname(synrix.__file__)
        if os.path.exists(synrix_path):
            return synrix_path
    except ImportError:
        pass
    
    return None

_synrix_path = _find_synrix_installation()
if _synrix_path:
    sys.path.insert(0, _synrix_path)

# Set DLL path before importing (fixes DLL loading issues on Windows)
if os.name == 'nt':  # Windows only
    # First, check if DLLs are in System32 (from fix script)
    system32_dll = r'C:\Windows\System32\libsynrix.dll'
    if os.path.exists(system32_dll):
        # DLLs are in System32 - use those
        os.environ['SYNRIX_LIB_PATH'] = system32_dll
        # Add System32 to DLL search path (Python 3.8+)
        if hasattr(os, 'add_dll_directory'):
            try:
                os.add_dll_directory(r'C:\Windows\System32')
            except:
                pass
    elif _synrix_path and os.path.exists(_synrix_path):
        # Find synrix directory (could be synrix/ subdirectory or the path itself)
        synrix_dir = os.path.join(_synrix_path, 'synrix')
        if not os.path.exists(synrix_dir):
            synrix_dir = _synrix_path
        
        synrix_dll_path = os.path.join(synrix_dir, 'libsynrix.dll')
        if os.path.exists(synrix_dll_path):
            # Set the DLL path environment variable
            os.environ['SYNRIX_LIB_PATH'] = synrix_dll_path
            # Add the synrix directory to PATH so Windows can find DLL dependencies
            current_path = os.environ.get('PATH', '')
            if synrix_dir not in current_path:
                os.environ['PATH'] = synrix_dir + os.pathsep + current_path
            # Also add the synrix directory to DLL search path (Python 3.8+)
            if hasattr(os, 'add_dll_directory'):
                try:
                    os.add_dll_directory(synrix_dir)
                except:
                    pass

# Try to import SYNRIX (will be None if not installed)
try:
    from synrix.ai_memory import get_ai_memory
    _SYNRIX_AVAILABLE = True
except ImportError:
    get_ai_memory = None
    _SYNRIX_AVAILABLE = False

from .embeddings import generate_embedding, batch_embeddings
from .config import get_api_key, load_config
from .licensing import LicenseManager, PricingTier

# Fallback in-memory storage when SYNRIX DLL isn't available
class InMemoryStorage:
    """Simple in-memory storage fallback when SYNRIX DLL isn't available."""
    def __init__(self):
        self._storage = {}
        self._next_id = 1
    
    def add(self, key: str, value: str):
        """Add a key-value pair."""
        self._storage[key] = value
        return True
    
    def query(self, prefix: str, limit: int = 100):
        """Query by prefix."""
        results = []
        for key, value in self._storage.items():
            if key.startswith(prefix):
                results.append({
                    "name": key,
                    "data": value,
                    "id": hash(key) % (2**31)
                })
                if len(results) >= limit:
                    break
        return results
    
    def get(self, node_id: int):
        """Get by ID (not implemented in fallback)."""
        return None
    
    def count(self):
        """Get count."""
        return len(self._storage)


class RAGMemory:
    """
    RAG Memory class for document storage and semantic search.
    
    Uses SYNRIX for vector storage and retrieval.
    """
    
    def __init__(
        self,
        collection_name: str = "rag_documents",
        embedding_model: str = "local",
        embedding_api_key: Optional[str] = None,
        synrix_client: Optional[Any] = None,
        config_path: Optional[str] = None,
        **kwargs
    ):
        """
        Initialize RAGMemory.
        
        Args:
            collection_name: Name of the SYNRIX collection
            embedding_model: Embedding model to use:
                - "local" or "sentence-transformers" (default, no API key needed)
                - "openai" (requires API key)
                - "cohere" (requires API key)
                - Custom sentence-transformers model name (e.g., "all-MiniLM-L6-v2")
            embedding_api_key: API key for cloud embeddings (not needed for local models)
            synrix_client: Optional SYNRIX client instance
            config_path: Optional path to config file for API keys
            license_key: Optional license key for paid tiers
            **kwargs: Additional arguments for SYNRIX client or embedding model
        """
        self.collection_name = collection_name
        self.embedding_model = embedding_model
        self.config_path = config_path
        # Get API key from parameter, config file, or environment variable
        # Only needed for cloud models (openai, cohere)
        if embedding_model in ("openai", "cohere"):
            self.embedding_api_key = get_api_key(
                service=embedding_model,
                config_path=config_path
            ) or embedding_api_key
        else:
            # Local models don't need API keys
            self.embedding_api_key = None
        self._use_fallback = False
        # Embedding cache: text_hash -> embedding
        self._embedding_cache = {}
        
        # Create or use provided SYNRIX memory
        if synrix_client is None:
            if get_ai_memory is None:
                # SYNRIX not available - use fallback
                self._use_fallback = True
                self.synrix_memory = InMemoryStorage()
                print("[WARN] SYNRIX DLL not available. Using in-memory fallback (data will not persist).")
            else:
                try:
                    self.synrix_memory = get_ai_memory()
                except Exception as e:
                    # DLL failed to load - use fallback
                    self._use_fallback = True
                    self.synrix_memory = InMemoryStorage()
                    print(f"[WARN] SYNRIX DLL failed to load: {str(e)}")
                    print("[WARN] Using in-memory fallback (data will not persist).")
        else:
            self.synrix_memory = synrix_client
        
        # Initialize license manager (after synrix_memory is set)
        license_key = kwargs.get("license_key") or os.getenv("SYNRIX_RAG_LICENSE_KEY")
        self.license_manager = LicenseManager(license_key=license_key)

        # Report usage to backend on startup (fire-and-forget) for warning emails
        if license_key and not self._use_fallback:
            try:
                from synrix_rag.usage_report import report_usage_to_backend
                import threading
                import hashlib
                inst_id = hashlib.sha256((self.collection_name or "default").encode()).hexdigest()[:16]
                def _report():
                    try:
                        current = self.license_manager.get_current_node_count(self.synrix_memory)
                        hwid = None
                        if hasattr(self.synrix_memory, "backend") and hasattr(self.synrix_memory.backend, "get_hardware_id"):
                            hwid = self.synrix_memory.backend.get_hardware_id()
                        report_usage_to_backend(license_key, current, hwid, inst_id)
                    except Exception:
                        pass
                threading.Thread(target=_report, daemon=True).start()
            except Exception:
                pass

    def add_document(
        self,
        doc_id: Optional[str] = None,
        text: str = "",
        embedding: Optional[List[float]] = None,
        metadata: Optional[Dict[str, Any]] = None,
        **kwargs
    ) -> str:
        """
        Store a document with its embedding.
        
        Args:
            doc_id: Unique document ID (generated if None)
            text: Document text content
            embedding: Pre-computed embedding (generated if None)
            metadata: Optional metadata dictionary
            **kwargs: Additional arguments for embedding generation
            
        Returns:
            The document ID
            
        Raises:
            ValueError: If text is empty and embedding is not provided
            RuntimeError: If storage fails
        """
        if not text and embedding is None:
            raise ValueError("Either text or embedding must be provided")
        
        # Generate document ID if not provided
        if doc_id is None:
            doc_id = str(uuid.uuid4())
        
        # Generate embedding if not provided (with caching)
        if embedding is None:
            # Check cache first
            text_hash = hashlib.md5(f"{text}:{self.embedding_model}".encode()).hexdigest()
            cache_key = f"EMBED:{text_hash}"
            
            # Try to get from cache in SYNRIX
            try:
                cached_results = self.synrix_memory.query(cache_key, limit=1)
                if cached_results:
                    cached_data = json.loads(cached_results[0].get("data", "{}"))
                    embedding = cached_data.get("embedding")
                    if embedding:
                        # Cache hit - use cached embedding
                        pass
            except:
                pass
            
            # Generate if not in cache
            if embedding is None:
                try:
                    embedding = generate_embedding(
                        text=text,
                        model=self.embedding_model,
                        api_key=self.embedding_api_key,
                        **kwargs
                    )
                    # Cache the embedding in SYNRIX
                    try:
                        cache_data = {
                            "embedding": embedding,
                            "text": text,
                            "model": self.embedding_model
                        }
                        self.synrix_memory.add(cache_key, json.dumps(cache_data))
                    except:
                        pass  # Caching is optional, don't fail if it doesn't work
                except Exception as e:
                    raise RuntimeError(f"Failed to generate embedding: {str(e)}")
        
        # Prepare metadata
        doc_metadata = metadata or {}
        doc_metadata["text"] = text  # Store text in metadata for retrieval
        
        # Check license limit before storing
        current_count = self.license_manager.get_current_node_count(self.synrix_memory)
        # Each document adds ~1-2 nodes (document + potentially cached embedding)
        nodes_to_add = 2  # Document + embedding cache entry
        
        allowed, message = self.license_manager.check_limit(current_count, nodes_to_add)
        if not allowed:
            raise RuntimeError(f"License limit exceeded: {message}")
        if "Warning" in message:
            print(f"[WARN] {message}")
        
        # Store in SYNRIX using ai_memory API
        # SYNRIX uses: memory.add(name, data)
        # For RAG, we use a prefix pattern: "RAG:collection:doc_id" -> text
        try:
            # Use prefix pattern for collection support
            memory_key = f"RAG:{self.collection_name}:{doc_id}"
            # Store text with metadata encoded in the key structure
            # Store the full document data as JSON-like string for retrieval
            document_data = {
                "text": text,
                "metadata": doc_metadata,
                "embedding": embedding if embedding else None,  # Store embedding to avoid regeneration
                "embedding_dim": len(embedding) if embedding else 0
            }
            # Store as JSON string (ensure proper encoding for large arrays)
            try:
                json_str = json.dumps(document_data, ensure_ascii=False)
                self.synrix_memory.add(memory_key, json_str)
            except Exception as e:
                # If JSON encoding fails (e.g., embedding too large), store without embedding
                # Embedding can be regenerated from text
                document_data_no_embedding = {
                    "text": text,
                    "metadata": doc_metadata,
                    "embedding_dim": len(embedding) if embedding else 0
                }
                json_str = json.dumps(document_data_no_embedding, ensure_ascii=False)
                self.synrix_memory.add(memory_key, json_str)
            
            # Update usage tracking
            new_count = self.license_manager.get_current_node_count(self.synrix_memory)
            self.license_manager.update_usage(new_count)
        except Exception as e:
            raise RuntimeError(f"Failed to store document: {str(e)}")
        
        return doc_id
    
    def add_documents(
        self,
        documents: List[Dict[str, Any]],
        **kwargs
    ) -> List[str]:
        """
        Store multiple documents in batch.
        
        Args:
            documents: List of document dictionaries, each with:
                - doc_id (optional): Document ID
                - text: Document text
                - embedding (optional): Pre-computed embedding
                - metadata (optional): Document metadata
            **kwargs: Additional arguments for embedding generation
            
        Returns:
            List of document IDs
        """
        doc_ids = []
        
        # Extract texts for batch embedding
        texts = []
        texts_to_embed = []
        indices_to_embed = []
        
        for i, doc in enumerate(documents):
            doc_id = doc.get("doc_id") or str(uuid.uuid4())
            doc_ids.append(doc_id)
            
            text = doc.get("text", "")
            texts.append(text)
            
            if "embedding" not in doc:
                texts_to_embed.append(text)
                indices_to_embed.append(i)
        
        # Generate embeddings in batch if needed
        if texts_to_embed:
            try:
                embeddings = batch_embeddings(
                    texts=texts_to_embed,
                    model=self.embedding_model,
                    api_key=self.embedding_api_key,
                    **kwargs
                )
                
                # Assign embeddings back to documents
                for idx, embedding in zip(indices_to_embed, embeddings):
                    documents[idx]["embedding"] = embedding
            except Exception as e:
                raise RuntimeError(f"Failed to generate batch embeddings: {str(e)}")
        
        # Prepare points for SYNRIX
        points = []
        for i, doc in enumerate(documents):
            doc_id = doc_ids[i]
            embedding = doc.get("embedding")
            text = texts[i]
            metadata = doc.get("metadata", {})
            metadata["text"] = text
            
            points.append({
                "id": doc_id,
                "vector": embedding,
                "metadata": metadata
            })
        
        # Store in SYNRIX using ai_memory API
        try:
            for point in points:
                doc_id = point["id"]
                memory_key = f"RAG:{self.collection_name}:{doc_id}"
                # Store document data as JSON
                document_data = {
                    "text": point.get("metadata", {}).get("text", ""),
                    "metadata": point.get("metadata", {}),
                    "embedding_dim": len(point.get("vector", []))
                }
                self.synrix_memory.add(memory_key, json.dumps(document_data))
        except Exception as e:
            raise RuntimeError(f"Failed to store documents: {str(e)}")
        
        return doc_ids
    
    def search(
        self,
        query: str = "",
        top_k: int = 5,
        embedding: Optional[List[float]] = None,
        metadata_filter: Optional[Dict[str, Any]] = None,
        **kwargs
    ) -> List[Dict[str, Any]]:
        """
        Perform semantic search.
        
        Args:
            query: Search query text (ignored if embedding is provided)
            top_k: Number of results to return
            embedding: Pre-computed query embedding (generated if None)
            metadata_filter: Optional metadata filter
            **kwargs: Additional arguments for embedding generation or search
            
        Returns:
            List of search results with id, text, metadata, and score
            
        Raises:
            ValueError: If both query and embedding are empty
            RuntimeError: If search fails
        """
        if not query and embedding is None:
            # Return empty results instead of raising error
            return []
        
        # Generate query embedding if not provided
        if embedding is None:
            try:
                embedding = generate_embedding(
                    text=query,
                    model=self.embedding_model,
                    api_key=self.embedding_api_key,
                    **kwargs
                )
            except Exception as e:
                raise RuntimeError(f"Failed to generate query embedding: {str(e)}")
        
        # Perform search using SYNRIX ai_memory
        # SYNRIX uses prefix-based semantic search: memory.query(prefix, limit)
        # According to docs: "O(k) queries - Prefix-based semantic search"
        # We query by collection prefix, then rank by embedding similarity for true semantic search
        try:
            import numpy as np
            
            # Query all documents in this collection using prefix
            collection_prefix = f"RAG:{self.collection_name}:"
            all_results = self.synrix_memory.query(collection_prefix, limit=1000)
            
            # Parse and rank by embedding similarity
            scored_results = []
            for node in all_results:
                try:
                    # Parse stored JSON data (handle potential truncation)
                    node_data_str = node.get("data", "{}")
                    try:
                        node_data = json.loads(node_data_str)
                    except json.JSONDecodeError:
                        # If JSON is malformed, try to extract text directly
                        # This can happen if embedding array was truncated
                        if '"text"' in node_data_str:
                            # Extract text field manually
                            text_start = node_data_str.find('"text"')
                            if text_start != -1:
                                text_val_start = node_data_str.find('"', text_start + 6) + 1
                                text_val_end = node_data_str.find('"', text_val_start)
                                node_text = node_data_str[text_val_start:text_val_end]
                                node_data = {"text": node_text, "metadata": {}}
                            else:
                                continue
                        else:
                            continue
                    
                    # Skip deleted documents
                    if node_data.get("deleted"):
                        continue
                    
                    node_text = node_data.get("text", "")
                    node_metadata = node_data.get("metadata", {})
                    
                    # Extract doc_id from the name (format: "RAG:collection:doc_id")
                    node_name = node.get("name", "")
                    if ":" in node_name:
                        doc_id = node_name.split(":")[-1]
                    else:
                        doc_id = str(node.get("id", ""))
                    
                    # Get stored embedding if available, otherwise generate from text
                    doc_embedding = node_data.get("embedding")
                    if not doc_embedding or not isinstance(doc_embedding, list) or len(doc_embedding) == 0:
                        # Try to get from cache or regenerate
                        text_hash = hashlib.md5(f"{node_text}:{self.embedding_model}".encode()).hexdigest()
                        cache_key = f"EMBED:{text_hash}"
                        try:
                            cached_results = self.synrix_memory.query(cache_key, limit=1)
                            if cached_results:
                                try:
                                    cached_data = json.loads(cached_results[0].get("data", "{}"))
                                    doc_embedding = cached_data.get("embedding")
                                except:
                                    pass
                        except:
                            pass
                        
                        # If still no embedding, regenerate from text (for search)
                        if not doc_embedding and node_text:
                            try:
                                doc_embedding = generate_embedding(
                                    text=node_text,
                                    model=self.embedding_model,
                                    api_key=self.embedding_api_key
                                )
                            except:
                                doc_embedding = None
                    
                    # Compute cosine similarity
                    score = 0.0
                    if embedding and doc_embedding and len(embedding) == len(doc_embedding):
                        try:
                            embedding_np = np.array(embedding, dtype=np.float32)
                            doc_embedding_np = np.array(doc_embedding, dtype=np.float32)
                            dot_product = np.dot(embedding_np, doc_embedding_np)
                            norm_product = np.linalg.norm(embedding_np) * np.linalg.norm(doc_embedding_np)
                            if norm_product > 0:
                                score = float(dot_product / norm_product)
                        except Exception as e:
                            score = 0.0
                    elif node_text and query:
                        # Fallback: simple text matching score if embeddings unavailable
                        query_lower = query.lower()
                        text_lower = node_text.lower()
                        if query_lower in text_lower:
                            score = 0.5
                        # Also check for word overlap
                        query_words = set(query_lower.split())
                        text_words = set(text_lower.split())
                        if query_words and text_words:
                            overlap = len(query_words & text_words) / len(query_words)
                            score = max(score, overlap * 0.3)
                    
                    # Only include results with some score
                    if score > 0:
                        scored_results.append({
                            "id": doc_id,
                            "text": node_text,
                            "metadata": node_metadata,
                            "score": score
                        })
                except Exception as e:
                    # Skip malformed entries
                    continue
            
            # Sort by score (descending) and limit
            scored_results.sort(key=lambda x: x["score"], reverse=True)
            formatted_results = scored_results[:top_k]
            
            return formatted_results
            
        except Exception as e:
            raise RuntimeError(f"Search failed: {str(e)}")
    
    def get_context(
        self,
        query: str,
        top_k: int = 10,
        max_tokens: Optional[int] = None,
        **kwargs
    ) -> str:
        """
        Get context string for LLM from search results.
        
        Args:
            query: Search query
            top_k: Number of documents to retrieve
            max_tokens: Maximum tokens (for truncation, not passed to embeddings)
            **kwargs: Additional arguments for search (excluding max_tokens)
            
        Returns:
            Formatted context string
        """
        # Remove max_tokens from kwargs to avoid passing to embedding API
        search_kwargs = {k: v for k, v in kwargs.items() if k != 'max_tokens'}
        results = self.search(query=query, top_k=top_k, **search_kwargs)
        
        if not results:
            return ""
        
        context_parts = []
        for i, result in enumerate(results, 1):
            text = result.get("text", "")
            if text:
                context_parts.append(f"[Document {i}]\n{text}")
        
        return "\n\n".join(context_parts)
    
    def delete_document(self, doc_id: str) -> bool:
        """
        Delete a document by ID.
        
        Args:
            doc_id: Document ID to delete
            
        Returns:
            True if successful, False otherwise
            
        Note: SYNRIX ai_memory doesn't have a direct delete method.
        We mark the document as deleted by storing a deletion marker.
        """
        try:
            memory_key = f"RAG:{self.collection_name}:{doc_id}"
            
            # Check if document exists
            results = self.synrix_memory.query(memory_key, limit=1)
            if not results:
                return False  # Document doesn't exist
            
            # Store deletion marker (overwrites the document)
            deletion_marker = {
                "deleted": True,
                "doc_id": doc_id,
                "deleted_at": str(uuid.uuid4())  # Timestamp-like identifier
            }
            self.synrix_memory.add(memory_key, json.dumps(deletion_marker))
            return True
        except Exception as e:
            raise RuntimeError(f"Failed to delete document {doc_id}: {str(e)}")
    
    def list_documents(self, limit: Optional[int] = None, include_deleted: bool = False) -> List[Dict[str, Any]]:
        """
        List all documents in the collection.
        
        Args:
            limit: Optional limit on number of documents to return
            include_deleted: If True, include deleted documents
            
        Returns:
            List of document dictionaries with id, text, and metadata
        """
        try:
            collection_prefix = f"RAG:{self.collection_name}:"
            query_limit = limit if limit else 10000  # Large limit to get all
            
            results = self.synrix_memory.query(collection_prefix, limit=query_limit)
            
            documents = []
            for node in results:
                try:
                    node_data = json.loads(node.get("data", "{}"))
                    
                    # Skip deleted documents unless include_deleted is True
                    if node_data.get("deleted") and not include_deleted:
                        continue
                    
                    # Skip if it's just a deletion marker
                    if node_data.get("deleted") and not node_data.get("text"):
                        continue
                    
                    # Extract doc_id from name
                    node_name = node.get("name", "")
                    if ":" in node_name:
                        doc_id = node_name.split(":")[-1]
                    else:
                        doc_id = str(node.get("id", ""))
                    
                    documents.append({
                        "id": doc_id,
                        "text": node_data.get("text", ""),
                        "metadata": node_data.get("metadata", {})
                    })
                except (json.JSONDecodeError, KeyError):
                    continue
            
            # Apply limit if specified
            if limit and len(documents) > limit:
                documents = documents[:limit]
            
            return documents
        except Exception as e:
            raise RuntimeError(f"Failed to list documents: {str(e)}")
    
    def get_document(self, doc_id: str) -> Optional[Dict[str, Any]]:
        """
        Retrieve a document by ID.
        
        Args:
            doc_id: Document ID
            
        Returns:
            Document dictionary with id, text, metadata, or None if not found
        """
        try:
            # Search for the specific document ID
            # This is a workaround - actual implementation depends on SYNRIX API
            results = self.synrix_client.search_points(
                collection=self.collection_name,
                vector=[0.0] * 1536,  # Dummy vector - adjust dimension as needed
                limit=1000  # Large number to potentially find the document
            )
            
            for result in results:
                if result.get("id") == doc_id:
                    metadata = result.get("metadata", {})
                    return {
                        "id": doc_id,
                        "text": metadata.get("text", ""),
                        "metadata": metadata
                    }
            
            return None
        except Exception as e:
            raise RuntimeError(f"Failed to get document: {str(e)}")
    
    def list_collections(self) -> List[str]:
        """
        List all collection names.
        
        Returns:
            List of collection names
        """
        try:
            # Query all RAG documents to find unique collections
            all_results = self.synrix_memory.query("RAG:", limit=10000)
            collections = set()
            
            for node in all_results:
                node_name = node.get("name", "")
                if node_name.startswith("RAG:") and ":" in node_name:
                    parts = node_name.split(":")
                    if len(parts) >= 2:
                        collections.add(parts[1])
            
            return sorted(list(collections))
        except Exception as e:
            raise RuntimeError(f"Failed to list collections: {str(e)}")
    
    def delete_collection(self, collection_name: Optional[str] = None) -> bool:
        """
        Delete all documents in a collection.
        
        Args:
            collection_name: Collection to delete (defaults to current collection)
            
        Returns:
            True if successful
        """
        try:
            target_collection = collection_name or self.collection_name
            collection_prefix = f"RAG:{target_collection}:"
            
            # Get all documents in collection
            results = self.synrix_memory.query(collection_prefix, limit=10000)
            
            # Mark each as deleted
            for node in results:
                node_name = node.get("name", "")
                if ":" in node_name:
                    doc_id = node_name.split(":")[-1]
                    self.delete_document(doc_id)
            
            return True
        except Exception as e:
            raise RuntimeError(f"Failed to delete collection {collection_name or self.collection_name}: {str(e)}")
    
    def get_license_info(self) -> Dict[str, Any]:
        """
        Get license and usage information.
        
        Returns:
            Dictionary with tier, limit, usage, and remaining nodes
        """
        # Update usage from actual SYNRIX count
        current_count = self.license_manager.get_current_node_count(self.synrix_memory)
        self.license_manager.update_usage(current_count)
        return self.license_manager.get_tier_info()
    
    def check_license_limit(self) -> tuple[bool, str]:
        """
        Check if current usage is within license limits.
        
        Returns:
            (within_limit, message)
        """
        current_count = self.license_manager.get_current_node_count(self.synrix_memory)
        return self.license_manager.check_limit(current_count)