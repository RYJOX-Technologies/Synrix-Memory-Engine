#!/usr/bin/env python3
"""
Import SYNRIX context from JSON export (for Windows transition).
Run this on Windows after transferring synrix_context_export.json
"""

import json
import os
import sys

# Add Python SDK to path
script_dir = os.path.dirname(os.path.abspath(__file__))
sdk_path = os.path.join(script_dir, "..", "..", "python-sdk")
if os.path.exists(sdk_path):
    sys.path.insert(0, sdk_path)

try:
    from synrix.raw_backend import RawSynrixBackend
except ImportError:
    print("⚠️  SYNRIX Python SDK not found. Context will be imported after DLL is built.")
    print("   For now, you can view the JSON file directly.")
    sys.exit(0)

def import_context(json_file):
    """Import context from JSON export into Windows SYNRIX lattice."""
    if not os.path.exists(json_file):
        print(f"❌ Context file not found: {json_file}")
        return False
    
    # Initialize Windows SYNRIX (will create new lattice if needed)
    memory_path = os.path.expanduser("~/.cursor_ai_memory.lattice")
    memory = RawSynrixBackend(memory_path, max_nodes=25000)
    
    # Load JSON export
    with open(json_file, 'r', encoding='utf-8') as f:
        export_data = json.load(f)
    
    imported = 0
    
    # Import Windows build context
    if export_data.get("windows_build_context"):
        context = export_data["windows_build_context"]
        memory.add_node(
            "TASK:windows_build_transition",
            json.dumps(context),
            node_type=5  # LATTICE_NODE_LEARNING
        )
        imported += 1
        print(f"✅ Imported Windows build context")
    
    # Import constraints
    for constraint in export_data.get("constraints", []):
        try:
            memory.add_node(
                constraint["id"],
                constraint["data"],
                node_type=6  # LATTICE_NODE_ANTI_PATTERN (constraints)
            )
            imported += 1
        except Exception as e:
            print(f"⚠️  Failed to import {constraint['id']}: {e}")
    
    # Import patterns
    for pattern in export_data.get("patterns", []):
        try:
            memory.add_node(
                pattern["id"],
                pattern["data"],
                node_type=3  # LATTICE_NODE_PATTERN
            )
            imported += 1
        except Exception as e:
            print(f"⚠️  Failed to import {pattern['id']}: {e}")
    
    # Import failures
    for failure in export_data.get("failures", []):
        try:
            memory.add_node(
                failure["id"],
                failure["data"],
                node_type=6  # LATTICE_NODE_ANTI_PATTERN
            )
            imported += 1
        except Exception as e:
            print(f"⚠️  Failed to import {failure['id']}: {e}")
    
    print(f"\n✅ Import complete: {imported} nodes imported to {memory_path}")
    return True

if __name__ == "__main__":
    # Look for context file in same directory or Downloads
    script_dir = os.path.dirname(os.path.abspath(__file__))
    json_file = os.path.join(script_dir, "synrix_context_export.json")
    
    if not os.path.exists(json_file):
        downloads = os.path.join(os.path.expanduser("~"), "Downloads", "synrix_context_export.json")
        if os.path.exists(downloads):
            json_file = downloads
        else:
            print(f"❌ Context file not found. Expected:")
            print(f"   {json_file}")
            print(f"   or {downloads}")
            sys.exit(1)
    
    import_context(json_file)
