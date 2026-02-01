#!/usr/bin/env python3
"""
Store actual codebase in SYNRIX
TOS-compliant: User's workflow explicitly stores their own code
"""

import subprocess
import json
import os
from pathlib import Path

SYNRIX_CLI = "./synrix_cli"
LATTICE = os.path.expanduser("~/.cursor/codebase.lattice")

def synrix_write(key, value):
    """Write to SYNRIX"""
    result = subprocess.run(
        [SYNRIX_CLI, "write", LATTICE, key, value],
        capture_output=True,
        text=True
    )
    for line in result.stdout.split('\n'):
        if line.strip().startswith('{'):
            return json.loads(line.strip())
    return {"success": False}

def synrix_read(key):
    """Read from SYNRIX"""
    result = subprocess.run(
        [SYNRIX_CLI, "read", LATTICE, key],
        capture_output=True,
        text=True
    )
    for line in result.stdout.split('\n'):
        if line.strip().startswith('{'):
            return json.loads(line.strip())
    return {"success": False}

def store_file_function(file_path, function_name, code):
    """Store a function from a file"""
    key = f"file:{file_path}:function:{function_name}"
    return synrix_write(key, code)

def store_file_class(file_path, class_name, code):
    """Store a class from a file"""
    key = f"file:{file_path}:class:{class_name}"
    return synrix_write(key, code)

def get_file_function(file_path, function_name):
    """Get a function from a file"""
    key = f"file:{file_path}:function:{function_name}"
    result = synrix_read(key)
    if result.get("success"):
        return result.get("data", {}).get("value")
    return None

def search_functions(pattern):
    """Search for functions matching a pattern"""
    result = subprocess.run(
        [SYNRIX_CLI, "search", LATTICE, f"file::function:{pattern}", "100"],
        capture_output=True,
        text=True
    )
    output = result.stdout.strip()
    json_start = output.find('{')
    if json_start >= 0:
        data = json.loads(output[json_start:])
        if data.get("success"):
            return data.get("data", {}).get("results", [])
    return []

# Example: Store actual code from this project
if __name__ == "__main__":
    print("=== Storing Codebase in SYNRIX ===\n")
    
    # Initialize
    if not os.path.exists(LATTICE):
        subprocess.run([SYNRIX_CLI, "init", LATTICE], check=True, capture_output=True)
        print("✓ Lattice initialized\n")
    
    # Example 1: Store a function from synrix_helper.py
    helper_path = "synrix_helper.py"
    if os.path.exists(helper_path):
        with open(helper_path, 'r') as f:
            code = f.read()
            # Extract a function (simplified - in real use, use AST parser)
            if "def synrix_write" in code:
                func_code = code[code.find("def synrix_write"):code.find("def synrix_read")]
                result = store_file_function(helper_path, "synrix_write", func_code)
                if result.get("success"):
                    print(f"✓ Stored function: {helper_path}:synrix_write")
    
    # Example 2: Store a real function from the codebase
    test_code = '''def process_data(data):
    """Process data and return result"""
    result = data.upper()
    return result'''
    
    result = store_file_function("src/utils.py", "process_data", test_code)
    if result.get("success"):
        print(f"✓ Stored function: src/utils.py:process_data")
    
    # Example 3: Retrieve it
    print("\n=== Retrieving Stored Code ===\n")
    code = get_file_function("src/utils.py", "process_data")
    if code:
        print(f"✓ Retrieved:\n{code}\n")
    
    # Example 4: Search for functions
    print("=== Searching for Functions ===\n")
    functions = search_functions("process")
    print(f"✓ Found {len(functions)} functions matching 'process'")
    for f in functions[:3]:
        print(f"  - {f.get('key')}: {f.get('value')[:50]}...")
    
    print("\n=== Codebase Storage Complete ===")
    print(f"\nYour codebase is stored in: {LATTICE}")
    print("This is TOS-compliant because:")
    print("  - You explicitly store your own code")
    print("  - SYNRIX is an external tool")
    print("  - No automatic code capture")

