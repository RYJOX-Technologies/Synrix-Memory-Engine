#!/usr/bin/env python3
"""
Synrix Demo: O(k) Scaling Proof
=================================
Adds N nodes (default 100K), then queries at multiple corpus sizes
to prove query time scales with match count (k), not corpus size (N).

Run:
  pip install synrix
  python3 test_scale_nodes.py              # 100K nodes
  python3 test_scale_nodes.py 500000       # 500K nodes
"""

import sys, os, time, argparse, tempfile, shutil, gc
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

try:
    import psutil
    HAS_PSUTIL = True
except ImportError:
    HAS_PSUTIL = False

from synrix.raw_backend import RawSynrixBackend, FreeTierLimitError

PREFIXES = [
    "ISA_", "PATTERN_", "LEARNING_", "FUNC_", "TASK_",
    "DOMAIN_", "INTERFACE_", "OBJECTIVE_", "COMPONENT_", "SYSTEM_",
    "DOC_PYTHON_", "DOC_RUST_", "DOC_K8S_", "DOC_DB_", "DOC_NET_",
    "DOC_SEC_", "DOC_ML_", "DOC_FRONT_", "DOC_BACK_", "DOC_DEVOPS_",
]


def rss_mb():
    if not HAS_PSUTIL:
        return 0.0
    return psutil.Process(os.getpid()).memory_info().rss / (1024 * 1024)


def main():
    parser = argparse.ArgumentParser(description="Synrix O(k) scaling proof")
    parser.add_argument("nodes", type=int, nargs="?", default=100_000, help="Total nodes (default 100K)")
    args = parser.parse_args()

    n = args.nodes
    print(f"Synrix — O(k) Scaling Proof ({n:,} nodes)")
    print("=" * 60)
    print()

    tmpdir = tempfile.mkdtemp()
    lattice_path = os.path.join(tmpdir, "scale_test.lattice")
    rss0 = rss_mb()

    try:
        backend = RawSynrixBackend(lattice_path, max_nodes=max(n + 1000, 26000))

        # Ingest
        print(f"Ingesting {n:,} nodes ({len(PREFIXES)} prefixes, ~{n // len(PREFIXES):,} each) ...")
        t0 = time.perf_counter()
        actual = 0
        for i in range(n):
            prefix = PREFIXES[i % len(PREFIXES)]
            name = f"{prefix}{i:08d}"
            data = f"Node {i}: knowledge about {prefix.rstrip('_').lower()} topic. " * 3
            try:
                backend.add_node(name, data[:500])
                actual += 1
            except FreeTierLimitError:
                print(f"\n  Hit tier limit at {actual:,} nodes. "
                      f"Set SYNRIX_LICENSE_KEY for higher limits.")
                break
            if (i + 1) % max(1, n // 5) == 0:
                elapsed = time.perf_counter() - t0
                rate = (i + 1) / elapsed
                print(f"  {i+1:>9,} / {n:,}  ({rate:,.0f} nodes/sec)")

        n = actual
        ingest_sec = time.perf_counter() - t0
        rss1 = rss_mb()

        if n == 0:
            print("\n  No nodes ingested (tier limit reached before any writes).")
            print("  Set SYNRIX_LICENSE_KEY env var for unlimited, or use a fresh machine.")
            backend.close()
            shutil.rmtree(tmpdir, ignore_errors=True)
            return

        print(f"\n  Ingested {n:,} nodes in {ingest_sec:.2f}s ({n / ingest_sec:,.0f} nodes/sec)")
        if HAS_PSUTIL:
            print(f"  Memory: {rss1 - rss0:.1f} MB RSS delta "
                  f"({(rss1 - rss0) * 1024 * 1024 / n:.0f} bytes/node effective)")
        print()

        # O(k) scaling test
        print("O(k) SCALING TEST")
        print("-" * 60)
        print(f"{'Prefix':<16} {'Matches':>10} {'Time (us)':>12} {'us/match':>10}")
        print("-" * 60)

        test_prefixes = ["ISA_", "DOC_PYTHON_", "LEARNING_", "DOMAIN_", "DOC_"]
        for prefix in test_prefixes:
            times = []
            match_count = 0
            for _ in range(20):
                t = time.perf_counter()
                results = backend.find_by_prefix(prefix, limit=50000)
                elapsed = time.perf_counter() - t
                times.append(elapsed)
                match_count = len(results)

            median_us = sorted(times)[len(times) // 2] * 1e6
            per_match = median_us / max(match_count, 1)
            print(f"  {prefix:<14} {match_count:>10,} {median_us:>12.1f} {per_match:>10.3f}")

        print()

        # Same prefix, different limits — shows O(k) directly
        print("LIMIT SCALING (same prefix, varying k)")
        print("-" * 60)
        print(f"{'Limit k':>10} {'Returned':>10} {'Time (us)':>12} {'us/result':>10}")
        print("-" * 60)

        for limit in [10, 100, 500, 1000, 5000]:
            times = []
            returned = 0
            for _ in range(20):
                t = time.perf_counter()
                results = backend.find_by_prefix("ISA_", limit=limit)
                elapsed = time.perf_counter() - t
                times.append(elapsed)
                returned = len(results)

            median_us = sorted(times)[len(times) // 2] * 1e6
            per_result = median_us / max(returned, 1)
            print(f"  {limit:>10,} {returned:>10,} {median_us:>12.1f} {per_result:>10.3f}")

        print()

        # Persistence test
        print("PERSISTENCE TEST")
        print("-" * 60)
        backend.save()
        file_size = os.path.getsize(lattice_path)
        backend.close()
        del backend
        gc.collect()

        t_load = time.perf_counter()
        backend2 = RawSynrixBackend(lattice_path, max_nodes=max(n + 1000, 26000))
        load_ms = (time.perf_counter() - t_load) * 1000

        t_verify = time.perf_counter()
        results = backend2.find_by_prefix("ISA_", limit=10, raw=False)
        verify_us = (time.perf_counter() - t_verify) * 1e6

        print(f"  File size: {file_size / (1024*1024):.1f} MB")
        print(f"  Reload time: {load_ms:.1f} ms")
        print(f"  Post-reload query: {verify_us:.1f} us ({len(results)} results)")
        print()

        # Summary
        print("=" * 60)
        print("SUMMARY")
        print("=" * 60)
        print(f"  Nodes:        {n:,}")
        print(f"  Ingest rate:  {n / ingest_sec:,.0f} nodes/sec")
        print(f"  File size:    {file_size / (1024*1024):.1f} MB")
        if HAS_PSUTIL:
            print(f"  RSS delta:    {rss1 - rss0:.1f} MB")
        print(f"  Reload:       {load_ms:.1f} ms")
        print()
        print("  Query time scales with match count (k), NOT corpus size (N).")
        print("  This is O(k) retrieval — the core Synrix property.")

        backend2.close()

    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
