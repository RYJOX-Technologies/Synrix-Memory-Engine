"""
Auto-Memory Module for Cursor AI Agent
=======================================
This module provides automatic SYNRIX integration that the AI agent can use
without explicit setup. Just import and use.

Usage:
    from synrix.auto_memory import get_ai_memory
    
    memory = get_ai_memory()
    constraints = memory.get_constraints()
    patterns = memory.get_patterns("async_handler")
    memory.store_pattern("async_handler", code, success=True)
"""

import os
import json
from typing import Optional, List, Dict, Any
from functools import lru_cache

try:
    from .raw_backend import RawSynrixBackend, LATTICE_NODE_PATTERN, LATTICE_NODE_LEARNING, LATTICE_NODE_ANTI_PATTERN
    BACKEND_AVAILABLE = True
except ImportError:
    BACKEND_AVAILABLE = False
    RawSynrixBackend = None


# Global memory instance (lazy-loaded)
_ai_memory: Optional[RawSynrixBackend] = None


def get_ai_memory() -> RawSynrixBackend:
    """
    Get or create the AI agent's persistent memory.
    
    This is a singleton - same instance across the session.
    Auto-creates the lattice file if it doesn't exist.
    
    Returns:
        RawSynrixBackend instance
        
    Example:
        >>> memory = get_ai_memory()
        >>> constraints = memory.get_constraints()
    """
    global _ai_memory
    
    if _ai_memory is None:
        if not BACKEND_AVAILABLE:
            raise ImportError(
                "SYNRIX backend not available. Install with: pip install -e ."
            )
        
        memory_path = os.path.expanduser("~/.cursor_ai_memory.lattice")
        _ai_memory = RawSynrixBackend(memory_path, max_nodes=25000)

        # Usage reporting to backend (fire-and-forget) when license key is set
        _report_usage_on_startup(_ai_memory, memory_path)
    
    return _ai_memory


def _report_usage_on_startup(backend: "RawSynrixBackend", lattice_path: Optional[str] = None) -> None:
    """Report node count to backend if SYNRIX_LICENSE_KEY is set (daemon thread)."""
    license_key = os.environ.get("SYNRIX_LICENSE_KEY") or os.environ.get("SYNRIX_RAG_LICENSE_KEY")
    if not license_key:
        return
    try:
        from .usage_report import report_usage_to_backend
        import threading
        import hashlib
        inst_id = hashlib.sha256((lattice_path or "").encode()).hexdigest()[:16] if lattice_path else None
        def _do():
            try:
                count = len(backend.find_by_prefix("", limit=1_000_000))
                hwid = backend.get_hardware_id() if hasattr(backend, "get_hardware_id") else None
                report_usage_to_backend(license_key, count, hwid, inst_id)
            except Exception:
                pass
        threading.Thread(target=_do, daemon=True).start()
    except Exception:
        pass


