"""
SYNRIX Agent Memory

High-level memory interface for AI agents.
Provides episodic memory, failure tracking, and pattern learning.
"""

import json
import time
from typing import Optional, Dict, List, Any
from .client import SynrixClient

# Try to import direct client (shared memory)
try:
    from .direct_client import SynrixDirectClient
    DIRECT_CLIENT_AVAILABLE = True
except ImportError:
    DIRECT_CLIENT_AVAILABLE = False
    SynrixDirectClient = None


class SynrixMemory:
    """
    Agent memory store built on SYNRIX.
    
    Provides persistent, fast memory for AI agents with:
    - Episodic memory (task attempts, results)
    - Failure pattern tracking
    - Success pattern learning
    - Fast semantic retrieval
    
    Example:
        >>> memory = SynrixMemory()
        >>> memory.write("task:1:attempt", "result_failed", {"error": "timeout"})
        >>> attempts = memory.get_last_attempts("1", limit=5)
        >>> failures = memory.get_failed_attempts("1")
    """
    
    def __init__(
        self,
        client: Optional[SynrixClient] = None,
        collection: str = "agent_memory",
        use_direct: bool = True
    ):
        """
        Initialize agent memory.
        
        Args:
            client: SYNRIX client (default: creates new client)
            collection: Collection name for storing memories
            use_direct: If True, try to use direct shared memory client (faster)
        """
        if client is None:
            if use_direct and DIRECT_CLIENT_AVAILABLE:
                try:
                    self.client = SynrixDirectClient()
                except Exception:
                    # Fallback to HTTP if shared memory not available
                    self.client = SynrixClient()
            else:
                self.client = SynrixClient()
        
        self.collection = collection
        self._ensure_collection()
    
    def _ensure_collection(self):
        """Ensure collection exists"""
        try:
            self.client.get_collection(self.collection)
        except Exception:
            self.client.create_collection(self.collection)
    
    def write(
        self,
        key: str,
        value: str,
        metadata: Optional[Dict[str, Any]] = None,
        timestamp: Optional[float] = None
    ) -> Optional[int]:
        """
        Store agent memory.
        
        Args:
            key: Memory key (e.g., "task:1:attempt", "api:stripe:call")
            value: Memory value (e.g., "result_failed", "success")
            metadata: Optional metadata (error details, context, etc.)
            timestamp: Optional timestamp (default: current time)
        
        Returns:
            Node ID if successful, None otherwise
        
        Example:
            >>> memory.write("task:1:attempt", "result_failed", {"error": "timeout"})
        """
        if timestamp is None:
            timestamp = time.time()
        
        data = {
            "value": value,
            "metadata": metadata or {},
            "timestamp": timestamp
        }
        
        data_str = json.dumps(data)
        return self.client.add_node(key, data_str, collection=self.collection)
    
    def get_node_by_id(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        O(1) direct lookup by node ID.
        
        Args:
            node_id: Node ID to lookup
            
        Returns:
            Node data dictionary or None if not found
            
        Example:
            >>> node = memory.get_node_by_id(12345)
            >>> print(node["payload"]["name"])
        """
        if hasattr(self.client, 'get_node_by_id'):
            return self.client.get_node_by_id(node_id)
        return None
    
    def read(self, pattern: str, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Retrieve memories by pattern.
        
        Args:
            pattern: Pattern to match (e.g., "task:1:*" -> "task:1:")
            limit: Maximum number of results
        
        Returns:
            List of memory entries
        
        Example:
            >>> memories = memory.read("task:1:*")
            >>> for mem in memories:
            ...     print(mem["value"])
        """
        # Convert pattern to prefix (remove wildcard)
        prefix = pattern.replace("*", "").split(":")[0] + ":"
        if ":" in pattern:
            parts = pattern.split(":")
            if len(parts) >= 2:
                prefix = ":".join(parts[:-1]) + ":"
        
        results = self.client.query_prefix(prefix, collection=self.collection, limit=limit)
        
        memories = []
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "")
            data_str = payload.get("data", "{}")
            
            try:
                data = json.loads(data_str)
                memories.append({
                    "key": name,
                    "value": data.get("value", ""),
                    "metadata": data.get("metadata", {}),
                    "timestamp": data.get("timestamp", 0)
                })
            except json.JSONDecodeError:
                # Fallback for non-JSON data
                memories.append({
                    "key": name,
                    "value": data_str,
                    "metadata": {},
                    "timestamp": 0
                })
        
        return memories
    
    def get_last_attempts(
        self,
        task_type: str,
        limit: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Get last N attempts for a task type.
        
        Args:
            task_type: Task type (e.g., "file_generation", "api_call")
            limit: Number of attempts to retrieve
        
        Returns:
            List of attempts, sorted by timestamp (newest first)
        
        Example:
            >>> attempts = memory.get_last_attempts("file_generation", limit=5)
            >>> for attempt in attempts:
            ...     print(f"{attempt['value']} at {attempt['timestamp']}")
        """
        prefix = f"task:{task_type}:"
        results = self.client.query_prefix(prefix, collection=self.collection, limit=limit * 2)
        
        attempts = []
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "")
            data_str = payload.get("data", "{}")
            
            try:
                data = json.loads(data_str)
                attempts.append({
                    "key": name,
                    "value": data.get("value", ""),
                    "metadata": data.get("metadata", {}),
                    "timestamp": data.get("timestamp", 0)
                })
            except json.JSONDecodeError:
                continue
        
        # Sort by timestamp (newest first) and limit
        attempts.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        return attempts[:limit]
    
    def get_failed_attempts(self, task_type: str) -> List[Dict[str, Any]]:
        """
        Get all failed attempts for a task type.
        
        Args:
            task_type: Task type to query
        
        Returns:
            List of failed attempts
        
        Example:
            >>> failures = memory.get_failed_attempts("api_call")
            >>> print(f"Found {len(failures)} failures")
        """
        prefix = f"task:{task_type}:"
        results = self.client.query_prefix(prefix, collection=self.collection, limit=100)
        
        failures = []
        for result in results:
            payload = result.get("payload", {})
            data_str = payload.get("data", "{}")
            
            try:
                data = json.loads(data_str)
                value = data.get("value", "").lower()
                if "fail" in value or "error" in value or "timeout" in value:
                    failures.append({
                        "key": payload.get("name", ""),
                        "value": data.get("value", ""),
                        "metadata": data.get("metadata", {}),
                        "timestamp": data.get("timestamp", 0)
                    })
            except json.JSONDecodeError:
                if "fail" in data_str.lower() or "error" in data_str.lower():
                    failures.append({
                        "key": payload.get("name", ""),
                        "value": data_str,
                        "metadata": {},
                        "timestamp": 0
                    })
        
        return failures
    
    def get_successful_patterns(self, task_type: str) -> List[Dict[str, Any]]:
        """
        Get successful patterns for a task type.
        
        Args:
            task_type: Task type to query
        
        Returns:
            List of successful attempts
        
        Example:
            >>> successes = memory.get_successful_patterns("file_generation")
            >>> print(f"Found {len(successes)} successful patterns")
        """
        prefix = f"task:{task_type}:"
        results = self.client.query_prefix(prefix, collection=self.collection, limit=100)
        
        successes = []
        for result in results:
            payload = result.get("payload", {})
            data_str = payload.get("data", "{}")
            
            try:
                data = json.loads(data_str)
                value = data.get("value", "").lower()
                if "success" in value or "complete" in value or "ok" in value:
                    successes.append({
                        "key": payload.get("name", ""),
                        "value": data.get("value", ""),
                        "metadata": data.get("metadata", {}),
                        "timestamp": data.get("timestamp", 0)
                    })
            except json.JSONDecodeError:
                if "success" in data_str.lower() or "complete" in data_str.lower():
                    successes.append({
                        "key": payload.get("name", ""),
                        "value": data_str,
                        "metadata": {},
                        "timestamp": 0
                    })
        
        return successes
    
    def get_task_memory_summary(self, task_type: str, limit: int = 20) -> Dict[str, Any]:
        """
        Get all memory data for a task type in a single O(k) query.
        Returns: {
            "last_attempts": [...],
            "failures": [...],
            "successes": [...],
            "most_common_failure": {...},
            "failure_patterns": set([...])
        }
        """
        # Single query - O(k) where k is result size
        prefix = f"task:{task_type}:"
        results = self.client.query_prefix(prefix, collection=self.collection, limit=limit * 3)
        
        attempts = []
        failures = []
        successes = []
        failure_patterns = set()
        
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "")
            data_str = payload.get("data", "{}")
            
            try:
                data = json.loads(data_str)
                value = data.get("value", "").lower()
                entry = {
                    "key": name,
                    "value": data.get("value", ""),
                    "metadata": data.get("metadata", {}),
                    "timestamp": data.get("timestamp", 0)
                }
                attempts.append(entry)
                
                if "fail" in value or "error" in value or "timeout" in value:
                    failures.append(entry)
                    # Extract error pattern
                    error = data.get("metadata", {}).get("error")
                    if error:
                        failure_patterns.add(error)
                elif "success" in value:
                    successes.append(entry)
            except json.JSONDecodeError:
                continue
        
        # Sort by timestamp (newest first)
        attempts.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        failures.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        successes.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        
        # Find most common failure
        most_common_failure = None
        if failures:
            error_counts = {}
            for failure in failures:
                error = failure.get("metadata", {}).get("error")
                if error:
                    error_counts[error] = error_counts.get(error, 0) + 1
            
            if error_counts:
                most_common_error = max(error_counts.items(), key=lambda x: x[1])[0]
                # Find the most recent occurrence of this error
                for failure in failures:
                    if failure.get("metadata", {}).get("error") == most_common_error:
                        most_common_failure = failure
                        break
        
        return {
            "last_attempts": attempts[:limit],
            "failures": failures,
            "successes": successes[:limit],
            "most_common_failure": most_common_failure,
            "failure_patterns": failure_patterns
        }
    
    def get_most_frequent_failure(self, task_type: str) -> Optional[Dict[str, Any]]:
        """
        Get the most frequent failure pattern.
        
        Args:
            task_type: Task type to analyze
        
        Returns:
            Most common failure pattern, or None
        
        Example:
            >>> failure = memory.get_most_frequent_failure("api_call")
            >>> if failure:
            ...     print(f"Most common failure: {failure['value']}")
        """
        failures = self.get_failed_attempts(task_type)
        if not failures:
            return None
        
        # Count failure patterns
        failure_counts = {}
        for failure in failures:
            value = failure.get("value", "")
            failure_counts[value] = failure_counts.get(value, 0) + 1
        
        # Get most frequent
        if failure_counts:
            most_common = max(failure_counts.items(), key=lambda x: x[1])
            # Find a matching failure entry
            for failure in failures:
                if failure.get("value") == most_common[0]:
                    failure["count"] = most_common[1]
                    return failure
        
        return failures[0] if failures else None
    
    def clear(self):
        """Clear all memories (for testing)"""
        try:
            self.client.delete_collection(self.collection)
            self._ensure_collection()
        except Exception:
            pass
    
    def close(self):
        """Close the client connection"""
        if hasattr(self.client, 'close'):
            self.client.close()

