#!/usr/bin/env python3
"""
Direct usage example for Cursor AI agent

This shows how Cursor AI (the agent) can use SYNRIX directly
without any Python SDK dependencies - just the binary.
"""

import subprocess
import json
import os
from pathlib import Path

# Find synrix_cli binary
def find_synrix_cli():
    """Find synrix_cli binary in common locations"""
    # Try current directory first
    current_dir = Path(__file__).parent.parent
    cli_path = current_dir / "synrix_cli"
    if cli_path.exists():
        return str(cli_path)
    
    # Try PATH
    import shutil
    path_cli = shutil.which("synrix_cli")
    if path_cli:
        return path_cli
    
    raise FileNotFoundError("synrix_cli binary not found. Build it with: make -f Makefile.synrix_cli")

# Lattice path for Cursor AI agent memory
LATTICE_PATH = os.path.expanduser("~/.cursor/synrix_memory.lattice")

def synrix_write(key: str, value: str) -> dict:
    """Write a key-value pair to SYNRIX"""
    cli_path = find_synrix_cli()
    result = subprocess.run(
        [cli_path, "write", LATTICE_PATH, key, value],
        capture_output=True,
        text=True
    )
    if result.returncode == 0:
        # Filter out debug output, find JSON line
        for line in result.stdout.split('\n'):
            line = line.strip()
            if line.startswith('{'):
                return json.loads(line)
        return {"success": False, "error": "No JSON response found"}
    else:
        return {"success": False, "error": result.stderr}

def synrix_read(key: str) -> dict:
    """Read a value by key from SYNRIX"""
    cli_path = find_synrix_cli()
    result = subprocess.run(
        [cli_path, "read", LATTICE_PATH, key],
        capture_output=True,
        text=True
    )
    if result.returncode == 0:
        # Filter out debug output, find JSON line
        for line in result.stdout.split('\n'):
            line = line.strip()
            if line.startswith('{'):
                return json.loads(line)
        return {"success": False, "error": "No JSON response found"}
    else:
        return {"success": False, "error": result.stderr}

def synrix_search(prefix: str, limit: int = 100) -> dict:
    """Search by prefix in SYNRIX"""
    cli_path = find_synrix_cli()
    result = subprocess.run(
        [cli_path, "search", LATTICE_PATH, prefix, str(limit)],
        capture_output=True,
        text=True
    )
    if result.returncode == 0:
        # Filter out debug output, find JSON line (may be multi-line)
        output = result.stdout.strip()
        # Find the JSON object (starts with {)
        json_start = output.find('{')
        if json_start >= 0:
            json_str = output[json_start:]
            return json.loads(json_str)
        return {"success": False, "error": "No JSON response found"}
    else:
        return {"success": False, "error": result.stderr}

def synrix_get(node_id: int) -> dict:
    """O(1) direct lookup by node ID"""
    cli_path = find_synrix_cli()
    result = subprocess.run(
        [cli_path, "get", LATTICE_PATH, str(node_id)],
        capture_output=True,
        text=True
    )
    if result.returncode == 0:
        # Filter out debug output, find JSON line
        for line in result.stdout.split('\n'):
            line = line.strip()
            if line.startswith('{'):
                return json.loads(line)
        return {"success": False, "error": "No JSON response found"}
    else:
        return {"success": False, "error": result.stderr}

# Example: How Cursor AI agent would use this
if __name__ == "__main__":
    print("Cursor AI Agent - Direct SYNRIX Usage")
    print("=" * 50)
    
    # Initialize lattice if needed
    cli_path = find_synrix_cli()
    if not os.path.exists(LATTICE_PATH):
        print(f"Initializing lattice at {LATTICE_PATH}...")
        subprocess.run([cli_path, "init", LATTICE_PATH], check=True)
    
    # Example 1: Remember a code pattern
    print("\n1. Remembering a code pattern...")
    result = synrix_write("pattern:python:error_handling", "Use try/except with specific exceptions")
    if result.get("success"):
        node_id = result.get("data", {}).get("node_id")
        print(f"   ✓ Stored (node_id: {node_id})")
        print(f"   Latency: {result.get('latency_us', 0)}µs")
    
    # Example 2: Recall the pattern
    print("\n2. Recalling the pattern...")
    result = synrix_read("pattern:python:error_handling")
    if result.get("success"):
        data = result.get("data", {})
        print(f"   ✓ Found: {data.get('value')}")
        print(f"   Latency: {result.get('latency_us', 0)}µs")
    
    # Example 3: Search for related patterns
    print("\n3. Searching for related patterns...")
    result = synrix_search("pattern:python:", 10)
    if result.get("success"):
        data = result.get("data", {})
        results = data.get("results", [])
        print(f"   ✓ Found {len(results)} patterns")
        for r in results:
            print(f"      - {r['key']}: {r['value']}")
        print(f"   Latency: {result.get('latency_us', 0)}µs")
    
    # Example 4: O(1) direct lookup
    if result.get("success") and result.get("data", {}).get("results"):
        first_result = result.get("data", {}).get("results", [])[0]
        node_id = first_result.get("id")
        if node_id:
            print(f"\n4. O(1) direct lookup by node ID {node_id}...")
            result = synrix_get(node_id)
            if result.get("success"):
                data = result.get("data", {})
                print(f"   ✓ Found: {data.get('value')}")
                print(f"   Latency: {result.get('latency_us', 0)}µs")
    
    print("\n" + "=" * 50)
    print("Cursor AI can now use SYNRIX for persistent memory!")

