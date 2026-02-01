#!/usr/bin/env python3
"""
SYNRIX CLI Python Wrapper for Cursor AI
Zero-dependency: uses subprocess to call synrix_cli binary

Usage:
    from cursor_synrix_cli import Synrix
    
    synrix = Synrix("/path/to/synrix_cli", "cursor_mem.lattice")
    synrix.write("task:fix_bug", "Add colon after if")
    result = synrix.read("task:fix_bug")
    results = synrix.search("task:")
"""

import subprocess
import json
import os
from typing import Optional, Dict, List, Any

class Synrix:
    """Zero-dependency SYNRIX client using CLI binary"""
    
    def __init__(self, cli_path: str, lattice_path: str, daemon: bool = True):
        """
        Initialize SYNRIX client
        
        Args:
            cli_path: Path to synrix_cli binary
            lattice_path: Path to lattice file
            daemon: If True, use daemon mode (faster, persistent)
        """
        self.cli_path = cli_path
        self.lattice_path = lattice_path
        self.daemon = daemon
        self.daemon_proc = None
        
        if daemon:
            self._start_daemon()
    
    def _start_daemon(self):
        """Start daemon mode process"""
        if not os.path.exists(self.lattice_path):
            # Initialize lattice
            subprocess.run(
                [self.cli_path, "init", self.lattice_path],
                check=True,
                capture_output=True
            )
        
        self.daemon_proc = subprocess.Popen(
            [self.cli_path, "daemon", self.lattice_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,  # Suppress debug output
            text=True,
            bufsize=1
        )
    
    def _daemon_request(self, op: str, **kwargs) -> Dict[str, Any]:
        """Send request to daemon and get response"""
        if not self.daemon_proc:
            raise RuntimeError("Daemon not started")
        
        request = json.dumps({"op": op, **kwargs})
        self.daemon_proc.stdin.write(request + "\n")
        self.daemon_proc.stdin.flush()
        
        # Read lines until we get a JSON response (starts with '{')
        # Skip debug output lines
        while True:
            response_line = self.daemon_proc.stdout.readline()
            if not response_line:
                raise RuntimeError("Daemon process ended")
            
            response_line = response_line.strip()
            if not response_line:
                continue
            
            # Check if it's JSON (starts with '{')
            if response_line.startswith('{'):
                try:
                    return json.loads(response_line)
                except json.JSONDecodeError as e:
                    raise RuntimeError(f"Invalid JSON response: {response_line[:100]}") from e
    
    def _single_call(self, command: str, *args) -> Dict[str, Any]:
        """Single-call mode (slower, but simpler)"""
        cmd = [self.cli_path, command, self.lattice_path] + list(args)
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False
        )
        
        if result.returncode != 0:
            return {
                "success": False,
                "error": result.stderr.strip() or "Command failed",
                "data": None
            }
        
        return json.loads(result.stdout)
    
    def write(self, key: str, value: str) -> Optional[int]:
        """
        Write a key-value pair
        
        Returns:
            node_id if successful, None otherwise
        """
        if self.daemon:
            response = self._daemon_request("write", key=key, value=value)
        else:
            response = self._single_call("write", key, value)
        
        if response.get("success"):
            return response.get("data", {}).get("node_id")
        return None
    
    def read(self, key: str) -> Optional[Dict[str, Any]]:
        """
        Read a value by key
        
        Returns:
            {"id": node_id, "key": key, "value": value} or None
        """
        if self.daemon:
            response = self._daemon_request("read", key=key)
        else:
            response = self._single_call("read", key)
        
        if response.get("success"):
            return response.get("data")
        return None
    
    def get(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        O(1) direct lookup by node ID
        
        Returns:
            {"id": node_id, "key": key, "value": value} or None
        """
        if self.daemon:
            response = self._daemon_request("get", node_id=node_id)
        else:
            response = self._single_call("get", str(node_id))
        
        if response.get("success"):
            return response.get("data")
        return None
    
    def search(self, prefix: str, limit: int = 100) -> List[Dict[str, Any]]:
        """
        O(k) semantic query by prefix
        
        Returns:
            List of {"id": node_id, "key": key, "value": value}
        """
        if self.daemon:
            response = self._daemon_request("search", prefix=prefix, limit=limit)
        else:
            response = self._single_call("search", prefix, str(limit))
        
        if response.get("success"):
            return response.get("data", {}).get("results", [])
        return []
    
    def close(self):
        """Close daemon connection"""
        if self.daemon_proc:
            self.daemon_proc.stdin.close()
            self.daemon_proc.wait()
            self.daemon_proc = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


# Example usage
if __name__ == "__main__":
    # Find synrix_cli (assume it's in the same directory or PATH)
    import sys
    cli_path = os.path.join(os.path.dirname(__file__), "..", "synrix_cli")
    if not os.path.exists(cli_path):
        cli_path = "synrix_cli"  # Try PATH
    
    lattice_path = "/tmp/cursor_synrix.lattice"
    
    print("SYNRIX CLI Python Wrapper Demo")
    print("=" * 50)
    
    with Synrix(cli_path, lattice_path, daemon=True) as synrix:
        # Write
        print("\n1. Writing key-value pair...")
        node_id = synrix.write("task:fix_bug", "Add colon after if statement")
        print(f"   ✓ Written (node_id: {node_id})")
        
        # Read
        print("\n2. Reading by key...")
        result = synrix.read("task:fix_bug")
        if result:
            print(f"   ✓ Found: {result['value']}")
        
        # O(1) lookup
        print("\n3. O(1) direct lookup by node ID...")
        if node_id:
            result = synrix.get(node_id)
            if result:
                print(f"   ✓ Found: {result['value']}")
        
        # Search
        print("\n4. Searching by prefix...")
        results = synrix.search("task:")
        print(f"   ✓ Found {len(results)} results")
        for r in results:
            print(f"      - {r['key']}: {r['value']}")
    
    print("\n" + "=" * 50)
    print("Demo complete!")

