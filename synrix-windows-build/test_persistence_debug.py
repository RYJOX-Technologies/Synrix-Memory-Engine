#!/usr/bin/env python3
"""Debug persistence issue"""

import sys
import os

sys.path.insert(0, 'python-sdk')
os.environ['SYNRIX_LIB_PATH'] = os.path.join(os.getcwd(), 'python-sdk', 'libsynrix.dll')

from synrix.raw_backend import RawSynrixBackend

# Clean up
test_file = 'test_persistence_debug.lattice'
if os.path.exists(test_file):
    os.remove(test_file)
if os.path.exists(test_file + '.wal'):
    os.remove(test_file + '.wal')

print("="*70)
print("PERSISTENCE DEBUG TEST")
print("="*70)

# Session 1: Add nodes
print("\n[Session 1] Adding 10 test nodes...")
b = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
for i in range(10):
    b.add_node(f'TEST:node_{i}', f'data_{i}', 5)

all_before = b.find_by_prefix('', limit=100)
print(f"  Nodes in memory: {len(all_before)}")

# Check file before save
if os.path.exists(test_file):
    size_before = os.path.getsize(test_file)
    print(f"  File size before save: {size_before} bytes")
else:
    print(f"  File does not exist before save")

# Save and checkpoint
print("\n[Session 1] Saving and checkpointing...")
b.flush()
b.checkpoint()
b.save()

# Check file after save
if os.path.exists(test_file):
    size_after = os.path.getsize(test_file)
    print(f"  File size after save: {size_after} bytes")
    expected_size = 16 + (10 * 1216)  # header + 10 nodes
    print(f"  Expected size: {expected_size} bytes")
    if size_after < expected_size:
        print(f"  ⚠️  File size is smaller than expected!")
else:
    print(f"  ❌ File does not exist after save!")

b.close()
print("  Closed")

# Session 2: Reload and verify
print("\n[Session 2] Reloading...")
if os.path.exists(test_file):
    size_before_reload = os.path.getsize(test_file)
    print(f"  File size before reload: {size_before_reload} bytes")
else:
    print(f"  ❌ File does not exist before reload!")

b2 = RawSynrixBackend(test_file, max_nodes=1000000, evaluation_mode=False)
all_after = b2.find_by_prefix('', limit=100)
print(f"  Nodes after reload: {len(all_after)}")

test_nodes = [n for n in all_after if 'TEST:' in str(n['name'])]
print(f"  Test nodes found: {len(test_nodes)}")

# Result
print("\n" + "="*70)
if len(test_nodes) == 10:
    print("✅ PERSISTENCE: PASS")
else:
    print(f"❌ PERSISTENCE: FAIL - Expected 10 nodes, found {len(test_nodes)}")
print("="*70)

b2.close()
