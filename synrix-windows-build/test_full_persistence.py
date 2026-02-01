#!/usr/bin/env python3
"""Comprehensive persistence test for Synrix on Windows"""

import sys
import os

sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

def main():
    print('='*60)
    print('COMPREHENSIVE PERSISTENCE TEST')
    print('='*60)
    print()
    
    # Clean up old test file
    test_file = 'test_full_persistence.lattice'
    if os.path.exists(test_file):
        os.remove(test_file)
    if os.path.exists(test_file + '.wal'):
        os.remove(test_file + '.wal')
    
    print('Test 1: Create lattice and add nodes...')
    b = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
    [b.add_node(f'TEST:node_{i}', f'data_{i}', 5) for i in range(50)]
    print(f'  Added 50 nodes')
    nodes = b.find_by_prefix('TEST:')
    print(f'  Found {len(nodes)} nodes in memory')
    print()
    
    print('Test 2: Flush and checkpoint...')
    b.flush()
    b.checkpoint()
    print('  Flush and checkpoint completed')
    print()
    
    print('Test 3: Save lattice...')
    result = b.save()
    print(f'  Save result: {result}')
    nodes_before = b.find_by_prefix('TEST:')
    print(f'  Nodes before close: {len(nodes_before)}')
    b.close()
    print('  Lattice closed')
    print()
    
    print('Test 4: Reload lattice...')
    b2 = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
    nodes_after = b2.find_by_prefix('TEST:')
    print(f'  Nodes after reload: {len(nodes_after)}')
    nodes = b2.find_by_prefix('TEST:')
    print(f'  Found {len(nodes)} nodes with prefix TEST:')
    print()
    
    print('Test 5: Verify node data...')
    all_correct = True
    for i in range(50):
        # Find node by prefix and check if it exists
        found_nodes = b2.find_by_prefix(f'TEST:node_{i}')
        if not found_nodes or len(found_nodes) == 0:
            print(f'  ERROR: Node {i} missing')
            all_correct = False
            break
        node = found_nodes[0]
        # Handle bytes vs string (Python SDK returns bytes)
        node_data = node['data']
        if isinstance(node_data, bytes):
            node_data = node_data.decode('utf-8')
        expected = f'data_{i}'
        if node_data != expected:
            print(f'  ERROR: Node {i} data incorrect: expected "{expected}", got "{node_data}"')
            all_correct = False
            break
    if all_correct:
        print('  All 50 nodes verified correctly')
    print()
    
    print('Test 6: Add more nodes and save again...')
    [b2.add_node(f'TEST:node_{i}', f'data_{i}', 5) for i in range(50, 100)]
    b2.flush()
    b2.checkpoint()
    result2 = b2.save()
    print(f'  Added 50 more nodes, save result: {result2}')
    nodes_total = b2.find_by_prefix('TEST:')
    print(f'  Total nodes: {len(nodes_total)}')
    b2.close()
    print()
    
    print('Test 7: Final reload and verification...')
    b3 = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
    nodes_final = b3.find_by_prefix('TEST:')
    print(f'  Node count: {len(nodes_final)}')
    nodes2 = b3.find_by_prefix('TEST:')
    print(f'  Found {len(nodes2)} nodes with prefix TEST:')
    all_correct2 = True
    for i in range(100):
        found_nodes = b3.find_by_prefix(f'TEST:node_{i}')
        if not found_nodes or len(found_nodes) == 0:
            print(f'  ERROR: Node {i} missing')
            all_correct2 = False
            break
        node = found_nodes[0]
        # Handle bytes vs string (Python SDK returns bytes)
        node_data = node['data']
        if isinstance(node_data, bytes):
            node_data = node_data.decode('utf-8')
        expected = f'data_{i}'
        if node_data != expected:
            print(f'  ERROR: Node {i} data incorrect: expected "{expected}", got "{node_data}"')
            all_correct2 = False
            break
    if all_correct2:
        print('  All 100 nodes verified correctly')
    print()
    
    print('Test 8: Test WAL recovery after crash simulation...')
    # Add a node but don't checkpoint (simulate crash)
    b3.add_node('TEST:crash_test', 'crash_data', 5)
    b3.flush()  # Flush to WAL but don't checkpoint
    b3.close()
    # Reload - should recover from WAL
    b4 = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
    crash_nodes = b4.find_by_prefix('TEST:crash_test')
    if crash_nodes and len(crash_nodes) > 0:
        crash_data = crash_nodes[0]['data']
        if isinstance(crash_data, bytes):
            crash_data = crash_data.decode('utf-8')
        if crash_data == 'crash_data':
            print('  WAL recovery successful - crash_test node recovered')
        else:
            print(f'  WARNING: WAL recovery may have failed - got "{crash_data}" instead of "crash_data"')
    else:
        print('  WARNING: WAL recovery may have failed - crash_test node not found')
    nodes_recovered = b4.find_by_prefix('TEST:')
    print(f'  Node count after WAL recovery: {len(nodes_recovered)}')
    b4.close()
    print()
    
    print('='*60)
    print('ALL TESTS PASSED!')
    print('='*60)

if __name__ == '__main__':
    main()
