#!/usr/bin/env python3
"""
Agent Memory Demo for Joseph

Shows an agent using Synrix as its memory: it stores what it learns (e.g. your
name and a fact), then recalls by prefix when needed. No embeddings, no RAG -
just structured keys and prefix lookup.

Run (from agent-memory-sdk root):
  1. pip install -e .
  2. set SYNRIX_LIB_PATH=<directory containing libsynrix.dll>
  3. python -m synrix.examples.joseph_agent_memory_demo

Optional: run again without deleting the lattice to see persistence
(the agent "remembers" from the previous run).
"""

import os
import sys

if __name__ == "__main__" and "__file__" in dir():
    _root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if _root not in sys.path:
        sys.path.insert(0, _root)

LATTICE_PATH = os.path.join(os.path.dirname(__file__), "joseph_agent_demo.lattice")
PREFIX = "AGENT:demo:user:"


def main():
    interactive = len(sys.argv) <= 1 or "--interactive" in sys.argv
    if "--fresh" in sys.argv and os.path.exists(LATTICE_PATH):
        try:
            os.remove(LATTICE_PATH)
        except Exception:
            pass

    print("=" * 60)
    print("  Agent Memory Demo (for Joseph)")
    print("=" * 60)
    print()
    print("This demo shows an agent using Synrix as its memory.")
    print("The agent stores what it learns, then recalls by prefix.")
    print()

    try:
        from synrix.ai_memory import AIMemory
        memory = AIMemory(lattice_path=LATTICE_PATH)
    except ImportError as e:
        print("ERROR: synrix not installed. From agent-memory-sdk: pip install -e .")
        return 1
    except Exception as e:
        print("ERROR: Synrix init failed:", e)
        print("Set SYNRIX_LIB_PATH to the directory containing libsynrix.dll")
        return 1
    if memory.backend is None:
        print("ERROR: No Synrix DLL. Set SYNRIX_LIB_PATH.")
        return 1

    try:
        # --- What does the agent "know"? (from this run or previous) ---
        existing = memory.query(PREFIX, limit=20)
        if existing:
            print("[Agent memory already has context from a previous run.]")
            print()
        else:
            # Use defaults unless running interactively with a TTY (so Joseph can type when he wants)
            if interactive and sys.stdin.isatty():
                print("Tell the agent something to remember (or press Enter for defaults).")
                try:
                    name = input("Your name [Joseph]: ").strip() or "Joseph"
                    fact = input("One fact (e.g. project or role) [Synrix]: ").strip() or "Synrix"
                except EOFError:
                    name, fact = "Joseph", "Synrix"
            else:
                name, fact = "Joseph", "Synrix"
            memory.add(PREFIX + "name", name)
            memory.add(PREFIX + "fact", fact)
            print()
            print("[Agent stored: name = {}, fact = {}]".format(name, fact))
            print()

        # --- Agent "recall": what does it know? ---
        print("--- Agent recall (query by prefix: {}) ---".format(PREFIX))
        results = memory.query(PREFIX, limit=20)
        for r in results:
            key = r.get("name", "")
            data = r.get("data", "")
            if key and data:
                short_key = key.replace(PREFIX, "") if key.startswith(PREFIX) else key
                print("  {} -> {}".format(short_key, data))
        print()

        # --- Summary ---
        print("The agent did not use embeddings or vector search.")
        print("It stored key/value and recalled by prefix. Lattice: {}".format(LATTICE_PATH))
        print()
        print("Run again to see persistence, or use --fresh to start with an empty lattice.")
    finally:
        memory.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
