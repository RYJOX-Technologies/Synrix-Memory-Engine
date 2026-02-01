#!/usr/bin/env python3
"""
Query SYNRIX lattice to see what work has been done.
"""

import os
import sys
import json

# Add Python SDK to path
script_dir = os.path.dirname(os.path.abspath(__file__))
sdk_path = os.path.join(script_dir, "python-sdk")
if os.path.exists(sdk_path):
    sys.path.insert(0, sdk_path)

try:
    from synrix.raw_backend import RawSynrixBackend
except ImportError:
    print("WARNING: SYNRIX Python SDK not found. Cannot query lattice.")
    print("   Install SDK: cd python-sdk && pip install -e .")
    sys.exit(1)

def query_work(lattice_path):
    """Query lattice for work-related nodes."""
    if not os.path.exists(lattice_path):
        print(f"ERROR: Lattice file not found: {lattice_path}")
        return
    
    print(f"Opening lattice: {lattice_path}")
    try:
        backend = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    except Exception as e:
        print(f"ERROR: Failed to open lattice: {e}")
        return
    
    # Common prefixes for work tracking
    prefixes = {
        "TASK:": "Tasks/Work Items",
        "PROJECT:": "Projects",
        "WORK:": "Work Items",
        "WINDOWS:": "Windows-specific work",
        "BUILD:": "Build-related work",
        "FEATURE:": "Features",
        "FIX:": "Fixes",
        "CONSTRAINT:": "Constraints/Rules",
        "PATTERN:": "Code Patterns",
        "FAILURE:": "Failures/Issues",
        "LEARNING:": "Learning/Knowledge",
    }
    
    print("\n" + "="*70)
    print("SYNRIX LATTICE WORK SUMMARY")
    print("="*70 + "\n")
    
    total_nodes = 0
    
    for prefix, description in prefixes.items():
        try:
            results = backend.find_by_prefix(prefix, limit=50, raw=False)
            if results:
                print(f"{description} ({prefix})")
                print(f"   Found {len(results)} nodes\n")
                
                for i, node in enumerate(results[:10], 1):  # Show first 10
                    name = node.get('name', '')
                    data = node.get('data', '')
                    
                    # Try to parse JSON data
                    try:
                        if isinstance(data, bytes):
                            data_str = data.decode('utf-8', errors='ignore')
                        else:
                            data_str = str(data)
                        
                        # Try to parse as JSON
                        try:
                            data_obj = json.loads(data_str)
                            if isinstance(data_obj, dict):
                                # Pretty print JSON
                                data_preview = json.dumps(data_obj, indent=2)[:200]
                                if len(json.dumps(data_obj)) > 200:
                                    data_preview += "..."
                            else:
                                data_preview = data_str[:200]
                        except:
                            data_preview = data_str[:200] if len(data_str) > 200 else data_str
                    except:
                        data_preview = str(data)[:200]
                    
                    print(f"   {i}. {name}")
                    if data_preview:
                        # Indent data preview
                        for line in data_preview.split('\n'):
                            print(f"      {line}")
                    print()
                
                if len(results) > 10:
                    print(f"   ... and {len(results) - 10} more\n")
                
                total_nodes += len(results)
        except Exception as e:
            print(f"WARNING: Error querying {prefix}: {e}\n")
    
    print("="*70)
    print(f"Total work-related nodes found: {total_nodes}")
    print("="*70)
    
    backend.close()

if __name__ == "__main__":
    # Try to find lattice file
    script_dir = os.path.dirname(os.path.abspath(__file__))
    lattice_path = os.path.join(script_dir, "lattice", "cursor_ai_memory.lattice")
    
    if not os.path.exists(lattice_path):
        # Try user home directory
        home_lattice = os.path.expanduser("~/.cursor_ai_memory.lattice")
        if os.path.exists(home_lattice):
            lattice_path = home_lattice
        else:
            print(f"ERROR: Lattice file not found. Tried:")
            print(f"   {lattice_path}")
            print(f"   {home_lattice}")
            sys.exit(1)
    
    query_work(lattice_path)
