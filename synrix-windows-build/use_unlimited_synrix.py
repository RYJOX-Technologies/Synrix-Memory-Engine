#!/usr/bin/env python3
"""
Unlimited Synrix Helper - For Creator Use
This ensures you use the unlimited version (not free tier)
"""

import sys
import os
sys.path.insert(0, 'python-sdk')

# Force use of standard DLL (not free tier)
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.path.dirname(__file__), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def get_unlimited_backend(lattice_path='lattice/cursor_ai_memory.lattice', max_nodes=1000000):
    """
    Get unlimited Synrix backend (no node limits).
    
    Args:
        lattice_path: Path to lattice file
        max_nodes: Max nodes in RAM cache (default: 1M for unlimited use)
    
    Returns:
        RawSynrixBackend with evaluation_mode=False (unlimited)
    """
    return RawSynrixBackend(lattice_path, max_nodes=max_nodes, evaluation_mode=False)

# Example usage
if __name__ == '__main__':
    print("=" * 60)
    print("Unlimited Synrix Test")
    print("=" * 60)
    
    # Create unlimited backend
    b = get_unlimited_backend('test_unlimited.lattice')
    print("OK: Unlimited Synrix loaded (evaluation_mode=False)")
    
    # Add a test node
    node_id = b.add_node('TEST:unlimited', 'This is unlimited!', 5)
    print(f"OK: Added node: {node_id}")
    
    # Retrieve it
    result = b.get_node(node_id)
    print(f"OK: Retrieved: {result['name']} = {result['data']}")
    
    # Save
    b.save()
    b.checkpoint()
    b.close()
    print("OK: Saved and closed")
    print("\n" + "=" * 60)
    print("OK: Unlimited Synrix is working!")
    print("=" * 60)
