import sys, json
sys.path.insert(0, 'python-sdk')
from synrix.raw_backend import RawSynrixBackend

b = RawSynrixBackend('lattice/cursor_ai_memory.lattice', max_nodes=100000, evaluation_mode=False)

# Add memory
node_id = b.add_node('MEMORY:test_save_check', 'This is a test memory to verify saving works', 5)
print(f'Added node ID: {node_id}')

# Save
b.save()
b.checkpoint()
print('Saved')

# Query immediately
result = b.find_by_prefix('MEMORY:test_save_check', limit=1)
print(f'Query result: {len(result)} nodes found')

if result:
    print('SUCCESS: Memory saved and retrieved!')
else:
    print('FAILED: Memory not found')

b.close()
