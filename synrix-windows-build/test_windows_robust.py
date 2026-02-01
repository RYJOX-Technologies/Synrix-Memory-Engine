#!/usr/bin/env python3
"""
Robust Windows Testing Suite for Synrix
Tests stability, persistence, edge cases, and error handling
"""

import sys
import os
import json
import time
import random
sys.path.insert(0, 'python-sdk')

# Force use of standard DLL
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.path.dirname(__file__), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def test_basic_operations():
    """Test 1: Basic add/get/query operations"""
    print("=" * 60)
    print("TEST 1: Basic Operations")
    print("=" * 60)
    
    b = RawSynrixBackend('test_basic.lattice', max_nodes=100000, evaluation_mode=False)
    
    # Add nodes
    node_ids = []
    for i in range(100):
        node_id = b.add_node(f'TEST:node_{i}', f'data_{i}', 5)
        if node_id == 0:
            print(f"  FAILED: Failed to add node {i}")
            return False
        node_ids.append(node_id)
    
    print(f"  OK: Added {len(node_ids)} nodes")
    
    # Retrieve nodes
    for i, node_id in enumerate(node_ids[:10]):
        result = b.get_node(node_id)
        if not result or result['name'] != f'TEST:node_{i}':
            print(f"  FAILED: Failed to retrieve node {i}")
            return False
    
    print("  OK: Retrieved 10 nodes correctly")
    
    # Query by prefix
    results = b.find_by_prefix('TEST:', limit=50)
    if len(results) < 50:
        print(f"  FAILED: Expected 50 results, got {len(results)}")
        return False
    
    print(f"  OK: Found {len(results)} nodes by prefix")
    
    b.save()
    b.close()
    print("  OK: Saved and closed")
    return True

def test_persistence():
    """Test 2: Persistence - save, reload, verify"""
    print("\n" + "=" * 60)
    print("TEST 2: Persistence")
    print("=" * 60)
    
    lattice_path = 'test_persistence.lattice'
    
    # Create and add nodes
    b1 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    test_data = {}
    for i in range(50):
        name = f'PERSIST:node_{i}'
        data = json.dumps({'id': i, 'value': f'data_{i}', 'timestamp': time.time()})
        node_id = b1.add_node(name, data, 5)
        test_data[node_id] = {'name': name, 'data': data}
    
    print(f"  OK: Added {len(test_data)} nodes")
    b1.save()
    b1.checkpoint()
    b1.close()
    print("  OK: Saved and closed")
    
    # Reload and verify
    b2 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    # Check how many nodes were actually loaded
    all_nodes = b2.find_by_prefix('PERSIST:', limit=1000)
    print(f"  DEBUG: Found {len(all_nodes)} nodes with PERSIST: prefix after reload")
    
    for node_id, expected in list(test_data.items())[:20]:
        result = b2.get_node(node_id)
        if not result:
            print(f"  FAILED: Node {node_id} not found after reload")
            return False
        if result['name'] != expected['name']:
            print(f"  FAILED: Name mismatch for node {node_id}")
            return False
        if result['data'] != expected['data']:
            print(f"  FAILED: Data mismatch for node {node_id}")
            return False
    
    print("  OK: Verified 20 nodes after reload")
    b2.close()
    return True

def test_large_dataset():
    """Test 3: Large dataset - many nodes"""
    print("\n" + "=" * 60)
    print("TEST 3: Large Dataset (1000 nodes)")
    print("=" * 60)
    
    b = RawSynrixBackend('test_large.lattice', max_nodes=100000, evaluation_mode=False)
    
    start_time = time.time()
    node_ids = []
    for i in range(1000):
        node_id = b.add_node(f'LARGE:node_{i}', f'data_{i}', 5)
        if node_id == 0:
            print(f"  FAILED: Failed to add node {i}")
            return False
        node_ids.append(node_id)
        if (i + 1) % 100 == 0:
            print(f"  Progress: {i + 1}/1000 nodes added")
    
    elapsed = time.time() - start_time
    print(f"  OK: Added 1000 nodes in {elapsed:.2f}s ({elapsed*1000/1000:.2f}ms per node)")
    
    # Query all
    results = b.find_by_prefix('LARGE:', limit=1000)
    if len(results) != 1000:
        print(f"  FAILED: Expected 1000 results, got {len(results)}")
        return False
    
    print(f"  OK: Found all 1000 nodes by prefix")
    
    # Save
    save_start = time.time()
    b.save()
    b.checkpoint()
    save_elapsed = time.time() - save_start
    print(f"  OK: Saved in {save_elapsed:.2f}s")
    
    b.close()
    return True

