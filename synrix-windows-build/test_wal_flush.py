#!/usr/bin/env python3
"""
Test Manual WAL Flush
Verifies that manual flush works correctly
"""

import sys
import os
sys.path.insert(0, 'python-sdk')

# Set DLL path before importing
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.path.dirname(__file__), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def test_manual_flush():
    """Test manual WAL flush"""
    print("=" * 60)
    print("TEST: Manual WAL Flush")
    print("=" * 60)
    
    lattice_path = 'test_flush.lattice'
    
    # Remove old files
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    if os.path.exists(lattice_path + '.wal'):
        os.remove(lattice_path + '.wal')
    
    # Create backend
    b = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    # Add a few nodes (not enough to trigger auto-flush)
    print("  Adding 10 nodes (not enough to trigger auto-flush)...")
    for i in range(10):
        b.add_node(f'TEST:node_{i}', f'data_{i}', 5)
    
    # Check WAL file size before flush
    wal_path = lattice_path + '.wal'
    if os.path.exists(wal_path):
        size_before = os.path.getsize(wal_path)
        print(f"  WAL file size before flush: {size_before} bytes")
    else:
        print("  WAL file doesn't exist yet (entries in buffer)")
        size_before = 0
    
    # Manually flush
    print("  Manually flushing WAL...")
    if b.flush():
        print("  OK: Flush successful")
    else:
        print("  WARNING: Flush returned False (may be OK if no entries)")
    
    # Check WAL file size after flush
    if os.path.exists(wal_path):
        size_after = os.path.getsize(wal_path)
        print(f"  WAL file size after flush: {size_after} bytes")
        if size_after > size_before:
            print("  OK: WAL file grew (entries were flushed)")
        else:
            print("  WARNING: WAL file size didn't change")
    else:
        print("  FAILED: WAL file still doesn't exist after flush")
        return False
    
    # Now checkpoint to apply entries
    print("  Checkpointing...")
    b.checkpoint()
    
    # Verify nodes persisted
    b.close()
    b2 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    results = b2.find_by_prefix('TEST:', limit=100)
    
    if len(results) >= 10:
        print(f"  OK: Found {len(results)} nodes after flush + checkpoint")
        return True
    else:
        print(f"  FAILED: Only found {len(results)}/10 nodes")
        return False

if __name__ == '__main__':
    success = test_manual_flush()
    sys.exit(0 if success else 1)
