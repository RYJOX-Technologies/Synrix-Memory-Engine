#!/usr/bin/env python3
"""
Optional slow test: verify the 25001st node raises FreeTierLimitError (free tier).

Run only when you need to confirm the 25k cap is enforced (takes a few minutes):
  set SYNRIX_LIB_PATH=<dir with libsynrix.dll>
  set SYNRIX_LICENSE_KEY=   (unset for free tier)
  python tests/test_hit_limit_25k.py

Expect: 25000 adds succeed; 25001st add raises FreeTierLimitError.
"""

import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

LIMIT = 25000
BATCH = 5000  # progress every N nodes


def main():
    if os.environ.get("SYNRIX_LICENSE_KEY", "").strip():
        print("Unset SYNRIX_LICENSE_KEY to test free tier limit.")
        return 1
    from synrix.raw_backend import RawSynrixBackend, FreeTierLimitError
    path = tempfile.mktemp(suffix=".lattice")
    if os.path.exists(path):
        os.remove(path)
    try:
        b = RawSynrixBackend(path, max_nodes=LIMIT, evaluation_mode=True)
        for i in range(1, LIMIT + 2):
            key = "limit:n%u" % i
            try:
                b.add_node(key, "v", node_type=5)
            except FreeTierLimitError:
                if i == LIMIT + 1:
                    print("[OK] 25001st add raised FreeTierLimitError (limit enforced).")
                    b.close()
                    return 0
                raise
            if i % BATCH == 0:
                print("  %u nodes..." % i)
        b.close()
        print("[FAIL] Expected FreeTierLimitError on 25001st add.")
        return 1
    except Exception as e:
        print("[ERROR] %s" % e)
        return 1
    finally:
        if os.path.exists(path):
            try:
                os.remove(path)
            except Exception:
                pass


if __name__ == "__main__":
    sys.exit(main())
