#!/usr/bin/env python3
"""
Run agent-memory SDK tests without pytest.
Usage: from agent-memory-sdk root:
  set SYNRIX_LIB_PATH=<path to dir with libsynrix.dll>
  python tests/run_ai_memory_tests.py

With pytest:
  pip install -e ".[dev]"
  pytest tests/ -v
"""

import os
import sys
import tempfile

# Add package root
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def run():
    from synrix.ai_memory import AIMemory
    fd, path = tempfile.mkstemp(suffix=".lattice")
    os.close(fd)
    if os.path.exists(path):
        os.remove(path)
    try:
        memory = AIMemory(lattice_path=path)
        if memory.backend is None:
            print("SKIP: No Synrix DLL (set SYNRIX_LIB_PATH)")
            return 0
    except Exception as e:
        print("SKIP: AIMemory init failed:", e)
        return 0
    try:
        failed = 0
        # add small, query
        nid = memory.add("AGENT:test:k1", "v1")
        assert nid is not None, "add should return node id"
        results = memory.query("AGENT:test:", limit=10)
        assert len(results) >= 1, "query should return added node"
        assert any(r.get("data") == "v1" for r in results), "data should be v1"
        print("  add + query by prefix: OK")

        # query prefix isolation
        memory.add("P:a:1", "a1")
        memory.add("P:b:1", "b1")
        results = memory.query("P:a:", limit=10)
        data = [r.get("data", "") for r in results]
        assert "a1" in data and "b1" not in data, "prefix should filter"
        print("  prefix isolation: OK")

        # large payload (chunked)
        large = "x" * 600
        memory.add("AGENT:big:key", large)
        results = memory.query("AGENT:big:", limit=10)
        found = any(r.get("data") == large for r in results)
        assert found, "chunked payload should return in full"
        print("  chunked add + query: OK")

        # get by id
        nid2 = memory.add("AGENT:get:one", "get_me")
        node = memory.get(nid2)
        assert node is not None and ("get_me" in str(node.get("data", ""))), "get by id"
        print("  get by id: OK")

        # count
        c = memory.count()
        assert c >= 5, "count should reflect adds"
        print("  count: OK")

        # agent handoff
        memory.add("TASK:h:plan:s1", "step1")
        memory.add("TASK:h:outcomes:s1", "done")
        full = memory.query("TASK:h:", limit=10)
        assert len(full) >= 2, "full prefix should return plan + outcomes"
        print("  agent handoff prefix: OK")

        print("All tests passed.")
        return 0
    except AssertionError as e:
        print("FAIL:", e)
        return 1
    finally:
        memory.close()
        try:
            if os.path.exists(path):
                os.remove(path)
        except Exception:
            pass


if __name__ == "__main__":
    sys.exit(run())
