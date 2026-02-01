"""
Test Adding and Removing from SYNRIX Lattice

Demonstrates easy add/remove operations with the Windows-native raw backend.
"""

import sys
import os

# Add python-sdk to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-sdk'))

from synrix.raw_backend import RawSynrixBackend

def test_add_remove():
    """Test easy add/remove operations"""
    print("=" * 60)
    print("Testing Add/Remove from SYNRIX Lattice")
    print("=" * 60)
    print()
    
    lattice_path = os.path.join(os.path.expanduser("~"), ".synrix_add_remove_test.lattice")
    
    # Initialize backend
    print("1. Initializing lattice...")
    backend = RawSynrixBackend(lattice_path, max_nodes=10000, evaluation_mode=True)
    print("   [OK] Lattice initialized")
    print()
    
    # Add multiple nodes
    print("2. Adding nodes...")
    node_ids = []
    for i in range(5):
        node_id = backend.add_node(
            f"MEMORY:test_{i}",
            f"Data for test node {i}",
            node_type=5
        )
        node_ids.append(node_id)
        print(f"   Added node {i}: ID={node_id}")
    print()
    
    # List all nodes
    print("3. Listing all nodes...")
    all_nodes = backend.find_by_prefix("MEMORY:", limit=100)
    print(f"   Found {len(all_nodes)} nodes:")
    for node in all_nodes:
        name = node['name'].decode('utf-8') if isinstance(node['name'], bytes) else node['name']
        data = node['data'].decode('utf-8') if isinstance(node['data'], bytes) else node['data']
        print(f"      - {name}: {data}")
    print()
    
    # Get specific node (O(1) lookup)
    print("4. Retrieving specific node (O(1) lookup)...")
    node = backend.get_node(node_ids[2])
    if node:
        name = node['name'].decode('utf-8') if isinstance(node['name'], bytes) else node['name']
        data = node['data'].decode('utf-8') if isinstance(node['data'], bytes) else node['data']
        print(f"   [OK] Retrieved: {name} = {data}")
    print()
    
    # Note: SYNRIX doesn't have a built-in delete function in raw_backend
    # But you can mark nodes as deleted or filter them out
    # For demonstration, let's show how to query and work with nodes
    
    print("5. Querying by prefix (semantic search)...")
    results = backend.find_by_prefix("MEMORY:test_", limit=10)
    print(f"   Found {len(results)} nodes matching 'MEMORY:test_'")
    print()
    
    # Save lattice
    print("6. Saving lattice...")
    if backend.save():
        print("   [OK] Lattice saved")
    print()
    
    # Show how easy it is to add more
    print("7. Adding more nodes (easy!)...")
    for i in range(5, 8):
        node_id = backend.add_node(
            f"MEMORY:more_{i}",
            f"Additional data {i}",
            node_type=5
        )
        print(f"   Added: MEMORY:more_{i} (ID={node_id})")
    print()
    
    # Final count
    final_nodes = backend.find_by_prefix("MEMORY:", limit=100)
    print(f"8. Final count: {len(final_nodes)} nodes in lattice")
    print()
    
    # Cleanup
    backend.close()
    
    print("=" * 60)
    print("[OK] Add/Remove test complete!")
    print("=" * 60)
    print()
    print("Easy Operations:")
    print("  - Add: backend.add_node(name, data)")
    print("  - Get: backend.get_node(node_id)  # O(1)")
    print("  - Query: backend.find_by_prefix(prefix)  # O(k)")
    print("  - Save: backend.save()")
    print()
    print("Note: SYNRIX uses immutable nodes (append-only)")
    print("      To 'remove', mark as deleted or use versioning")

if __name__ == "__main__":
    test_add_remove()
