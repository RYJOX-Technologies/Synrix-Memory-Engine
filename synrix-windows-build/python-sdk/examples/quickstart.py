#!/usr/bin/env python3
"""
SYNRIX Quickstart Example

A simple, clean example that demonstrates the SYNRIX API.
This is the "magical" example that gets developers excited.
"""

# Try to import synrix - works if installed via pip
try:
    from synrix import SynrixClient, SynrixMockClient
except ImportError:
    # If not installed, try importing from parent directory (repo layout)
    import sys
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)  # python-sdk directory
    if parent_dir not in sys.path:
        sys.path.insert(0, parent_dir)
    from synrix import SynrixClient, SynrixMockClient


def main():
    print("SYNRIX Quickstart")
    print("=" * 50)
    print()
    
    # Use mock client for this example (no server required)
    # In production, use: client = SynrixClient("http://localhost:6334")
    print("Creating SYNRIX client...")
    client = SynrixMockClient()
    print("✅ Client ready")
    print()
    
    # Create a collection
    print("Creating collection...")
    client.create_collection("knowledge_base")
    print("✅ Collection created")
    print()
    
    # Add some knowledge nodes
    print("Adding knowledge nodes...")
    nodes = [
        ("ISA_ADD", "Addition operation"),
        ("ISA_SUBTRACT", "Subtraction operation"),
        ("ISA_MULTIPLY", "Multiplication operation"),
        ("QDRANT_COLLECTION:documents", "Document collection"),
    ]
    
    for name, data in nodes:
        node_id = client.add_node(name, data, collection="knowledge_base")
        print(f"  ✅ Added: {name} (ID: {node_id})")
    
    print()
    
    # Query by prefix (O(k) semantic query)
    print("Querying nodes with prefix 'ISA_'...")
    results = client.query_prefix("ISA_", collection="knowledge_base")
    print(f"✅ Found {len(results)} nodes:")
    for result in results:
        name = result.get("payload", {}).get("name", "unknown")
        print(f"  • {name}")
    
    print()
    print("=" * 50)
    print("That's it! Clean, simple, powerful.")
    print("=" * 50)
    
    client.close()


if __name__ == "__main__":
    main()

