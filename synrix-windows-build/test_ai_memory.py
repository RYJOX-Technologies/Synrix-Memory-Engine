"""Test AI Memory Interface"""
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-sdk'))

from synrix.ai_memory import get_ai_memory

print("Testing AI Memory Interface (Direct DLL Access)...")
print()

# Get memory interface
memory = get_ai_memory()
print(f"Backend: {'Raw Backend (Direct DLL)' if memory.backend else 'CLI'}")
print(f"Lattice Path: {memory.lattice_path}")
print()

# Add a memory
print("1. Adding memory...")
node_id = memory.add("AI_MEMORY:direct_test", "This was added directly by AI")
print(f"   Added: node_id={node_id}")
print()

# Query memories
print("2. Querying memories...")
results = memory.query("AI_MEMORY:")
print(f"   Found {len(results)} memories:")
for r in results:
    print(f"      - {r['name']}: {r['data']}")
print()

# Count
print("3. Counting all memories...")
count = memory.count()
print(f"   Total: {count} memories")
print()

print("Success! AI can now directly access SYNRIX memory.")
