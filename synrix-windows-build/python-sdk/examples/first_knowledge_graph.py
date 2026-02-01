#!/usr/bin/env python3
"""
Your First Knowledge Graph - Guided Tutorial

A step-by-step tutorial for building your first knowledge graph with SYNRIX.
Perfect for beginners - no prior knowledge graph experience needed!

This tutorial will teach you:
1. What a knowledge graph is (in simple terms)
2. How to create nodes (pieces of knowledge)
3. How to connect related concepts
4. How to query your graph
5. How to build something useful

Let's build a knowledge graph about programming concepts!

INSTALLATION:
  First, install the SDK:
    cd python-sdk
    pip install -e .
  
  Then run this tutorial:
    python examples/first_knowledge_graph.py
"""

# Try to import synrix - works if installed via pip
try:
    from synrix import SynrixMockClient
except ImportError:
    # If not installed, try importing from parent directory (repo layout)
    import sys
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)  # python-sdk directory
    if parent_dir not in sys.path:
        sys.path.insert(0, parent_dir)
    try:
        from synrix import SynrixMockClient
    except ImportError:
        print("=" * 60)
        print("❌ SYNRIX SDK not found!")
        print("=" * 60)
        print("\nPlease install the SDK first:")
        print("\n  pip install -e .")
        print("\nOr from PyPI:")
        print("\n  pip install synrix")
        print("\nThen run:")
        print("  python examples/first_knowledge_graph.py")
        sys.exit(1)

import time


def print_step(step_num, title):
    """Print a formatted step header"""
    print("\n" + "=" * 60)
    print(f"STEP {step_num}: {title}")
    print("=" * 60)
    time.sleep(0.5)


def print_info(text):
    """Print informational text"""
    print(f"\n💡 {text}")
    time.sleep(0.3)


def print_success(text):
    """Print success message"""
    print(f"✅ {text}")
    time.sleep(0.2)


def wait_for_user():
    """Wait for user to press Enter"""
    input("\nPress Enter to continue... ")


