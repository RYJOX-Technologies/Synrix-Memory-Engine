#!/usr/bin/env python3
"""
SYNRIX Agent Memory – public demo

Shows store and recall by prefix: an agent stores key/value pairs under a
namespace and queries by prefix. No embeddings, no cloud—just local persistence.

Run (from agent-memory-sdk root, with SYNRIX_LIB_PATH set):
  pip install -e .
  python -m synrix.examples.agent_memory_demo
"""

import os
import sys

if __name__ == "__main__" and "__file__" in dir():
    _root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if _root not in sys.path:
        sys.path.insert(0, _root)

def main():
    # Use a temp lattice so the demo is self-contained and repeatable
    import tempfile
    path = os.path.join(tempfile.gettempdir(), "synrix_agent_memory_demo.lattice")
    if os.path.exists(path):
        try:
            os.remove(path)
        except Exception:
            pass

    print("SYNRIX Agent Memory Demo")
    print("========================\n")

    try:
        from synrix.ai_memory import get_ai_memory
        memory = get_ai_memory(lattice_path=path)
    except ImportError:
        print("ERROR: synrix not installed. From agent-memory-sdk: pip install -e .")
        return 1
    except Exception as e:
        print("ERROR: Synrix init failed:", e)
        print("Set SYNRIX_LIB_PATH to the directory containing libsynrix.dll")
        return 1

    if getattr(memory, "backend", None) is None:
        print("ERROR: No Synrix DLL. Set SYNRIX_LIB_PATH.")
        return 1

    try:
        # Store a few keys under a prefix (e.g. user context an agent might remember)
        PREFIX = "USER:context:"
        memory.add(PREFIX + "name", "Alice")
        memory.add(PREFIX + "role", "developer")
        memory.add(PREFIX + "preference", "prefers Python")

        print("1. Stored three keys under prefix '{}'".format(PREFIX))
        print("   (name, role, preference)\n")

        # Recall everything under that prefix
        results = memory.query(PREFIX, limit=10)
        print("2. Query by prefix '{}' – recalled {} item(s):".format(PREFIX, len(results)))
        for r in results:
            key = r.get("name", "")
            data = r.get("data", "")
            if key and data:
                short = key.replace(PREFIX, "") if key.startswith(PREFIX) else key
                print("   {} -> {}".format(short, data))

        print("\nDone. Agent memory uses prefix keys and local persistence only.")
    finally:
        memory.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
