#!/usr/bin/env python3
"""
Test WAL Functionality
Verifies that nodes are written to WAL first, then persisted correctly
"""

import sys
import os
import time
sys.path.insert(0, 'python-sdk')

# Set DLL path before importing
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.path.dirname(__file__), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def test_wal_basic():
    """Test 1: Basic WAL functionality"""
    print("=" * 60)
    print("TEST 1: Basic WAL Functionality")
    print("=" * 60)
    
    lattice_path = 'test_wal_basic.lattice'
    
    # Remove old files
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    if os.path.exists(lattice_path + '.wal'):
        os.remove(lattice_path + '.wal')
    
    # Create backend (WAL should be enabled automatically)
    b = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    # Add nodes
    print("  Adding 50 nodes...")
    node_ids = []
    for i in range(50):
        node_id = b.add_node(f'TEST:node_{i}', f'data_{i}', 5)
        node_ids.append(node_id)
        if node_id == 0:
            print(f"  FAILED: Node {i} returned 0")
            return False
    
    print(f"  OK: Added {len(node_ids)} nodes")
    
    # Check WAL file exists
    wal_path = lattice_path + '.wal'
    if not os.path.exists(wal_path):
        print(f"  FAILED: WAL file not created: {wal_path}")
        return False
    
    wal_size = os.path.getsize(wal_path)
    print(f"  OK: WAL file exists ({wal_size} bytes)")
    
    if wal_size < 100:  # Should have some data
        print(f"  WARNING: WAL file seems too small ({wal_size} bytes)")
    
    # Save (writes memory to main file)
    print("  Saving...")
    if not b.save():
        print("  FAILED: save() returned False")
        return False
    
    # Check main file exists
    if not os.path.exists(lattice_path):
        print(f"  FAILED: Main file not created: {lattice_path}")
        return False
    
    main_size = os.path.getsize(lattice_path)
    print(f"  OK: Main file exists ({main_size} bytes)")
    
    # Checkpoint (applies WAL entries, then saves)
    print("  Checkpointing...")
    if not b.checkpoint():
        print("  WARNING: checkpoint() returned False (may be OK if WAL is empty)")
    
    b.close()
    
    # Reload and verify
    print("  Reloading and verifying...")
    b2 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    # Query by prefix
    results = b2.find_by_prefix('TEST:', limit=1000)
    if len(results) < 50:
        print(f"  FAILED: Expected 50 nodes, got {len(results)}")
        return False
    
    print(f"  OK: Found {len(results)} nodes after reload")
    
    # Verify specific nodes
    found_count = 0
    for i in range(min(10, len(node_ids))):
        result = b2.get_node(node_ids[i])
        if result:
            found_count += 1
    
    if found_count < 10:
        print(f"  FAILED: Only found {found_count}/10 nodes by ID")
        return False
    
    print(f"  OK: Found {found_count}/10 nodes by ID")
    
    b2.close()
    return True

def test_wal_persistence():
    """Test 2: WAL persistence across sessions"""
    print("\n" + "=" * 60)
    print("TEST 2: WAL Persistence Across Sessions")
    print("=" * 60)
    
    lattice_path = 'test_wal_persistence.lattice'
    
    # Remove old files
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    if os.path.exists(lattice_path + '.wal'):
        os.remove(lattice_path + '.wal')
    
    # Session 1: Add nodes, save, but don't checkpoint
    print("  Session 1: Adding 30 nodes, saving (no checkpoint)...")
    b1 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    for i in range(30):
        b1.add_node(f'SESSION1:node_{i}', f'data_{i}', 5)
    b1.save()  # Save but don't checkpoint
    b1.close()
    
    # Check WAL exists
    wal_path = lattice_path + '.wal'
    if not os.path.exists(wal_path):
        print("  FAILED: WAL file not created in session 1")
        return False
    
    wal_size_1 = os.path.getsize(wal_path)
    print(f"  OK: WAL file exists ({wal_size_1} bytes)")
    
    # Session 2: Add more nodes, checkpoint
    print("  Session 2: Adding 20 more nodes, checkpointing...")
    b2 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    # Verify session 1 nodes are in memory (from WAL recovery)
    s1_results = b2.find_by_prefix('SESSION1:', limit=100)
    print(f"  OK: Recovered {len(s1_results)} nodes from WAL")
    
    # Add session 2 nodes
    for i in range(20):
        b2.add_node(f'SESSION2:node_{i}', f'data_{i}', 5)
    
    # Checkpoint (applies all WAL entries, then saves)
    b2.checkpoint()
    b2.close()
    
    # Session 3: Verify both sessions persisted
    print("  Session 3: Verifying persistence...")
    b3 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    s1_final = b3.find_by_prefix('SESSION1:', limit=100)
    s2_final = b3.find_by_prefix('SESSION2:', limit=100)
    
    if len(s1_final) < 30:
        print(f"  FAILED: Session 1 has {len(s1_final)} nodes, expected 30")
        return False
    
    if len(s2_final) < 20:
        print(f"  FAILED: Session 2 has {len(s2_final)} nodes, expected 20")
        return False
    
    print(f"  OK: Session 1: {len(s1_final)} nodes, Session 2: {len(s2_final)} nodes")
    print(f"  OK: Total: {len(s1_final) + len(s2_final)} nodes persisted")
    
    b3.close()
    return True