class AIMemoryHelper:
    """
    High-level helper for AI agent memory operations.
    
    Provides convenient methods for common memory operations:
    - Getting constraints before code generation
    - Storing/retrieving patterns
    - Tracking successes/failures
    """
    
    def __init__(self):
        self.backend = get_ai_memory()
    
    def get_constraints(self) -> List[Dict[str, Any]]:
        """
        Get all project constraints.
        
        Returns:
            List of constraint dictionaries with 'name' and 'data' keys
            
        Example:
            >>> memory = AIMemoryHelper()
            >>> constraints = memory.get_constraints()
            >>> for c in constraints:
            ...     print(f"{c['name']}: {c['data']}")
        """
        results = self.backend.find_by_prefix("CONSTRAINT:", limit=100)
        return [
            {"name": r.get("name", ""), "data": r.get("data", "")}
            for r in results
        ]
    
    def get_patterns(self, pattern_name: Optional[str] = None, limit: int = 20) -> List[Dict[str, Any]]:
        """
        Get code patterns.
        
        Args:
            pattern_name: Optional pattern name to filter (e.g., "async_handler")
            limit: Maximum number of results
            
        Returns:
            List of pattern dictionaries
            
        Example:
            >>> memory = AIMemoryHelper()
            >>> patterns = memory.get_patterns("async_handler")
            >>> for p in patterns:
            ...     code = json.loads(p['data']).get('code')
        """
        if pattern_name:
            prefix = f"PATTERN:{pattern_name}"
        else:
            prefix = "PATTERN:"
        
        results = self.backend.find_by_prefix(prefix, limit=limit)
        patterns = []
        for r in results:
            try:
                data = json.loads(r.get("data", "{}"))
                patterns.append({
                    "name": r.get("name", ""),
                    "data": data,
                    "id": r.get("id")
                })
            except json.JSONDecodeError:
                patterns.append({
                    "name": r.get("name", ""),
                    "data": {"value": r.get("data", "")},
                    "id": r.get("id")
                })
        return patterns
    
    def store_pattern(
        self,
        pattern_name: str,
        code: str,
        context: str = "",
        success_rate: float = 1.0,
        metadata: Optional[Dict] = None
    ) -> int:
        """
        Store a code pattern.
        
        Args:
            pattern_name: Name of the pattern (e.g., "async_handler")
            code: The code pattern
            context: Context where this pattern is used
            success_rate: Success rate (0.0-1.0)
            metadata: Optional additional metadata
            
        Returns:
            Node ID
            
        Example:
            >>> memory = AIMemoryHelper()
            >>> node_id = memory.store_pattern(
            ...     "async_handler",
            ...     "async def handle(req): ...",
            ...     context="HTTP server",
            ...     success_rate=0.95
            ... )
        """
        data = {
            "code": code,
            "context": context,
            "success_rate": success_rate,
            "metadata": metadata or {}
        }
        return self.backend.add_node(
            f"PATTERN:{pattern_name}",
            json.dumps(data),
            node_type=LATTICE_NODE_PATTERN
        )
    
    def store_constraint(self, constraint_name: str, description: str) -> int:
        """
        Store a project constraint.
        
        Args:
            constraint_name: Name of constraint (e.g., "no_regex")
            description: Description of the constraint
            
        Returns:
            Node ID
            
        Example:
            >>> memory = AIMemoryHelper()
            >>> memory.store_constraint("no_regex", "User prefers semantic reasoning")
        """
        return self.backend.add_node(
            f"CONSTRAINT:{constraint_name}",
            description,
            node_type=LATTICE_NODE_ANTI_PATTERN
        )
    
    def store_success(
        self,
        task_id: str,
        solution: str,
        metadata: Optional[Dict] = None
    ) -> int:
        """
        Store a successful task completion.
        
        Args:
            task_id: Unique task identifier
            solution: Description of the solution
            metadata: Optional additional metadata
            
        Returns:
            Node ID
        """
        data = {
            "solution": solution,
            "success": True,
            "metadata": metadata or {}
        }
        return self.backend.add_node(
            f"TASK:{task_id}",
            json.dumps(data),
            node_type=LATTICE_NODE_LEARNING
        )
    
    def store_failure(
        self,
        error_type: str,
        error: str,
        context: str = "",
        avoid: str = ""
    ) -> int:
        """
        Store a failure to avoid repeating.
        
        Args:
            error_type: Type of error (e.g., "regex_approach")
            error: Error description
            context: Context where error occurred
            avoid: What to avoid in the future
            
        Returns:
            Node ID
        """
        data = {
            "error": error,
            "context": context,
            "avoid": avoid
        }
        return self.backend.add_node(
            f"FAILURE:{error_type}",
            json.dumps(data),
            node_type=LATTICE_NODE_ANTI_PATTERN
        )
    
    def get_failures(self, error_type: Optional[str] = None, limit: int = 20) -> List[Dict[str, Any]]:
        """
        Get stored failures.
        
        Args:
            error_type: Optional error type to filter
            limit: Maximum results
            
        Returns:
            List of failure dictionaries
        """
        if error_type:
            prefix = f"FAILURE:{error_type}"
        else:
            prefix = "FAILURE:"
        
        results = self.backend.find_by_prefix(prefix, limit=limit)
        failures = []
        for r in results:
            try:
                data = json.loads(r.get("data", "{}"))
                failures.append({
                    "name": r.get("name", ""),
                    "data": data,
                    "id": r.get("id")
                })
            except json.JSONDecodeError:
                failures.append({
                    "name": r.get("name", ""),
                    "data": {"error": r.get("data", "")},
                    "id": r.get("id")
                })
        return failures
    
    def check_before_generate(self) -> Dict[str, Any]:
        """
        Check memory before generating code.
        
        Returns a summary of:
        - Constraints to follow
        - Similar patterns to consider
        - Failures to avoid
        
        Example:
            >>> memory = AIMemoryHelper()
            >>> context = memory.check_before_generate()
            >>> print(f"Found {len(context['constraints'])} constraints")
            >>> print(f"Found {len(context['patterns'])} similar patterns")
        """
        return {
            "constraints": self.get_constraints(),
            "patterns": self.get_patterns(limit=10),
            "failures": self.get_failures(limit=10)
        }


# Convenience function
def get_memory() -> AIMemoryHelper:
    """
    Get AI memory helper (convenience function).
    
    Example:
        >>> from synrix.auto_memory import get_memory
        >>> memory = get_memory()
        >>> constraints = memory.get_constraints()
    """
    return AIMemoryHelper()
