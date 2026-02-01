#!/usr/bin/env python3
"""
Example: Using SYNRIX directly in Cursor AI Agent

This demonstrates how Cursor AI agents can use SYNRIX for persistent memory.
"""

import sys
import os

# Add parent directory to path for direct execution
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, parent_dir)

from synrix.agent_backend import get_synrix_backend


def main():
    print("=" * 60)
    print("Cursor AI Agent - SYNRIX Integration Example")
    print("=" * 60)
    print()
    
    # Get backend (automatically uses best available)
    backend = get_synrix_backend(collection="cursor_agent_memory")
    print(f"✓ Backend initialized: {backend.backend_type}")
    print()
    
    # Example 1: Store learned patterns
    print("1. Storing learned code patterns...")
    backend.write("pattern:missing_colon", {
        "error": "SyntaxError: invalid syntax",
        "pattern": "if statement without colon",
        "fix": "add colon after if condition",
        "success_rate": 0.95
    })
    print("   ✓ Stored pattern: missing_colon")
    
    # Example 2: Store task-specific memory
    print("\n2. Storing task memory...")
    backend.write("task:fix_bug:file_123", {
        "file": "main.py",
        "line": 42,
        "error": "NameError: name 'x' is not defined",
        "fix": "import x or define x",
        "success": True
    })
    print("   ✓ Stored task memory")
    
    # Example 3: Retrieve memory
    print("\n3. Retrieving memory...")
    pattern = backend.read("pattern:missing_colon")
    if pattern:
        data = pattern.get('data', {})
        fix = data.get('value', {}).get('fix', 'N/A') if isinstance(data.get('value'), dict) else 'N/A'
        print(f"   ✓ Found pattern: {fix}")
    
    # Example 4: Query by prefix (semantic search)
    print("\n4. Querying by prefix...")
    all_patterns = backend.query_prefix("pattern:", limit=10)
    print(f"   ✓ Found {len(all_patterns)} patterns")
    
    # Example 5: Get task-specific memory summary
    print("\n5. Getting task memory summary...")
    task_memory = backend.get_task_memory("fix_bug", limit=10)
    print(f"   ✓ Found {len(task_memory['last_attempts'])} attempts")
    print(f"   ✓ Found {len(task_memory['failures'])} failures")
    print(f"   ✓ Found {len(task_memory['successes'])} successes")
    
    if task_memory['most_common_failure']:
        error = task_memory['most_common_failure']['data']['metadata'].get('error', 'Unknown')
        print(f"   ✓ Most common failure: {error}")
    
    # Example 6: O(1) lookup by node ID
    if all_patterns and all_patterns[0].get('id'):
        node_id = all_patterns[0]['id']
        print(f"\n6. O(1) lookup by node ID ({node_id})...")
        node = backend.get_by_id(node_id)
        if node:
            print(f"   ✓ Retrieved node: {node.get('payload', {}).get('name', 'Unknown')}")
    
    print("\n" + "=" * 60)
    print("Example complete!")
    print("=" * 60)
    print()
    print("In Cursor AI, you can now use:")
    print("  from synrix.agent_backend import get_synrix_backend")
    print("  backend = get_synrix_backend()")
    print("  backend.write('key', {'data': 'value'})")
    print("  memory = backend.read('key')")
    print()
    
    backend.close()


if __name__ == "__main__":
    main()

