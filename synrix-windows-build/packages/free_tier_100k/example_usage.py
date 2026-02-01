#!/usr/bin/env python3
"""
SYNRIX Usage Example
Run this after installation to verify SYNRIX works
"""

from synrix.ai_memory import get_ai_memory

print("=== SYNRIX Usage Example ===\n")

# Get memory instance (auto-creates if needed)
print("1. Getting memory instance...")
memory = get_ai_memory()
print("   [OK] Memory instance created\n")

# Store some data
print("2. Storing data...")
memory.add("PROJECT:name", "My Project")
memory.add("TASK:fix_bug", "Fixed null pointer")
memory.add("NOTE:important", "Remember to update docs")
print("   [OK] Stored 3 entries\n")

# Query by prefix
print("3. Querying by prefix 'TASK:'...")
tasks = memory.query("TASK:")
print(f"   [OK] Found {len(tasks)} tasks:")
for task in tasks:
    print(f"      - {task['name']}: {task['data']}")
print()

# Get total count
print("4. Getting total node count...")
count = memory.count()
print(f"   [OK] Total nodes: {count}\n")

# Get specific node
if tasks:
    print("5. Getting specific node by ID...")
    node_id = tasks[0]['id']
    node = memory.get(node_id)
    if node:
        print(f"   [OK] Node {node_id}: {node['name']} = {node['data']}\n")

print("=== All tests passed! ===")
print("\nSYNRIX is working correctly.")
print("You can now use it in any Python script:")
print("  from synrix.ai_memory import get_ai_memory")
print("  memory = get_ai_memory()")
print("  memory.add('KEY', 'value')")