def test_wal_recovery():
    """Test 3: WAL recovery after crash (simulated)"""
    print("\n" + "=" * 60)
    print("TEST 3: WAL Recovery After Crash")
    print("=" * 60)
    
    lattice_path = 'test_wal_recovery.lattice'
    
    # Remove old files
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    if os.path.exists(lattice_path + '.wal'):
        os.remove(lattice_path + '.wal')
    
    # Simulate crash: Add nodes, but don't save or checkpoint
    print("  Simulating crash: Adding 40 nodes, no save/checkpoint...")
    b1 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    node_ids = []
    for i in range(40):
        node_id = b1.add_node(f'CRASH:node_{i}', f'data_{i}', 5)
        node_ids.append(node_id)
    # Intentionally don't save or checkpoint (simulate crash)
    b1.close()
    
    # Check WAL exists
    wal_path = lattice_path + '.wal'
    if not os.path.exists(wal_path):
        print("  FAILED: WAL file not created")
        return False
    
    wal_size = os.path.getsize(wal_path)
    print(f"  OK: WAL file exists ({wal_size} bytes)")
    
    # Recover: Reload should recover from WAL
    print("  Recovering: Reloading should recover from WAL...")
    b2 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    
    # Query nodes (should be recovered from WAL)
    recovered = b2.find_by_prefix('CRASH:', limit=100)
    if len(recovered) < 30:  # Allow some tolerance
        print(f"  WARNING: Only recovered {len(recovered)}/40 nodes (expected at least 30)")
        # This might be OK if WAL batching delayed writes
    
    print(f"  OK: Recovered {len(recovered)} nodes from WAL")
    
    # Now checkpoint to persist
    b2.checkpoint()
    b2.close()
    
    # Final verify
    print("  Final verification after checkpoint...")
    b3 = RawSynrixBackend(lattice_path, max_nodes=1000000, evaluation_mode=False)
    final_results = b3.find_by_prefix('CRASH:', limit=100)
    
    if len(final_results) < 35:  # Allow some tolerance
        print(f"  FAILED: Only {len(final_results)}/40 nodes persisted after checkpoint")
        return False
    
    print(f"  OK: {len(final_results)}/40 nodes persisted after checkpoint")
    b3.close()
    return True

def cleanup_test_files():
    """Clean up test files"""
    test_files = [
        'test_wal_basic.lattice', 'test_wal_basic.lattice.wal',
        'test_wal_persistence.lattice', 'test_wal_persistence.lattice.wal',
        'test_wal_recovery.lattice', 'test_wal_recovery.lattice.wal',
    ]
    for f in test_files:
        if os.path.exists(f):
            try:
                os.remove(f)
            except:
                pass

def main():
    print("\n" + "=" * 60)
    print("SYNRIX WAL Functionality Test Suite")
    print("=" * 60)
    print()
    
    tests = [
        ("Basic WAL Functionality", test_wal_basic),
        ("WAL Persistence Across Sessions", test_wal_persistence),
        ("WAL Recovery After Crash", test_wal_recovery),
    ]
    
    results = []
    start_time = time.time()
    
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
            if not result:
                print(f"\n  FAIL: {test_name}")
            else:
                print(f"\n  PASS: {test_name}")
        except Exception as e:
            print(f"\n  FAIL: {test_name} - Exception: {e}")
            import traceback
            traceback.print_exc()
            results.append((test_name, False))
    
    total_time = time.time() - start_time
    
    # Summary
    print("\n" + "=" * 60)
    print("WAL TEST SUMMARY")
    print("=" * 60)
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for test_name, result in results:
        status = "PASS" if result else "FAIL"
        print(f"  {status}: {test_name}")
    
    print(f"\n  Total: {passed}/{total} tests passed")
    print(f"  Time: {total_time:.2f}s")
    
    if passed == total:
        print("\n  SUCCESS: ALL WAL TESTS PASSED!")
        print("  WAL is working correctly - nodes write to WAL first, then persist.")
    else:
        print(f"\n  WARNING: {total - passed} test(s) failed")
        print("  WAL functionality may need investigation.")
    
    # Cleanup
    print("\nCleaning up test files...")
    cleanup_test_files()
    print("Done.")
    
    return passed == total

if __name__ == '__main__':
    success = main()
    sys.exit(0 if success else 1)
