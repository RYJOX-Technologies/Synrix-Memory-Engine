"""
SYNRIX Guided Tour

Runs when users type: python -m synrix

This is a safe, beginner-friendly introduction that uses the mock engine only.
"""

import time

# Use absolute import to avoid relative import issues
try:
    from synrix.mock import SynrixMockClient
except ImportError:
    # Fallback for development/repo layout
    from ..mock import SynrixMockClient


def print_step(step_num, title):
    print("\n" + "=" * 60)
    print(f"STEP {step_num}: {title}")
    print("=" * 60)
    time.sleep(0.25)


def print_info(text):
    print(f"\n[INFO] {text}")
    time.sleep(0.20)


def print_success(text):
    print(f"[OK] {text}")
    time.sleep(0.15)


def run_tour():
    time.sleep(0.2)
    
    print("""
============================================================
  Welcome to SYNRIX!

  Let's build your first knowledge graph in a few minutes.
============================================================
""")
    
    print_info("What is a knowledge graph?")
    print("""
A knowledge graph stores information as connected nodes.
Think of it like Wikipedia, but structured for computers:

  • Each node = a concept or fact
  • Prefixes organize related knowledge (LANGUAGE:, CONCEPT:)
  • You can query by prefix to retrieve related ideas instantly
""")
    
    input("\nPress Enter to begin... ")
    
    # ------------------------------------------------------------
    # STEP 1 — Create the graph
    # ------------------------------------------------------------
    print_step(1, "Creating Your Knowledge Graph")
    
    try:
        client = SynrixMockClient()
        client.create_collection("my_first_graph")
    except Exception as e:
        print(f"[ERROR] Unexpected error: {e}")
        return
    
    print_success("Knowledge graph created!")
    
    # ------------------------------------------------------------
    # STEP 2 — Add nodes
    # ------------------------------------------------------------
    print_step(2, "Adding Knowledge")
    
    concepts = [
        ("LANGUAGE:Python", "A high-level programming language"),
        ("LANGUAGE:JavaScript", "The language of the web"),
        ("CONCEPT:Variable", "A container that holds a value"),
        ("CONCEPT:Function", "A reusable block of code"),
    ]
    
    print("\nAdding concepts to your graph:")
    for name, desc in concepts:
        client.add_node(name, desc, collection="my_first_graph")
        short = name.split(":")[1]
        print(f"  • Added: {short}")
        time.sleep(0.08)
    
    print_success(f"Added {len(concepts)} concepts!")
    
    # ------------------------------------------------------------
    # STEP 3 — Run queries
    # ------------------------------------------------------------
    print_step(3, "Querying Your Graph")
    
    print("\n[SEARCH] Finding all programming languages...")
    languages = client.query_prefix("LANGUAGE:", collection="my_first_graph")
    print_success(f"Found {len(languages)} languages:")
    for result in languages:
        print("  •", result["payload"]["name"].split(":")[1])
    
    print("\n[SEARCH] Finding all general concepts...")
    items = client.query_prefix("CONCEPT:", collection="my_first_graph")
    print_success(f"Found {len(items)} concepts:")
    for result in items:
        print("  •", result["payload"]["name"].split(":")[1])
    
    # ------------------------------------------------------------
    # Summary + Next Steps
    # ------------------------------------------------------------
    print("\n" + "=" * 60)
    print("Success! You built your first knowledge graph.")
    print("=" * 60)
    print("""
What you learned:

  • Knowledge graphs store facts as nodes
  • Prefixes group related information
  • Querying by prefix retrieves structured knowledge quickly

Next steps:

  • Try: python examples/first_knowledge_graph.py
  • Try: python examples/quickstart.py
  • Explore the full SDK in README.md

Tip:

This tour uses the mock engine so it's 100% safe and local.
You can switch to a real SYNRIX server later without changing your code.
""")
    
    # No need to close a mock client, but harmless
    client.close()


if __name__ == "__main__":
    run_tour()
