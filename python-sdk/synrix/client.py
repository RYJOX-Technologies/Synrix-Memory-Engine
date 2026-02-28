"""
SYNRIX Python Client

A client library for interacting with SYNRIX knowledge graph engine.
"""

import json
import requests
import time
from typing import Optional, Dict, List, Any, Union
from .exceptions import SynrixError, SynrixConnectionError, SynrixNotFoundError, SynrixValidationError
from .telemetry import get_telemetry, record_operation


class SynrixClient:
    """
    Client for interacting with SYNRIX knowledge graph engine.
    
    Args:
        host: SYNRIX server host (default: "localhost")
        port: SYNRIX server port (default: 6334)
        timeout: Request timeout in seconds (default: 30)
    
    Example:
        >>> client = SynrixClient(host="localhost", port=6334)
        >>> node_id = client.add_node("ISA_ADD", "Addition operation")
        >>> results = client.query_prefix("ISA_")
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 6334,
        timeout: int = 30
    ):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.base_url = f"http://{host}:{port}"
        self.session = requests.Session()
    
    def _request(
        self,
        method: str,
        endpoint: str,
        data: Optional[Dict[str, Any]] = None,
        params: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """Make HTTP request to SYNRIX server"""
        url = f"{self.base_url}{endpoint}"
        
        # Record operation start for telemetry
        start_time = time.time()
        operation_name = f"{method.lower()}_{endpoint.split('/')[1] if len(endpoint.split('/')) > 1 else 'unknown'}"
        
        try:
            if method == "GET":
                response = self.session.get(url, params=params, timeout=self.timeout)
            elif method == "POST":
                # Compact JSON so server parsers that expect "key":"value" work (spaced "key": "value" can fail)
                if data is not None:
                    response = self.session.post(url, data=json.dumps(data, separators=(',', ':')),
                        headers={"Content-Type": "application/json"}, timeout=self.timeout)
                else:
                    response = self.session.post(url, timeout=self.timeout)
            elif method == "PUT":
                if data is not None:
                    response = self.session.put(url, data=json.dumps(data, separators=(',', ':')),
                        headers={"Content-Type": "application/json"}, timeout=self.timeout)
                else:
                    response = self.session.put(url, timeout=self.timeout)
            elif method == "DELETE":
                response = self.session.delete(url, timeout=self.timeout)
            else:
                raise SynrixError(f"Unsupported HTTP method: {method}")
            
            response.raise_for_status()
            
            # Record successful operation
            latency_ms = (time.time() - start_time) * 1000
            record_operation(operation_name, latency_ms=latency_ms, success=True)
            
            if response.content:
                return response.json()
            return {}
            
        except requests.exceptions.ConnectionError as e:
            latency_ms = (time.time() - start_time) * 1000
            record_operation(operation_name, latency_ms=latency_ms, success=False, error_type="ConnectionError")
            raise SynrixConnectionError(
                f"Failed to connect to SYNRIX server at {self.base_url}: {e}"
            )
        except requests.exceptions.Timeout as e:
            latency_ms = (time.time() - start_time) * 1000
            record_operation(operation_name, latency_ms=latency_ms, success=False, error_type="Timeout")
            raise SynrixConnectionError(
                f"Request to SYNRIX server timed out after {self.timeout}s: {e}"
            )
        except requests.exceptions.HTTPError as e:
            latency_ms = (time.time() - start_time) * 1000
            error_type = f"HTTP{response.status_code}"
            record_operation(operation_name, latency_ms=latency_ms, success=False, error_type=error_type)
            if response.status_code == 404:
                raise SynrixNotFoundError(f"Resource not found: {endpoint}")
            raise SynrixError(f"HTTP error {response.status_code}: {response.text}")
        except json.JSONDecodeError as e:
            latency_ms = (time.time() - start_time) * 1000
            record_operation(operation_name, latency_ms=latency_ms, success=False, error_type="JSONDecodeError")
            raise SynrixError(f"Invalid JSON response: {e}")
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            record_operation(operation_name, latency_ms=latency_ms, success=False, error_type=type(e).__name__)
            raise
    
    # Collection operations (Qdrant-compatible API)
    
    def list_collections(self) -> List[str]:
        """
        List all collections.
        
        Returns:
            List of collection names
        
        Example:
            >>> collections = client.list_collections()
            >>> print(collections)
            ['my_collection', 'another_collection']
        """
        response = self._request("GET", "/collections")
        result = response.get("result", {})
        collections = result.get("collections", [])
        return [col.get("name", col) if isinstance(col, dict) else col for col in collections]
    
    def get_collection(self, name: str) -> Dict[str, Any]:
        """
        Get collection information.
        
        Args:
            name: Collection name
        
        Returns:
            Collection information dictionary
        
        Example:
            >>> collection = client.get_collection("my_collection")
            >>> print(collection["points_count"])
            1000
        """
        return self._request("GET", f"/collections/{name}")
    
    def create_collection(
        self,
        name: str,
        vector_dim: Optional[int] = None,
        distance: str = "Cosine"
    ) -> bool:
        """
        Create a new collection.
        
        Args:
            name: Collection name
            vector_dim: Vector dimension (optional, engine-specific default if None)
            distance: Distance metric ("Cosine", "Euclidean", "Dot") (default: "Cosine")
        
        Returns:
            True if successful
        
        Example:
            >>> client.create_collection("embeddings", vector_dim=384)
            True
        """
        # Use engine default if not specified
        if vector_dim is None:
            vector_dim = 128  # Default for compatibility, but not enforced by engine
        
        config = {
            "vectors": {
                "size": vector_dim,
                "distance": distance
            }
        }
        self._request("PUT", f"/collections/{name}", data={"vectors": config})
        return True
    
    def delete_collection(self, name: str) -> bool:
        """
        Delete a collection.
        
        Args:
            name: Collection name
        
        Returns:
            True if successful
        """
        self._request("DELETE", f"/collections/{name}")
        return True
    
    # Point/Node operations
    
    def upsert_points(
        self,
        collection: str,
        points: List[Dict[str, Any]]
    ) -> Dict[str, Any]:
        """
        Upsert points (vectors) into a collection.
        
        Args:
            collection: Collection name
            points: List of point dictionaries with "id" and "vector" keys
        
        Returns:
            Operation result
        
        Example:
            >>> points = [
            ...     {"id": 1, "vector": [0.1, 0.2, 0.3]},
            ...     {"id": 2, "vector": [0.4, 0.5, 0.6]}
            ... ]
            >>> client.upsert_points("embeddings", points)
        """
        return self._request("PUT", f"/collections/{collection}/points", data={"points": points})
    
    def search_points(
        self,
        collection: str,
        vector: List[float],
        limit: int = 10,
        score_threshold: Optional[float] = None
    ) -> List[Dict[str, Any]]:
        """
        Search for similar vectors.
        
        Args:
            collection: Collection name
            vector: Query vector
            limit: Maximum number of results (default: 10)
            score_threshold: Minimum similarity score (optional)
        
        Returns:
            List of similar points with scores
        
        Example:
            >>> results = client.search_points("embeddings", [0.1, 0.2, 0.3], limit=5)
            >>> for result in results:
            ...     print(f"ID: {result['id']}, Score: {result['score']}")
        """
        data = {
            "vector": vector,
            "limit": limit
        }
        if score_threshold is not None:
            data["score_threshold"] = score_threshold
        
        response = self._request("POST", f"/collections/{collection}/points/search", data=data)
        return response.get("result", [])
    
    def get_point(self, collection: str, point_id: Union[int, str]) -> Dict[str, Any]:
        """
        Get a point by ID.
        
        Args:
            collection: Collection name
            point_id: Point ID
        
        Returns:
            Point data
        
        Example:
            >>> point = client.get_point("embeddings", 123)
            >>> print(point["vector"])
            [0.1, 0.2, 0.3]
        """
        response = self._request("GET", f"/collections/{collection}/points/{point_id}")
        return response.get("result", {})
    
    # Direct SYNRIX graph operations (if native API exists)
    # These would require a native SYNRIX REST API endpoint
    
    def add_node(
        self,
        name: str,
        data: str = "",
        node_type: str = "learning",
        collection: Optional[str] = None
    ) -> Optional[int]:
        """
        Add a node to the knowledge graph using native SYNRIX API.
        
        Nodes are stored by semantic name (e.g., "ISA_ADD") and automatically
        indexed by the dynamic prefix index for O(k) queries.
        
        Args:
            name: Node name (use prefix like "ISA_" for semantic indexing)
            data: Node data (text)
            node_type: Node type ("learning", "primitive", "pattern", "performance", "material")
            collection: Collection/namespace (default: "default", stored as "collection:name")
        
        Returns:
            Node ID if successful, None otherwise
        
        Example:
            >>> node_id = client.add_node("ISA_ADD", "Addition operation")
            >>> print(node_id)
            12345
        """
        if collection is None:
            collection = "default"
        
        # Use native SYNRIX API endpoint (stores by semantic name, works with dynamic prefix index)
        try:
            request_data = {
                "name": name,
                "data": data,
                "type": node_type,
                "collection": collection
            }
            response = self._request("POST", "/synrix/nodes", data=request_data)
            result = response.get("result", {})
            return result.get("id")
        except SynrixConnectionError:
            # Fallback: try direct shared memory if available
            try:
                from .direct_client import SynrixDirectClient
                direct_client = SynrixDirectClient()
                node_id = direct_client.add_node(name, data, collection)
                direct_client.close()
                return node_id
            except:
                raise SynrixError("Failed to connect to SYNRIX server")
        except Exception as e:
            raise SynrixError(f"Failed to add node: {e}")
    
    def query_prefix(
        self,
        prefix: str,
        collection: Optional[str] = None,
        limit: int = 100
    ) -> List[Dict[str, Any]]:
        """
        Query nodes by prefix using dynamic prefix index (O(k) semantic query).
        
        Uses the dynamic prefix index for fast queries. Works with nodes stored
        via add_node() which stores by semantic name.
        
        Args:
            prefix: Prefix to search for (e.g., "ISA_")
            collection: Collection/namespace (default: "default", queries "collection:prefix")
            limit: Maximum number of results (default: 100)
        
        Returns:
            List of matching nodes with "id", "payload" containing "name" and "data"
        
        Example:
            >>> results = client.query_prefix("ISA_")
            >>> for node in results:
            ...     payload = node.get("payload", {})
            ...     print(payload.get("name"))
        """
        if collection is None:
            collection = "default"
        
        try:
            # Query using native SYNRIX query endpoint
            # Format: {"query":{"prefix":{"key":"name","match":{"value":"ISA_"}}},"limit":100}
            data = {
                "query": {
                    "prefix": {
                        "key": "name",
                        "match": {
                            "value": prefix
                        }
                    }
                },
                "limit": limit
            }
            response = self._request("POST", f"/collections/{collection}/query", data=data)
            return response.get("result", {}).get("points", [])
        except SynrixConnectionError:
            # Fallback: try direct shared memory if available
            try:
                from .direct_client import SynrixDirectClient
                direct_client = SynrixDirectClient()
                results = direct_client.query_prefix(prefix, collection, limit)
                direct_client.close()
                return results
            except:
                return []
        except Exception:
            return []
    
    def close(self):
        """Close the client session"""
        self.session.close()
    
    def __enter__(self):
        """Context manager entry"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.close()

