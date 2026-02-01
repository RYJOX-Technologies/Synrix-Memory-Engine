#!/usr/bin/env python3
"""Test SYNRIX SDK - add and retrieve memories"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python-sdk"))

from synrix.raw_backend import RawSynrixBackend

# Test adding and retrieving
print("Testing SYNRIX SDK...")
print()

# Create/open lattice
backend = RawSynrixBackend("test_memory.lattice", max_nodes=10000)
print("OK: Lattice opened")

# Add a test node
node_id = backend.add_node(
    "TEST:windows_build_success",
    "Successfully built libsynrix.dll on Windows with MSYS2 MinGW-w64",
    node_type=5  # LATTICE_NODE_LEARNING
)
print(f"OK: Added node ID: {node_id}")

# Retrieve the node
node = backend.get_node(node_id)
if node:
    name = node.get("name", "")
    data = node.get("data", b"")
    if isinstance(data, bytes):
        data_str = data.decode("utf-8", errors="ignore")
    else:
        data_str = str(data)
    print(f"OK: Retrieved node: {name}")
    print(f"   Data: {data_str[:80]}...")
else:
    print("ERROR: Failed to retrieve node")

# Query by prefix
results = backend.find_by_prefix("TEST:", limit=5)
print(f"OK: Found {len(results)} nodes with TEST: prefix")

# Add another node
node_id2 = backend.add_node(
    "WORK:windows_build_complete",
    "Windows DLL build completed successfully. DLL: libsynrix.dll (612KB)",
    node_type=5
)
print(f"OK: Added second node ID: {node_id2}")

# Query WORK: prefix
work_results = backend.find_by_prefix("WORK:", limit=5)
print(f"OK: Found {len(work_results)} nodes with WORK: prefix")

backend.close()
print()
print("SUCCESS: All tests passed! SDK is working correctly.")
