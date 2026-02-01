#!/usr/bin/env python3
"""Test that Synrix actually saves and persists data"""

import sys
import os
import json

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python-sdk"))

from synrix.raw_backend import RawSynrixBackend

print("=" * 70)
print("Testing Synrix Save/Persistence")
print("=" * 70)
print()

lattice_path = "lattice/cursor_ai_memory.lattice"

# Step 1: Add a test node
print("Step 1: Adding test node...")
b1 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
test_id = b1.add_node(
    "TEST:save_verification",
    json.dumps({"test": "This should persist", "timestamp": "2025-01-12"}),
    5
)
print(f"  Added node ID: {test_id}")

# Step 2: Save and checkpoint
print("Step 2: Saving and checkpointing...")
b1.save()
b1.checkpoint()
print("  Saved and checkpointed")
b1.close()
print("  Closed backend")
print()

# Step 3: Reopen and verify
print("Step 3: Reopening to verify persistence...")
b2 = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
found = b2.find_by_prefix("TEST:save_verification", limit=1)

if found:
    node = found[0]
    name = node.get("name", b"")
    if isinstance(name, bytes):
        name = name.decode("utf-8", errors="ignore")
    print(f"  SUCCESS: Found stored node: {name}")
    print("  Data is persisting correctly!")
else:
    print("  FAILED: Node not found after reopen")
    print("  Data may not be persisting")
print()

# Step 4: Check what we stored earlier
print("Step 4: Checking previously stored nodes...")
patterns = b2.find_by_prefix("PATTERN:build_tier", limit=10)
print(f"  Found {len(patterns)} tier build patterns")
for p in patterns:
    name = p.get("name", b"")
    if isinstance(name, bytes):
        name = name.decode("utf-8", errors="ignore")
    print(f"    - {name}")

constraints = b2.find_by_prefix("CONSTRAINT:tier", limit=10)
print(f"  Found {len(constraints)} tier constraints")
for c in constraints:
    name = c.get("name", b"")
    if isinstance(name, bytes):
        name = name.decode("utf-8", errors="ignore")
    print(f"    - {name}")

# Step 5: Count all nodes
print()
print("Step 5: Total node count...")
all_patterns = b2.find_by_prefix("PATTERN:", limit=1000)
all_constraints = b2.find_by_prefix("CONSTRAINT:", limit=1000)
all_work = b2.find_by_prefix("WORK:", limit=1000)
all_test = b2.find_by_prefix("TEST:", limit=1000)

total = len(all_patterns) + len(all_constraints) + len(all_work) + len(all_test)
print(f"  PATTERN nodes: {len(all_patterns)}")
print(f"  CONSTRAINT nodes: {len(all_constraints)}")
print(f"  WORK nodes: {len(all_work)}")
print(f"  TEST nodes: {len(all_test)}")
print(f"  Total: {total} nodes")
print()

b2.close()

print("=" * 70)
if found:
    print("RESULT: Data IS persisting correctly!")
else:
    print("RESULT: Data may NOT be persisting - check WAL")
print("=" * 70)
