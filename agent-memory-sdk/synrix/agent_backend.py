"""
SYNRIX Agent Backend - Direct C Integration for AI Agents
==========================================================
This module provides the most robust, direct access to SYNRIX for AI agents.

It automatically detects and uses the best available backend:
1. Direct shared memory (fastest, if server running)
2. HTTP client (fallback)
3. Mock client (for testing)

Usage in Cursor AI:
    from synrix.agent_backend import get_synrix_backend
    
    backend = get_synrix_backend()
    backend.write("task:fix_bug", {"error": "SyntaxError", "fix": "add colon"})
    memory = backend.read("task:fix_bug")
"""

import os
import json
from typing import Optional, Dict, Any, List
try:
    from .direct_client import SynrixDirectClient
    DIRECT_CLIENT_AVAILABLE = True
except ImportError:
    SynrixDirectClient = None
    DIRECT_CLIENT_AVAILABLE = False
from .client import SynrixClient
from .mock import SynrixMockClient


class SynrixAgentBackend:
    """
    Unified backend interface for AI agents to use SYNRIX.
    
    Automatically selects the best available backend:
    - Direct shared memory (if server running) - fastest
    - HTTP client (if server running on different machine)
    - Mock client (for testing/development)
    """
    
    def __init__(self, 
                 use_direct: bool = True,
                 use_mock: bool = False,
                 server_url: Optional[str] = None,
                 collection: str = "agent_memory"):
        """
        Initialize SYNRIX backend.
        
        Args:
            use_direct: Try to use direct shared memory (fastest)
            use_mock: Use mock client (for testing)
            server_url: HTTP server URL (if not using direct)
            collection: Collection name for storing data
        """
        self.collection = collection
        self.client = None
        self._init_client(use_direct, use_mock, server_url)
    
    def _init_client(self, use_direct: bool, use_mock: bool, server_url: Optional[str]):
        """Initialize the best available client"""
        if use_mock:
            self.client = SynrixMockClient()
            self.backend_type = "mock"
            return
        
        if use_direct and DIRECT_CLIENT_AVAILABLE:
            try:
                self.client = SynrixDirectClient()
                self.backend_type = "direct"
                # Ensure collection exists
                try:
                    self.client.create_collection(self.collection)
                except:
                    pass  # Collection might already exist
                return
            except Exception as e:
                # Fall back to HTTP
                pass
        
        # Fall back to HTTP
        self.client = SynrixClient(base_url=server_url or "http://localhost:6333")
        self.backend_type = "http"
        try:
            self.client.create_collection(self.collection)
        except:
            pass  # Collection might already exist
    
    def write(self, key: str, value: Any, metadata: Optional[Dict] = None) -> Optional[int]:
        """
        Write data to SYNRIX.
        
        Args:
            key: Key/name for the data (e.g., "task:fix_bug")
            value: Data to store (will be JSON serialized)
            metadata: Optional metadata dict
        
        Returns:
            Node ID if successful, None otherwise
        """
        data = {
            "value": value,
            "metadata": metadata or {},
            "timestamp": self._get_timestamp()
        }
        data_str = json.dumps(data)
        
        try:
            node_id = self.client.add_node(
                name=key,
                data=data_str,
                collection=self.collection
            )
            return node_id
        except Exception as e:
            print(f"Warning: Failed to write to SYNRIX: {e}")
            return None
    
    def read(self, key: str) -> Optional[Dict[str, Any]]:
        """
        Read data from SYNRIX by exact key (O(1) if using direct client with node ID).
        
        Args:
            key: Key/name to retrieve
        
        Returns:
            Dict with data, or None if not found
        """
        # Try O(1) lookup if we have a node ID stored
        # For now, use prefix query (O(k)) - can be optimized later
        results = self.query_prefix(key, limit=1)
        if results:
            return results[0]
        return None
    
    def get_by_id(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        Get node by ID (O(1) lookup).
        
        Args:
            node_id: Node ID
        
        Returns:
            Node data or None
        """
        if hasattr(self.client, 'get_node_by_id'):
            try:
                return self.client.get_node_by_id(node_id)
            except:
                pass
        return None
    
    def query_prefix(self, prefix: str, limit: int = 100) -> List[Dict[str, Any]]:
        """
        Query by prefix (O(k) semantic search).
        
        Args:
            prefix: Prefix to search for (e.g., "task:")
            limit: Maximum results
        
        Returns:
            List of matching nodes
        """
        try:
            results = self.client.query_prefix(
                prefix=prefix,
                collection=self.collection,
                limit=limit
            )
            # Parse JSON data from results
            parsed = []
            for result in results:
                payload = result.get("payload", {})
                data_str = payload.get("data", "{}")
                try:
                    data = json.loads(data_str)
                    parsed.append({
                        "key": payload.get("name", ""),
                        "data": data,
                        "id": result.get("id"),
                        "score": result.get("score", 0.0)
                    })
                except json.JSONDecodeError:
                    # Fallback for non-JSON data
                    parsed.append({
                        "key": payload.get("name", ""),
                        "data": {"value": data_str},
                        "id": result.get("id"),
                        "score": result.get("score", 0.0)
                    })
            return parsed
        except Exception as e:
            print(f"Warning: Failed to query SYNRIX: {e}")
            return []
    
    def get_task_memory(self, task_type: str, limit: int = 20) -> Dict[str, Any]:
        """
        Get all memory for a task type (optimized single query).
        
        Args:
            task_type: Task type (e.g., "fix_bug")
            limit: Maximum results
        
        Returns:
            Dict with last_attempts, failures, successes, most_common_failure
        """
        prefix = f"task:{task_type}:"
        results = self.query_prefix(prefix, limit=limit * 3)
        
        attempts = []
        failures = []
        successes = []
        failure_patterns = set()
        
        for result in results:
            data = result.get("data", {})
            value = str(data.get("value", "")).lower()
            entry = {
                "key": result.get("key", ""),
                "data": data,
                "id": result.get("id"),
                "timestamp": data.get("timestamp", 0)
            }
            attempts.append(entry)
            
            if any(word in value for word in ["fail", "error", "timeout"]):
                failures.append(entry)
                error = data.get("metadata", {}).get("error")
                if error:
                    failure_patterns.add(error)
            elif "success" in value:
                successes.append(entry)
        
        # Sort by timestamp
        attempts.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        failures.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        successes.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        
        # Find most common failure
        most_common_failure = None
        if failures:
            error_counts = {}
            for failure in failures:
                error = failure.get("data", {}).get("metadata", {}).get("error")
                if error:
                    error_counts[error] = error_counts.get(error, 0) + 1
            if error_counts:
                most_common_error = max(error_counts, key=error_counts.get)
                most_common_failure = next(
                    (f for f in failures 
                     if f.get("data", {}).get("metadata", {}).get("error") == most_common_error),
                    None
                )
        
        return {
            "last_attempts": attempts[:limit],
            "failures": failures[:limit],
            "successes": successes[:limit],
            "most_common_failure": most_common_failure,
            "failure_patterns": list(failure_patterns)
        }
    
    def close(self):
        """Close the backend connection"""
        if hasattr(self.client, 'close'):
            self.client.close()
    
    def _get_timestamp(self) -> int:
        """Get current timestamp in microseconds"""
        import time
        return int(time.time() * 1000000)
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


def get_synrix_backend(**kwargs) -> SynrixAgentBackend:
    """
    Get a SYNRIX backend instance (factory function).
    
    Automatically detects the best available backend.
    
    Usage:
        backend = get_synrix_backend()
        backend.write("key", {"data": "value"})
        memory = backend.read("key")
    """
    return SynrixAgentBackend(**kwargs)

