#!/usr/bin/env python3
"""
Example: How to integrate SYNRIX into your actual workflow

This shows a realistic example of using SYNRIX in a development tool
"""

import subprocess
import json
import os

# Configuration
SYNRIX_CLI = "./synrix_cli"  # or "/usr/local/bin/synrix_cli" if installed
LATTICE = os.path.expanduser("~/.cursor/synrix_memory.lattice")

def synrix(op, **params):
    """
    Call SYNRIX - works with both single-call and daemon mode
    """
    if op in ["write", "read", "get", "search"]:
        # Single-call mode
        if op == "write":
            cmd = [SYNRIX_CLI, "write", LATTICE, params["key"], params["value"]]
        elif op == "read":
            cmd = [SYNRIX_CLI, "read", LATTICE, params["key"]]
        elif op == "get":
            cmd = [SYNRIX_CLI, "get", LATTICE, str(params["node_id"])]
        elif op == "search":
            cmd = [SYNRIX_CLI, "search", LATTICE, params["prefix"], str(params.get("limit", 10))]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # Parse JSON from output (filter debug messages)
        for line in result.stdout.split('\n'):
            line = line.strip()
            if line.startswith('{'):
                try:
                    return json.loads(line)
                except:
                    continue
        return {"success": False, "error": "No valid JSON response"}
    
    return {"success": False, "error": "Unknown operation"}

# Example 1: Remember a code pattern
def remember_pattern(pattern_name, code_example):
    """Store a code pattern for later recall"""
    result = synrix("write", key=f"pattern:{pattern_name}", value=code_example)
    if result.get("success"):
        print(f"✓ Remembered pattern: {pattern_name}")
        return result.get("data", {}).get("node_id")
    else:
        print(f"✗ Failed to remember pattern: {result.get('error')}")
        return None

# Example 2: Recall a pattern
def recall_pattern(pattern_name):
    """Recall a stored code pattern"""
    result = synrix("read", key=f"pattern:{pattern_name}")
    if result.get("success"):
        value = result.get("data", {}).get("value")
        print(f"✓ Found pattern: {value}")
        return value
    else:
        print(f"✗ Pattern not found: {pattern_name}")
        return None

# Example 3: Remember an error fix
def remember_error_fix(error_type, fix):
    """Store how to fix a specific error"""
    result = synrix("write", key=f"error:{error_type}", value=fix)
    if result.get("success"):
        print(f"✓ Remembered fix for: {error_type}")
        return True
    return False

# Example 4: Get fix for an error
def get_error_fix(error_type):
    """Get the stored fix for an error"""
    result = synrix("read", key=f"error:{error_type}")
    if result.get("success"):
        return result.get("data", {}).get("value")
    return None

# Example 5: Search for related patterns
def search_patterns(prefix):
    """Search for patterns matching a prefix"""
    result = synrix("search", prefix=f"pattern:{prefix}", limit=10)
    if result.get("success"):
        results = result.get("data", {}).get("results", [])
        return results
    return []

# Example usage in a real workflow
if __name__ == "__main__":
    print("=== SYNRIX Workflow Integration Example ===\n")
    
    # Initialize lattice if needed
    if not os.path.exists(LATTICE):
        print("Initializing lattice...")
        subprocess.run([SYNRIX_CLI, "init", LATTICE], check=True, capture_output=True)
        print("✓ Lattice initialized\n")
    
    # Scenario: Learning from a coding session
    print("1. Storing learned patterns...")
    remember_pattern("python:async", "Use asyncio.create_task() for concurrent operations")
    remember_pattern("python:error_handling", "Use try/except with specific exception types")
    remember_error_fix("python:syntax:missing_colon", "Add ':' after if/for/while statements")
    print()
    
    # Scenario: Recalling in a new session
    print("2. Recalling patterns in a new session...")
    pattern = recall_pattern("python:async")
    print()
    
    # Scenario: Encountering an error
    print("3. Encountering an error...")
    error_type = "python:syntax:missing_colon"
    fix = get_error_fix(error_type)
    if fix:
        print(f"   → Known error! Fix: {fix}")
        print("   → Would apply this fix automatically")
    else:
        print(f"   → Unknown error: {error_type}")
        print("   → Would need to fix manually")
    print()
    
    # Scenario: Searching for related knowledge
    print("4. Searching for related patterns...")
    patterns = search_patterns("python:")
    print(f"   → Found {len(patterns)} related patterns")
    for p in patterns[:3]:  # Show first 3
        print(f"      - {p.get('key')}: {p.get('value')[:50]}...")
    print()
    
    print("=== Example Complete ===")
    print(f"\nYour memory is stored in: {LATTICE}")
    print("This persists across sessions!")

