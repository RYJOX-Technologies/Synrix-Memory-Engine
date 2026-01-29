"""
SYNRIX Mock Client

In-memory mock implementation for testing without a SYNRIX server.
Provides the same API as SynrixClient but stores data in memory.
"""

from typing import Optional, Dict, List, Any, Union
from .client import SynrixClient
from .exceptions import SynrixError, SynrixNotFoundError


class SynrixMockClient(SynrixClient):
    """
    In-memory mock client for testing.
    
    Provides the same API as SynrixClient but stores data in memory,
    making it useful for unit tests and demos without a SYNRIX server.
    
    Example:
        >>> client = SynrixMockClient()
        >>> client.create_collection("test")
        True
        >>> client.add_node("ISA_ADD", "Addition", collection="test")
        12345
        >>> results = client.query_prefix("ISA_", collection="test")
        [{'id': 12345, 'payload': {'name': 'ISA_ADD', 'data': 'Addition'}}]
    """
    
    def __init__(self):
        # Don't call super().__init__() - we don't want HTTP requests
        self.host = "mock"
        self.port = 0
        self.timeout = 30
        self.base_url = "mock://localhost"
        
        # In-memory storage
        self._collections: Dict[str, Dict[str, Any]] = {}
        self._points: Dict[str, Dict[Union[int, str], Dict[str, Any]]] = {}
        self._next_id = 1
    
    def _request(self, *args, **kwargs):
        """Override to prevent actual HTTP requests"""
        raise NotImplementedError("Mock client doesn't make HTTP requests")
    
    def list_collections(self) -> List[str]:
        """List all collections"""
        return list(self._collections.keys())
    
    def get_collection(self, name: str) -> Dict[str, Any]:
        """Get collection information"""
        if name not in self._collections:
            raise SynrixNotFoundError(f"Collection '{name}' not found")
        
        collection = self._collections[name]
        points_count = len(self._points.get(name, {}))
        
        return {
            "result": {
                "name": name,
                "config": {
                    "params": {
                        "vectors": {
                            "size": collection.get("vector_dim", 128),
                            "distance": collection.get("distance", "Cosine")
                        }
                    }
                },
                "points_count": points_count
            }
        }
    
    def create_collection(
        self,
        name: str,
        vector_dim: Optional[int] = None,
        distance: str = "Cosine"
    ) -> bool:
        """Create a new collection"""
        # Use default if not specified
        if vector_dim is None:
            vector_dim = 128
        
        if name in self._collections:
            # Collection already exists, update config
            self._collections[name]["vector_dim"] = vector_dim
            self._collections[name]["distance"] = distance
        else:
            self._collections[name] = {
                "vector_dim": vector_dim,
                "distance": distance
            }
            self._points[name] = {}
        return True
    
    def delete_collection(self, name: str) -> bool:
        """Delete a collection"""
        if name not in self._collections:
            raise SynrixNotFoundError(f"Collection '{name}' not found")
        
        del self._collections[name]
        if name in self._points:
            del self._points[name]
        return True
    
    def upsert_points(
        self,
        collection: str,
        points: List[Dict[str, Any]]
    ) -> Dict[str, Any]:
        """Upsert points (vectors) into a collection"""
        if collection not in self._collections:
            self.create_collection(collection)
        
        if collection not in self._points:
            self._points[collection] = {}
        
        for point in points:
            point_id = point.get("id")
            if point_id is None:
                point_id = self._next_id
                self._next_id += 1
                point["id"] = point_id
            
            self._points[collection][point_id] = {
                "id": point_id,
                "vector": point.get("vector", []),
                "payload": point.get("payload", {})
            }
        
        return {"status": "ok", "result": {"operation_id": self._next_id}}
    
    def search_points(
        self,
        collection: str,
        vector: List[float],
        limit: int = 10,
        score_threshold: Optional[float] = None
    ) -> List[Dict[str, Any]]:
        """Search for similar vectors (mock: returns all points)"""
        if collection not in self._points:
            return []
        
        # Mock implementation: return all points with fake scores
        results = []
        for point_id, point_data in list(self._points[collection].items())[:limit]:
            score = 0.95 - (len(results) * 0.05)  # Fake decreasing scores
            if score_threshold is None or score >= score_threshold:
                results.append({
                    "id": point_id,
                    "score": score,
                    "payload": point_data.get("payload", {})
                })
        
        return results
    
    def get_point(self, collection: str, point_id: Union[int, str]) -> Dict[str, Any]:
        """Get a point by ID"""
        if collection not in self._points:
            raise SynrixNotFoundError(f"Collection '{collection}' not found")
        
        if point_id not in self._points[collection]:
            raise SynrixNotFoundError(f"Point {point_id} not found in collection '{collection}'")
        
        return {"result": self._points[collection][point_id]}
    
    def add_node(
        self,
        name: str,
        data: str = "",
        node_type: str = "primitive",
        collection: Optional[str] = None
    ) -> Optional[int]:
        """Add a node to the knowledge graph"""
        if collection is None:
            collection = "nodes"
        
        # Create collection if it doesn't exist
        if collection not in self._collections:
            self.create_collection(collection)  # Uses engine default
        
        # Generate ID
        node_id = hash(name) % (2**63)
        if node_id < 0:
            node_id = -node_id
        
        # Store node as point
        point = {
            "id": node_id,
            "vector": [0.0] * 128,  # Placeholder vector
            "payload": {
                "name": name,
                "data": data,
                "type": node_type
            }
        }
        
        self.upsert_points(collection, [point])
        return node_id
    
    def query_prefix(
        self,
        prefix: str,
        collection: Optional[str] = None,
        limit: int = 100
    ) -> List[Dict[str, Any]]:
        """Query nodes by prefix (O(k) semantic query)"""
        if collection is None:
            collection = "nodes"
        
        if collection not in self._points:
            return []
        
        # Filter points by prefix in name
        results = []
        for point_id, point_data in self._points[collection].items():
            payload = point_data.get("payload", {})
            name = payload.get("name", "")
            
            if name.startswith(prefix):
                results.append({
                    "id": point_id,
                    "payload": payload
                })
                
                if len(results) >= limit:
                    break
        
        return results
    
    def close(self):
        """Close the mock client (no-op)"""
        pass

