import sys, json
sys.path.insert(0, 'python-sdk')
from synrix.raw_backend import RawSynrixBackend

lattice_path = 'lattice/cursor_ai_memory.lattice'

print("Test 1: Add node and save")
print("-" * 50)
b1 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
node_id = b1.add_node('MEMORY:persistence_test', 'This should persist across sessions', 5)
print(f"Added node ID: {node_id}")

# Query immediately (should find it)
result1 = b1.find_by_prefix('MEMORY:persistence_test', limit=1)
print(f"Query before close: {len(result1)} nodes found")

b1.save()
b1.checkpoint()
b1.close()
print("Closed backend")
print()

print("Test 2: Reopen and check if node persists")
print("-" * 50)
b2 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)

# Query after reopen
result2 = b2.find_by_prefix('MEMORY:persistence_test', limit=1)
print(f"Query after reopen: {len(result2)} nodes found")

if result2:
    node = result2[0]
    name = node.get('name', b'')
    if isinstance(name, bytes):
        name = name.decode('utf-8', errors='ignore')
    data = node.get('data', b'')
    if isinstance(data, bytes):
        data = data.decode('utf-8', errors='ignore')
    print(f"SUCCESS: Found node '{name}'")
    print(f"Data: {data[:50]}...")
    print()
    print("RESULT: Memory IS persisting across sessions!")
else:
    print("FAILED: Node not found after reopen")
    print("RESULT: Memory is NOT persisting - only in RAM")
    print()
    print("This means the save() is failing and data is only in memory")

b2.close()