def main():
    print("""
╔════════════════════════════════════════════════════════════╗
║                                                            ║
║   Your First Knowledge Graph - Guided Tutorial             ║
║                                                            ║
║   Learn to build a knowledge graph in 5 simple steps!      ║
║                                                            ║
╚════════════════════════════════════════════════════════════╝
    """)
    
    print_info("What is a knowledge graph?")
    print("""
A knowledge graph is a way to store information where:
  • Each piece of information is a "node" (like a fact or concept)
  • Nodes can be connected to show relationships
  • You can query the graph to find related information quickly

Think of it like Wikipedia, but structured so computers can understand
the relationships between concepts automatically.
    """)
    
    wait_for_user()
    
    # Initialize client
    print_step(1, "Setting Up Your Knowledge Graph")
    print("\nFirst, let's create a SYNRIX client...")
    client = SynrixMockClient()
    print_success("Client created!")
    
    print_info("We're using the mock client, which means:")
    print("  • No server needed - everything runs in memory")
    print("  • Perfect for learning and testing")
    print("  • Same API as the real SYNRIX engine")
    
    print("\nNow let's create a collection to store our knowledge:")
    collection_name = "programming_concepts"
    client.create_collection(collection_name)
    print_success(f"Collection '{collection_name}' created!")
    
    wait_for_user()
    
    # Add nodes
    print_step(2, "Adding Your First Nodes")
    print("\nNodes are the building blocks of your knowledge graph.")
    print("Each node represents a concept, fact, or piece of information.")
    
    print("\nLet's add some programming concepts:")
    concepts = [
        ("PROGRAMMING_LANGUAGE:Python", "Python is a high-level programming language known for its simplicity"),
        ("PROGRAMMING_LANGUAGE:JavaScript", "JavaScript is a programming language used for web development"),
        ("CONCEPT:Variable", "A variable is a container that stores a value"),
        ("CONCEPT:Function", "A function is a reusable block of code that performs a specific task"),
        ("CONCEPT:Class", "A class is a blueprint for creating objects"),
    ]
    
    node_ids = []
    for name, description in concepts:
        node_id = client.add_node(name, description, collection=collection_name)
        node_ids.append((node_id, name))
        print(f"  ✅ Added: {name}")
        print(f"     Description: {description[:60]}...")
        time.sleep(0.2)
    
    print_success(f"Added {len(concepts)} nodes to your knowledge graph!")
    
    print_info("Notice the naming pattern:")
    print("  • PROGRAMMING_LANGUAGE:Python")
    print("  • CONCEPT:Variable")
    print("\nThis prefix pattern (PROGRAMMING_LANGUAGE:, CONCEPT:) helps us")
    print("organize and query related nodes together!")
    
    wait_for_user()
    
    # Query by prefix
    print_step(3, "Querying Your Knowledge Graph")
    print("\nNow let's search for nodes by their category (prefix):")
    
    print("\n🔍 Searching for all programming languages...")
    languages = client.query_prefix("PROGRAMMING_LANGUAGE:", collection=collection_name)
    print_success(f"Found {len(languages)} programming languages:")
    for result in languages:
        name = result.get("payload", {}).get("name", "unknown")
        data = result.get("payload", {}).get("data", "")
        print(f"  • {name.replace('PROGRAMMING_LANGUAGE:', '')}: {data[:50]}...")
    
    print("\n🔍 Searching for all concepts...")
    concepts_found = client.query_prefix("CONCEPT:", collection=collection_name)
    print_success(f"Found {len(concepts_found)} concepts:")
    for result in concepts_found:
        name = result.get("payload", {}).get("name", "unknown")
        data = result.get("payload", {}).get("data", "")
        print(f"  • {name.replace('CONCEPT:', '')}: {data[:50]}...")
    
    print_info("This is the power of prefix queries!")
    print("  • Fast retrieval of related concepts")
    print("  • Organized by category automatically")
    print("  • Scales to millions of nodes efficiently")
    
    wait_for_user()
    
    # Add more nodes
    print_step(4, "Expanding Your Knowledge Graph")
    print("\nLet's add more concepts to make your graph more useful:")
    
    more_concepts = [
        ("CONCEPT:List", "A list is an ordered collection of items"),
        ("CONCEPT:Dictionary", "A dictionary stores key-value pairs"),
        ("CONCEPT:Loop", "A loop repeats code until a condition is met"),
        ("PROGRAMMING_LANGUAGE:Java", "Java is an object-oriented programming language"),
        ("PROGRAMMING_LANGUAGE:C++", "C++ is a general-purpose programming language"),
    ]
    
    for name, description in more_concepts:
        node_id = client.add_node(name, description, collection=collection_name)
        print(f"  ✅ Added: {name}")
        time.sleep(0.1)
    
    print_success(f"Added {len(more_concepts)} more nodes!")
    
    # Show updated counts
    print("\n📊 Your knowledge graph now contains:")
    languages = client.query_prefix("PROGRAMMING_LANGUAGE:", collection=collection_name)
    concepts = client.query_prefix("CONCEPT:", collection=collection_name)
    print(f"  • {len(languages)} programming languages")
    print(f"  • {len(concepts)} programming concepts")
    
    wait_for_user()
    
    # Real-world example
    print_step(5, "Building Something Useful")
    print("\nNow let's build a simple 'programming tutor' that can answer questions!")
    
    print("\nExample: 'What programming languages do you know about?'")
    languages = client.query_prefix("PROGRAMMING_LANGUAGE:", collection=collection_name)
    print("\nAnswer:")
    for result in languages:
        name = result.get("payload", {}).get("name", "unknown")
        lang_name = name.replace("PROGRAMMING_LANGUAGE:", "")
        print(f"  • {lang_name}")
    
    print("\nExample: 'What is a variable?'")
    variables = client.query_prefix("CONCEPT:Variable", collection=collection_name)
    if variables:
        answer = variables[0].get("payload", {}).get("data", "")
        print(f"\nAnswer: {answer}")
    
    print_info("This is how knowledge graphs power AI systems:")
    print("  • Store structured knowledge")
    print("  • Query quickly for relevant information")
    print("  • Use in RAG (Retrieval-Augmented Generation) systems")
    print("  • Build intelligent assistants")
    
    wait_for_user()
    
    # Summary
    print("\n" + "=" * 60)
    print("🎉 CONGRATULATIONS!")
    print("=" * 60)
    print("\nYou've built your first knowledge graph!")
    
    print("\n📚 What you learned:")
    print("  ✅ What a knowledge graph is")
    print("  ✅ How to create nodes (pieces of knowledge)")
    print("  ✅ How to organize nodes with prefixes")
    print("  ✅ How to query your graph")
    print("  ✅ How knowledge graphs are used in AI")
    
    print("\n🚀 Next steps:")
    print("  1. Add more nodes to your graph")
    print("  2. Try different prefix patterns")
    print("  3. Build a simple Q&A system")
    print("  4. Connect to a real SYNRIX server")
    print("  5. Integrate with an LLM for RAG")
    
    print("\n💡 Tips:")
    print("  • Use consistent prefix patterns (CATEGORY:Item)")
    print("  • Keep node descriptions clear and concise")
    print("  • Organize related concepts with the same prefix")
    print("  • Query by prefix for fast retrieval")
    
    print("\n📖 Resources:")
    print("  • Check examples/hello_memory.py for more examples")
    print("  • Read the README.md for full API documentation")
    print("  • Experiment with the mock client - it's free!")
    
    print("\n" + "=" * 60)
    print("Happy knowledge graph building! 🎓")
    print("=" * 60 + "\n")
    
    client.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n👋 Tutorial interrupted. Come back anytime to continue learning!")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        print("Don't worry - this is just a tutorial. Try running it again!")