def test_concurrent_operations():
    """Test 4: Simulate concurrent-like operations (rapid add/get)"""
    print("\n" + "=" * 60)
    print("TEST 4: Concurrent-like Operations")
    print("=" * 60)
    
    b = RawSynrixBackend('test_concurrent.lattice', max_nodes=100000, evaluation_mode=False)
    
    # Rapid add/get pattern
    node_ids = []
    for i in range(200):
        # Add
        node_id = b.add_node(f'CONC:node_{i}', f'data_{i}', 5)
        node_ids.append(node_id)
        
        # Immediately get (simulates concurrent read)
        if i % 10 == 0:
            result = b.get_node(node_id)
            if not result:
                print(f"  FAILED: Failed to get node immediately after add (i={i})")
                return False
    
    print(f"  OK: Added and immediately retrieved {len(node_ids)} nodes")
    
    # Random access
    random.shuffle(node_ids)
    for node_id in node_ids[:50]:
        result = b.get_node(node_id)
        if not result:
            print(f"  FAILED: Failed to get node {node_id} in random access")
            return False
    
    print("  OK: Random access to 50 nodes successful")
    
    b.save()
    b.close()
    return True

def test_wal_recovery():
    """Test 5: WAL recovery - simulate crash and recovery"""
    print("\n" + "=" * 60)
    print("TEST 5: WAL Recovery")
    print("=" * 60)
    
    lattice_path = 'test_wal.lattice'
    
    # Add nodes but don't checkpoint (simulate crash)
    b1 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    node_ids = []
    for i in range(100):
        node_id = b1.add_node(f'WAL:node_{i}', f'data_{i}', 5)
        node_ids.append(node_id)
    
    print(f"  OK: Added {len(node_ids)} nodes")
    b1.save()  # Save but don't checkpoint
    b1.close()
    print("  OK: Closed without checkpoint (simulating crash)")
    
    # Reload - should recover from WAL
    b2 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    
    # Verify nodes exist (recovered from WAL)
    recovered = 0
    for node_id in node_ids[:20]:
        result = b2.get_node(node_id)
        if result:
            recovered += 1
    
    if recovered < 20:
        print(f"  FAILED: Only recovered {recovered}/20 nodes from WAL")
        return False
    
    print(f"  OK: Recovered {recovered}/20 nodes from WAL")
    
    # Now checkpoint
    b2.save()
    b2.checkpoint()
    b2.close()
    
    # Reload again - should be in main file now
    b3 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    for node_id in node_ids[:20]:
        result = b3.get_node(node_id)
        if not result:
            print(f"  FAILED: Node {node_id} not found after checkpoint")
            return False
    
    print("  OK: All nodes persisted after checkpoint")
    b3.close()
    return True

def test_edge_cases():
    """Test 6: Edge cases - empty data, long names, special chars"""
    print("\n" + "=" * 60)
    print("TEST 6: Edge Cases")
    print("=" * 60)
    
    b = RawSynrixBackend('test_edge.lattice', max_nodes=100000, evaluation_mode=False)
    
    # Empty data
    node_id1 = b.add_node('EDGE:empty', '', 5)
    result = b.get_node(node_id1)
    if not result or result['data'] != '':
        print("  FAILED: Empty data not handled correctly")
        return False
    print("  OK: Empty data handled")
    
    # Long name (64 char limit)
    long_name = 'EDGE:' + 'x' * 58
    node_id2 = b.add_node(long_name, 'data', 5)
    result = b.get_node(node_id2)
    if not result:
        print("  FAILED: Long name not handled")
        return False
    print("  OK: Long name handled")
    
    # Special characters in data
    special_data = json.dumps({'key': 'value\nwith\nnewlines', 'unicode': '测试'})
    node_id3 = b.add_node('EDGE:special', special_data, 5)
    result = b.get_node(node_id3)
    if not result or result['data'] != special_data:
        print("  FAILED: Special characters not handled")
        return False
    print("  OK: Special characters handled")
    
    # Large data (near 512 byte limit)
    large_data = 'x' * 500
    node_id4 = b.add_node('EDGE:large', large_data, 5)
    result = b.get_node(node_id4)
    if not result or len(result['data']) != 500:
        print("  FAILED: Large data not handled")
        return False
    print("  OK: Large data handled")
    
    b.save()
    b.close()
    return True

