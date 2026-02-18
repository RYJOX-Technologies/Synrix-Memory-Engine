"""
AI Memory Interface for SYNRIX

Allows AI assistants to directly interact with SYNRIX memory lattice
without requiring user intervention.

Uses raw_backend for direct DLL access (faster, more reliable than CLI).

Usage:
    from synrix.ai_memory import AIMemory
    
    memory = AIMemory()
    memory.add("key", "value")
    results = memory.query("prefix")
"""

import os
from typing import Optional, Dict, List, Any
from pathlib import Path

# Use raw_backend for direct DLL access (Windows-native)
try:
    from .raw_backend import RawSynrixBackend
    USE_RAW_BACKEND = True
except ImportError:
    USE_RAW_BACKEND = False
    import subprocess
    import json
    import platform


class AIMemory:
    """
    Direct AI access to SYNRIX memory lattice.
    
    Uses the synrix.exe CLI to interact with the memory lattice,
    allowing AI assistants to store and retrieve memories directly.
    """
    
    def __init__(self, lattice_path: Optional[str] = None, cli_path: Optional[str] = None):
        """
        Initialize AI memory interface.
        
        Args:
            lattice_path: Path to lattice file (default: ~/.synrix_ai_memory.lattice)
            cli_path: Path to synrix.exe (only used if raw_backend unavailable)
        """
        self.lattice_path = lattice_path or str(Path.home() / ".synrix_ai_memory.lattice")
        
        if USE_RAW_BACKEND:
            # Use direct DLL access (faster, more reliable)
            self.backend = RawSynrixBackend(self.lattice_path, max_nodes=25000, evaluation_mode=True)
            self.cli_path = None
        else:
            # Fallback to CLI
            self.cli_path = cli_path or self._find_cli()
            self.backend = None
    
    def _find_cli(self) -> str:
        """Find synrix.exe in common locations"""
        # Check environment variable first
        env_path = os.environ.get("SYNRIX_CLI_PATH")
        if env_path and os.path.exists(env_path):
            return env_path
        
        # Check relative to this file
        current_file = Path(__file__).resolve()
        # python-sdk/synrix/ai_memory.py -> project_root
        project_root = current_file.parent.parent.parent
        
        # Common locations
        search_paths = [
            project_root / "build" / "windows" / "build_msys2" / "bin" / "synrix.exe",
            project_root / "build" / "windows" / "build" / "bin" / "synrix.exe",
            project_root / "bin" / "synrix.exe",
            Path.cwd() / "build" / "windows" / "build_msys2" / "bin" / "synrix.exe",
        ]
        
        for path in search_paths:
            if path.exists():
                return str(path)
        
        raise FileNotFoundError(
            "synrix.exe not found. Set SYNRIX_CLI_PATH environment variable "
            "or ensure synrix.exe is in a standard location."
        )
    
    def _run_cli(self, command: str, *args) -> Dict[str, Any]:
        """Run synrix.exe command and return JSON result"""
        cmd = [self.cli_path, command, self.lattice_path] + list(args)
        
        # Set PATH to include DLL directory
        env = os.environ.copy()
        cli_dir = os.path.dirname(self.cli_path)
        if cli_dir not in env.get("PATH", ""):
            env["PATH"] = cli_dir + os.pathsep + env.get("PATH", "")
        
        try:
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,  # Capture stderr to filter debug messages
                text=True,
                timeout=10,
                env=env  # Use modified environment with DLL path
            )
            
            if result.returncode != 0:
                # Try to extract error from stderr if available
                error_msg = result.stderr or result.stdout
                return {"success": False, "error": error_msg[:200]}
            
            # Parse JSON from output (filter out debug messages)
            # Look for JSON in both stdout and stderr (debug messages go to stderr)
            output = result.stdout + "\n" + (result.stderr or "")
            
            for line in output.split('\n'):
                line = line.strip()
                if line.startswith('{') and 'success' in line:
                    try:
                        parsed = json.loads(line)
                        if isinstance(parsed, dict) and 'success' in parsed:
                            return parsed
                    except json.JSONDecodeError:
                        continue
            
            # If no JSON found, return error with output for debugging
            return {"success": False, "error": f"No valid JSON in output. Output: {output[:200]}"}
            
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Command timeout"}
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def add(self, name: str, data: str) -> Optional[int]:
        """
        Add a memory to the lattice.
        
        Args:
            name: Memory key/name (e.g., "AI_MEMORY:user_preference")
            data: Memory data/value
        
        Returns:
            Node ID if successful, None otherwise
        """
        if USE_RAW_BACKEND and self.backend:
            try:
                node_id = self.backend.add_node(name, data, node_type=5)  # LATTICE_NODE_LEARNING
                if node_id:
                    self.backend.save()  # Auto-save
                return node_id if node_id else None
            except Exception as e:
                return None
        
        # Fallback to CLI
        result = self._run_cli("add", name, data)
        if result.get("success"):
            return result.get("node_id")
        return None
    
    def get(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        Get a memory by node ID (O(1) lookup).
        
        Args:
            node_id: Node ID to retrieve
        
        Returns:
            Node data dict or None if not found
        """
        if USE_RAW_BACKEND and self.backend:
            try:
                node = self.backend.get_node(node_id)
                if node:
                    return {
                        "id": node.get("id"),
                        "name": node.get("name"),
                        "data": node.get("data")
                    }
            except Exception:
                pass
            return None
        
        # Fallback to CLI
        result = self._run_cli("get", str(node_id))
        if result.get("success"):
            return {
                "id": result.get("id"),
                "name": result.get("name"),
                "data": result.get("data")
            }
        return None
    
    def query(self, prefix: str, limit: int = 100) -> List[Dict[str, Any]]:
        """
        Query memories by prefix (O(k) semantic search).
        
        Args:
            prefix: Prefix to search for (e.g., "AI_MEMORY:")
            limit: Maximum number of results
        
        Returns:
            List of matching nodes
        """
        if USE_RAW_BACKEND and self.backend:
            try:
                results = self.backend.find_by_prefix(prefix, limit=limit, raw=False)
                return [
                    {
                        "id": r.get("id"),
                        "name": r.get("name") if isinstance(r.get("name"), str) else r.get("name", b"").decode('utf-8', errors='ignore'),
                        "data": r.get("data") if isinstance(r.get("data"), str) else r.get("data", b"").decode('utf-8', errors='ignore')
                    }
                    for r in results
                ]
            except Exception:
                return []
        
        # Fallback to CLI
        result = self._run_cli("query", prefix, str(limit))
        if result.get("success"):
            return result.get("nodes", [])
        return []
    
    def count(self) -> int:
        """
        Count all memories in the lattice.
        
        Returns:
            Total number of nodes
        """
        if USE_RAW_BACKEND and self.backend:
            try:
                results = self.backend.find_by_prefix("", limit=30000)
                return len(results)
            except Exception:
                return 0
        
        # Fallback to CLI
        result = self._run_cli("count")
        if result.get("success"):
            return result.get("count", 0)
        return 0
    
    def list_all(self, prefix: str = "") -> List[Dict[str, Any]]:
        """
        List all memories (optionally filtered by prefix).
        
        Args:
            prefix: Optional prefix filter
        
        Returns:
            List of all nodes
        """
        return self.query(prefix, limit=10000)
    
    def close(self):
        """Close the memory interface and cleanup resources"""
        if USE_RAW_BACKEND and self.backend:
            self.backend.close()


# Convenience function for AI assistants
def get_ai_memory(lattice_path: Optional[str] = None) -> AIMemory:
    """
    Get AI memory interface instance.
    
    This is the recommended way for AI assistants to access SYNRIX memory.
    
    Example:
        memory = get_ai_memory()
        memory.add("AI_MEMORY:user_name", "Alice")
        results = memory.query("AI_MEMORY:")
    """
    return AIMemory(lattice_path=lattice_path)
