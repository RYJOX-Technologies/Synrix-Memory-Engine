#!/usr/bin/env python3
"""
Full test against a REAL lattice file. Run once, verify everything, then delete.
Usage (from agent-memory-sdk root):
  set SYNRIX_LIB_PATH=<dir with libsynrix.dll>
  python tests/test_real_lattice.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Real lattice path - we will delete it after
LATTICE_PATH = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tests", "real_test_lattice.lattice"
)


def main():
    if os.path.exists(LATTICE_PATH):
        os.remove(LATTICE_PATH)
    try:
        from synrix.ai_memory import AIMemory
        memory = AIMemory(lattice_path=LATTICE_PATH)
    except Exception as e:
        print("FAIL: Could not init AIMemory:", e)
        return 1
    if memory.backend is None:
        print("FAIL: No Synrix DLL. Set SYNRIX_LIB_PATH.")
        return 1

    errors = []
    try:
        # 1. Add small, query by prefix
        n1 = memory.add("REAL:small:key1", "value1")
        if n1 is None:
            errors.append("add small returned None")
        r = memory.query("REAL:small:", limit=10)
        if not r or not any(x.get("data") == "value1" for x in r):
            errors.append("query did not return small value")
        else:
            print("[OK] add small + query by prefix")

        # 2. Prefix isolation
        memory.add("REAL:P:a:1", "a1")
        memory.add("REAL:P:b:1", "b1")
        r = memory.query("REAL:P:a:", limit=10)
        data = [x.get("data", "") for x in r]
        if "b1" in data or "a1" not in data:
            errors.append("prefix isolation failed")
        else:
            print("[OK] prefix isolation")

        # 3. Large payload (chunked)
        large = "LARGE_PAYLOAD_" + ("x" * 600)
        memory.add("REAL:chunked:big", large)
        r = memory.query("REAL:chunked:", limit=10)
        if not any(x.get("data") == large for x in r):
            errors.append("chunked payload not returned in full")
        else:
            print("[OK] chunked add + query full")

        # 4. Get by id
        n2 = memory.add("REAL:get:id_key", "id_value")
        node = memory.get(n2)
        if not node or "id_value" not in str(node.get("data", "")):
            errors.append("get by id failed")
        else:
            print("[OK] get by id")

        # 5. Count
        c = memory.count()
        if c < 5:
            errors.append("count too low")
        else:
            print("[OK] count =", c)

        # 6. Agent handoff (plan / outcomes / full prefix)
        memory.add("REAL:TASK:t1:plan:s1", "plan_step_1")
        memory.add("REAL:TASK:t1:plan:s2", "plan_step_2")
        memory.add("REAL:TASK:t1:outcomes:s1", "done_1")
        memory.add("REAL:TASK:t1:outcomes:s2", "done_2")
        full = memory.query("REAL:TASK:t1:", limit=20)
        if len(full) < 4:
            errors.append("agent handoff prefix returned too few")
        data_full = [x.get("data", "") for x in full]
        if "plan_step_1" not in data_full or "done_1" not in data_full:
            errors.append("agent handoff data missing")
        else:
            print("[OK] agent handoff (plan + outcomes under TASK prefix)")

        # 7. Persistence: close and reopen same lattice
        memory.close()
        memory2 = AIMemory(lattice_path=LATTICE_PATH)
        if memory2.backend is None:
            errors.append("reopen: backend None")
        else:
            r2 = memory2.query("REAL:small:", limit=5)
            if not r2 or not any(x.get("data") == "value1" for x in r2):
                errors.append("reopen: data not persisted")
            else:
                print("[OK] persistence (close + reopen, data still there)")
            memory2.close()

    finally:
        memory.close()
        if os.path.exists(LATTICE_PATH):
            try:
                os.remove(LATTICE_PATH)
                print("[OK] lattice file deleted")
            except Exception as e:
                print("[WARN] could not delete lattice:", e)

    if errors:
        print("\nFAILED:", errors)
        return 1
    print("\nAll real-lattice tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