def test_stress():
    """Test 7: Stress test - rapid operations"""
    print("\n" + "=" * 60)
    print("TEST 7: Stress Test (500 nodes, rapid operations)")
    print("=" * 60)
    
    b = RawSynrixBackend('test_stress.lattice', max_nodes=100000, evaluation_mode=False)
    
    start_time = time.time()
    node_ids = []
    
    # Rapid add
    for i in range(500):
        node_id = b.add_node(f'STRESS:node_{i}', f'data_{i}', 5)
        node_ids.append(node_id)
    
    add_time = time.time() - start_time
    print(f"  OK: Added 500 nodes in {add_time:.2f}s")
    
    # Rapid get
    get_start = time.time()
    for node_id in node_ids[:100]:
        result = b.get_node(node_id)
        if not result:
            print(f"  FAILED: Failed to get node {node_id}")
            return False
    get_time = time.time() - get_start
    print(f"  OK: Retrieved 100 nodes in {get_time:.2f}s")
    
    # Rapid queries
    query_start = time.time()
    for i in range(10):
        results = b.find_by_prefix('STRESS:', limit=50)
        if len(results) < 50:
            print(f"  FAILED: Query {i} returned only {len(results)} results")
            return False
    query_time = time.time() - query_start
    print(f"  OK: 10 queries in {query_time:.2f}s")
    
    # Save under stress
    save_start = time.time()
    b.save()
    b.checkpoint()
    save_time = time.time() - save_start
    print(f"  OK: Saved under stress in {save_time:.2f}s")
    
    b.close()
    return True

def cleanup_test_files():
    """Clean up test lattice files"""
    test_files = [
        'test_basic.lattice', 'test_basic.lattice.wal',
        'test_persistence.lattice', 'test_persistence.lattice.wal',
        'test_large.lattice', 'test_large.lattice.wal',
        'test_concurrent.lattice', 'test_concurrent.lattice.wal',
        'test_wal.lattice', 'test_wal.lattice.wal',
        'test_edge.lattice', 'test_edge.lattice.wal',
        'test_stress.lattice', 'test_stress.lattice.wal',
    ]
    for f in test_files:
        if os.path.exists(f):
            try:
                os.remove(f)
            except:
                pass

def main():
    print("\n" + "=" * 60)
    print("SYNRIX Windows Robustness Test Suite")
    print("=" * 60)
    print()
    
    tests = [
        ("Basic Operations", test_basic_operations),
        ("Persistence", test_persistence),
        ("Large Dataset", test_large_dataset),
        ("Concurrent Operations", test_concurrent_operations),
        ("WAL Recovery", test_wal_recovery),
        ("Edge Cases", test_edge_cases),
        ("Stress Test", test_stress),
    ]
    
    results = []
    start_time = time.time()
    
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
            if not result:
                print(f"\n  FAIL: {test_name} FAILED")
            else:
                print(f"\n  PASS: {test_name} PASSED")
        except Exception as e:
            print(f"\n  FAIL: {test_name} FAILED with exception: {e}")
            import traceback
            traceback.print_exc()
            results.append((test_name, False))
    
    total_time = time.time() - start_time
    
    # Summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for test_name, result in results:
        status = "PASS" if result else "FAIL"
        print(f"  {status}: {test_name}")
    
    print(f"\n  Total: {passed}/{total} tests passed")
    print(f"  Time: {total_time:.2f}s")
    
    if passed == total:
        print("\n  SUCCESS: ALL TESTS PASSED - Synrix is stable on Windows!")
    else:
        print(f"\n  WARNING: {total - passed} test(s) failed")
    
    # Cleanup
    print("\nCleaning up test files...")
    cleanup_test_files()
    print("Done.")
    
    return passed == total

if __name__ == '__main__':
    success = main()
    sys.exit(0 if success else 1)
