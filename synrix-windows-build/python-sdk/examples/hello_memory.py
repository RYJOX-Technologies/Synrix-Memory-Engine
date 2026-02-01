#!/usr/bin/env python3
"""
SYNRIX Hello Memory Example

A simple example demonstrating basic SYNRIX operations:
- Create a collection
- Add nodes to the knowledge graph
- Query nodes by prefix
- Retrieve nodes
"""

# Try to import synrix - works if installed via pip
try:
    from synrix import SynrixClient, SynrixError
except ImportError:
    # If not installed, try importing from parent directory (repo layout)
    import sys
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)  # python-sdk directory
    if parent_dir not in sys.path:
        sys.path.insert(0, parent_dir)
    from synrix import SynrixClient, SynrixError


def main():
    print("=" * 60)
    print("SYNRIX Hello Memory Example")
    print("=" * 60)
    print()
    
    # Initialize client
    print("1. Connecting to SYNRIX server...")
    try:
        client = SynrixClient(host="localhost", port=6334)
        print("   ✅ Connected to SYNRIX server")
    except SynrixError as e:
        print(f"   ❌ Failed to connect: {e}")
        print("\n   Make sure SYNRIX server is running on localhost:6334")
        return
    
    print()
    
    # Create a collection
    collection_name = "hello_memory"
    print(f"2. Creating collection '{collection_name}'...")
    try:
        client.create_collection(collection_name)  # Uses engine default
        print(f"   ✅ Collection '{collection_name}' created")
    except SynrixError as e:
        print(f"   ⚠️  Collection may already exist: {e}")
        # Try to get it
        try:
            info = client.get_collection(collection_name)
            print(f"   ✅ Collection '{collection_name}' exists ({info.get('result', {}).get('points_count', 0)} points)")
        except SynrixError:
            print(f"   ❌ Failed to create or access collection: {e}")
            return
    
    print()
    
    # Add some nodes
    print("3. Adding nodes to the knowledge graph...")
    nodes = [
        ("ISA_ADD", "Addition operation: takes two numbers and returns their sum"),
        ("ISA_SUBTRACT", "Subtraction operation: takes two numbers and returns their difference"),
        ("ISA_MULTIPLY", "Multiplication operation: takes two numbers and returns their product"),
        ("ISA_DIVIDE", "Division operation: takes two numbers and returns their quotient"),
        ("QDRANT_COLLECTION:documents", "Document collection for RAG system"),
        ("QDRANT_COLLECTION:embeddings", "Embedding vectors for semantic search"),
    ]
    
    node_ids = []
    for name, data in nodes:
        try:
            node_id = client.add_node(name, data, collection=collection_name)
            if node_id:
                node_ids.append((node_id, name))
                print(f"   ✅ Added node: {name} (ID: {node_id})")
            else:
                print(f"   ⚠️  Failed to add node: {name}")
        except SynrixError as e:
            print(f"   ❌ Error adding node {name}: {e}")
    
    print()
    
    # Query by prefix
    print("4. Querying nodes by prefix 'ISA_'...")
    try:
        results = client.query_prefix("ISA_", collection=collection_name)
        print(f"   ✅ Found {len(results)} nodes with prefix 'ISA_':")
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "unknown")
            data = payload.get("data", "")
            print(f"      - {name}: {data[:50]}...")
    except SynrixError as e:
        print(f"   ❌ Error querying prefix: {e}")
    
    print()
    
    # Query another prefix
    print("5. Querying nodes by prefix 'QDRANT_COLLECTION:'...")
    try:
        results = client.query_prefix("QDRANT_COLLECTION:", collection=collection_name)
        print(f"   ✅ Found {len(results)} nodes with prefix 'QDRANT_COLLECTION:':")
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "unknown")
            print(f"      - {name}")
    except SynrixError as e:
        print(f"   ❌ Error querying prefix: {e}")
    
    print()
    
    # List collections
    print("6. Listing all collections...")
    try:
        collections = client.list_collections()
        print(f"   ✅ Found {len(collections)} collections:")
        for col in collections:
            try:
                info = client.get_collection(col)
                count = info.get("result", {}).get("points_count", 0)
                print(f"      - {col} ({count} points)")
            except SynrixError:
                print(f"      - {col}")
    except SynrixError as e:
        print(f"   ❌ Error listing collections: {e}")
    
    print()
    print("=" * 60)
    print("Example completed!")
    print("=" * 60)
    print()
    print("Next steps:")
    print("  - Try adding more nodes with different prefixes")
    print("  - Use query_prefix() for O(k) semantic queries")
    print("  - Use search_points() for vector similarity search")
    print("  - Check the SYNRIX documentation for more examples")
    
    # Clean up
    client.close()


if __name__ == "__main__":
    main()

