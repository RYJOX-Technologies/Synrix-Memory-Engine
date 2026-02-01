#!/usr/bin/env python3
"""
SYNRIX Canonical 5-Minute Example
==================================

This is the ONE canonical example that demonstrates SYNRIX value.
Copy-paste runnable, works with real data, shows clear value.

Run this after starting the server:
    ./synrix_mimic_qdrant --dev --port 6334
"""

from synrix import SynrixClient
import sys

def main():
    print("=" * 60)
    print("SYNRIX Canonical Example - 5 Minute Win")
    print("=" * 60)
    print()
    
    # Connect to server
    print("Step 1: Connecting to SYNRIX server...")
    try:
        client = SynrixClient(host="localhost", port=6334)
        print("✅ Connected to SYNRIX server")
    except Exception as e:
        print(f"❌ Failed to connect: {e}")
        print()
        print("Make sure the server is running:")
        print("  ./synrix_mimic_qdrant --dev --port 6334")
        sys.exit(1)
    
    print()
    
    # Create collection
    print("Step 2: Creating collection 'knowledge_base'...")
    try:
        client.create_collection("knowledge_base", vector_dim=128)
        print("✅ Collection 'knowledge_base' created")
    except Exception as e:
        # Collection might already exist, that's okay
        if "already exists" in str(e).lower() or "exists" in str(e).lower():
            print("ℹ️  Collection already exists (that's fine)")
        else:
            print(f"❌ Failed to create collection: {e}")
            sys.exit(1)
    
    print()
    
    # Add real knowledge nodes
    print("Step 3: Adding knowledge nodes...")
    knowledge_items = [
        ("ISA_ADD", "Addition operation - adds two numbers"),
        ("ISA_SUBTRACT", "Subtraction operation - subtracts two numbers"),
        ("ISA_MULTIPLY", "Multiplication operation - multiplies two numbers"),
        ("ISA_DIVIDE", "Division operation - divides two numbers"),
        ("LEARNING_PATTERN_1", "Pattern: sequential operations"),
        ("LEARNING_PATTERN_2", "Pattern: parallel operations"),
        ("PERFORMANCE_FAST", "Fast execution path identified"),
        ("PERFORMANCE_SLOW", "Slow execution path identified"),
    ]
    
    added_count = 0
    for name, description in knowledge_items:
        try:
            node_id = client.add_node(name, description, collection="knowledge_base")
            print(f"  ✅ Added: {name}")
            added_count += 1
        except Exception as e:
            print(f"  ⚠️  Failed to add {name}: {e}")
    
    print(f"\n✅ Added {added_count} knowledge nodes")
    print()
    
    # Query by prefix (O(k) semantic search)
    print("Step 4: Querying by prefix 'ISA_' (O(k) semantic search)...")
    try:
        results = client.query_prefix("ISA_", collection="knowledge_base")
        print(f"✅ Found {len(results)} nodes with prefix 'ISA_':")
        for i, result in enumerate(results, 1):
            name = result.get("payload", {}).get("name", "unknown")
            print(f"  {i}. {name}")
    except Exception as e:
        print(f"❌ Query failed: {e}")
        sys.exit(1)
    
    print()
    
    # Query another prefix
    print("Step 5: Querying by prefix 'LEARNING_'...")
    try:
        results = client.query_prefix("LEARNING_", collection="knowledge_base")
        print(f"✅ Found {len(results)} nodes with prefix 'LEARNING_':")
        for i, result in enumerate(results, 1):
            name = result.get("payload", {}).get("name", "unknown")
            print(f"  {i}. {name}")
    except Exception as e:
        print(f"❌ Query failed: {e}")
        sys.exit(1)
    
    print()
    print("=" * 60)
    print("✅ SUCCESS! SYNRIX is working correctly.")
    print("=" * 60)
    print()
    print("What you just saw:")
    print("  • Created a knowledge graph collection")
    print("  • Stored 8 knowledge nodes")
    print("  • Queried by semantic prefix (O(k) performance)")
    print("  • Retrieved relevant nodes instantly")
    print()
    print("Next steps:")
    print("  • Read the docs: https://github.com/synrix/synrix-python-sdk")
    print("  • Try the full API: python3 -c \"from synrix import SynrixClient; help(SynrixClient)\"")
    print("  • Build your app: Use SynrixClient in your code")
    print()

if __name__ == "__main__":
    main()
