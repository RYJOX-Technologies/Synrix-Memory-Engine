"""
SYNRIX Guided Tour

Runs when users type: python -m synrix

Uses the real SYNRIX engine (requires libsynrix.dll; set SYNRIX_LIB_PATH).
"""

import time
import tempfile
import os

def print_step(step_num, title):
    print("\n" + "=" * 60)
    print(f"STEP {step_num}: {title}")
    print("=" * 60)
    time.sleep(0.25)


def print_info(text):
    print(f"\n💡 {text}")
    time.sleep(0.20)


def print_success(text):
    print(f"✅ {text}")
    time.sleep(0.15)


def run_tour():
    time.sleep(0.2)

    print(r"""
╔════════════════════════════════════════════════════════════╗
║                                                            ║
║  Welcome to SYNRIX!                                        ║
║                                                            ║
║  Let's build your first knowledge graph in a few minutes. ║
║                                                            ║
╚════════════════════════════════════════════════════════════╝
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
    # STEP 1 — Open memory (real engine)
    # ------------------------------------------------------------
    print_step(1, "Opening SYNRIX Memory")

    path = tempfile.mktemp(suffix=".lattice")
    try:
        from ..ai_memory import get_ai_memory
        memory = get_ai_memory(lattice_path=path)
        if memory.backend is None:
            raise RuntimeError("DLL not loaded")
    except Exception as e:
        print(f"❌ Could not load SYNRIX engine: {e}")
        print("   Set SYNRIX_LIB_PATH to the directory containing libsynrix.dll and try again.")
        return

    print_success("Memory opened!")

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
        memory.add(name, desc)
        short = name.split(":")[1]
        print(f"  • Added: {short}")
        time.sleep(0.08)

    print_success(f"Added {len(concepts)} concepts!")

    # ------------------------------------------------------------
    # STEP 3 — Run queries
    # ------------------------------------------------------------
    print_step(3, "Querying Your Graph")

    print("\n🔍 Finding all programming languages...")
    languages = memory.query("LANGUAGE:", limit=10)
    print_success(f"Found {len(languages)} languages:")
    for r in languages:
        print("  •", r.get("name", "").split(":")[-1])

    print("\n🔍 Finding all general concepts...")
    items = memory.query("CONCEPT:", limit=10)
    print_success(f"Found {len(items)} concepts:")
    for r in items:
        print("  •", r.get("name", "").split(":")[-1])

    # ------------------------------------------------------------
    # Summary + Next Steps
    # ------------------------------------------------------------
    print("\n" + "=" * 60)
    print("🎉 Success! You built your first knowledge graph.")
    print("=" * 60)
    print("""
📚 What you learned:

  • Knowledge graphs store facts as nodes
  • Prefixes group related information
  • Querying by prefix retrieves structured knowledge quickly

🚀 Next steps:

  • See README.md for RoboticsNexus and agent memory usage
  • Set SYNRIX_LIB_PATH for your own lattices
""")
    try:
        memory.close()
    except Exception:
        pass


if __name__ == "__main__":
    run_tour()
