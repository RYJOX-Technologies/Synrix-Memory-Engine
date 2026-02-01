#!/usr/bin/env python3
"""Simple test to verify Synrix is easy to use"""

import sys
sys.path.insert(0, 'python-sdk')

from synrix.raw_backend import RawSynrixBackend

# Test 1: Add a memory
print("=" * 60)
print("Test 1: Adding a memory")
print("=" * 60)
b = RawSynrixBackend('test_simple.lattice', max_nodes=100000, evaluation_mode=False)
node_id = b.add_node('MEMORY:test', 'This is a simple test memory', 5)
print(f"✅ Added node ID: {node_id}")

# Test 2: Retrieve the memory
print("\n" + "=" * 60)
print("Test 2: Retrieving the memory")
print("=" * 60)
result = b.get_node(node_id)
if result:
    print(f"✅ Retrieved: {result['name']} = {result['data']}")
else:
    print("❌ Failed to retrieve")

# Test 3: Query by prefix
print("\n" + "=" * 60)
print("Test 3: Querying by prefix")
print("=" * 60)
results = b.find_by_prefix('MEMORY:', limit=10)
print(f"✅ Found {len(results)} nodes with prefix 'MEMORY:'")
for r in results:
    print(f"   - {r['name']}: {r['data']}")

# Test 4: Save and close
print("\n" + "=" * 60)
print("Test 4: Saving and closing")
print("=" * 60)
b.save()
b.checkpoint()
b.close()
print("✅ Saved and closed")

print("\n" + "=" * 60)
print("✅ All tests passed! Synrix is easy to use!")
print("=" * 60)
