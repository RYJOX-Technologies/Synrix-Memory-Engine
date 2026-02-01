#!/usr/bin/env python3
"""Verify persistence is working correctly"""

import sys
import os

sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

# Clean up any existing test files
test_file = 'test_persistence_check.lattice'
if os.path.exists(test_file):
    os.remove(test_file)
if os.path.exists(test_file + '.wal'):
    os.remove(test_file + '.wal')

print("="*70)
print("PERSISTENCE VERIFICATION TEST")
print("="*70)

# Session 1: Add nodes
print("\n[Session 1] Adding 10 test nodes...")
b = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
for i in range(10):
    b.add_node(f'TEST:node_{i}', f'data_{i}', 5)

all_before = b.find_by_prefix('', limit=100)
print(f"  Nodes in memory: {len(all_before)}")

# Save and checkpoint
print("\n[Session 1] Saving and checkpointing...")
b.flush()
b.checkpoint()
b.save()
b.close()
print("  Closed")

# Session 2: Reload and verify
print("\n[Session 2] Reloading...")
b2 = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
all_after = b2.find_by_prefix('', limit=100)
print(f"  Nodes after reload: {len(all_after)}")

test_nodes = [n for n in all_after if 'TEST:' in str(n['name'])]
print(f"  Test nodes found: {len(test_nodes)}")

# Result
print("\n" + "="*70)
if len(test_nodes) == 10:
    print("[OK] PERSISTENCE: PASS - All 10 nodes persisted correctly")
    result = True
else:
    print(f"[ERROR] PERSISTENCE: FAIL - Expected 10 nodes, found {len(test_nodes)}")
    result = False
print("="*70)

b2.close()
sys.exit(0 if result else 1)
