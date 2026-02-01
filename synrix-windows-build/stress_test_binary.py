#!/usr/bin/env python3
"""
Stress test a SYNRIX DLL via a packaged distribution.

Usage:
  python stress_test_binary.py --package "C:\\path\\to\\package" --dll libsynrix_free_tier.dll --nodes 20000 --evaluation
  python stress_test_binary.py --package "C:\\path\\to\\package" --dll libsynrix.dll --nodes 20000 --no-evaluation
"""

import argparse
import os
import sys
import time
import random
from pathlib import Path


def format_number(value: int) -> str:
    return f"{value:,}"


def remove_if_exists(path: Path) -> None:
    try:
        if path.exists():
            path.unlink()
    except OSError:
        pass


def main() -> int:
    parser = argparse.ArgumentParser(description="Stress test a SYNRIX DLL binary")
    parser.add_argument("--package", required=True, help="Path to package directory")
    parser.add_argument("--dll", required=True, help="DLL filename in package/synrix/")
    parser.add_argument("--nodes", type=int, default=20000, help="Number of nodes to insert")
    parser.add_argument("--evaluation", action="store_true", help="Enable evaluation mode")
    parser.add_argument("--no-evaluation", dest="evaluation", action="store_false")
    parser.set_defaults(evaluation=True)
    args = parser.parse_args()

    package_dir = Path(args.package).resolve()
    if not package_dir.exists():
        print(f"[ERROR] Package directory not found: {package_dir}")
        return 1

    dll_path = package_dir / "synrix" / args.dll
    if not dll_path.exists():
        print(f"[ERROR] DLL not found: {dll_path}")
        return 1

    sys.path.insert(0, str(package_dir))
    os.environ["SYNRIX_LIB_PATH"] = str(dll_path)

    try:
        from synrix.raw_backend import RawSynrixBackend
    except Exception as exc:
        print(f"[ERROR] Failed to import synrix.raw_backend: {exc}")
        return 1

    lattice_path = package_dir / "stress_test.lattice"
    wal_path = package_dir / "stress_test.lattice.wal"
    remove_if_exists(lattice_path)
    remove_if_exists(wal_path)

    total_nodes = int(args.nodes)
    max_nodes = int(total_nodes * 1.2) + 100
    progress_interval = max(1000, total_nodes // 5)

    print("=" * 70)
    print("SYNRIX STRESS TEST")
    print("=" * 70)
    print(f"Package: {package_dir}")
    print(f"DLL: {dll_path.name}")
    print(f"Evaluation mode: {args.evaluation}")
    print(f"Target nodes: {format_number(total_nodes)}")
    print(f"Max nodes: {format_number(max_nodes)}")
    print()

    print("Phase 1: Insert nodes")
    sys.stdout.flush()
    sys.stderr.flush()
    start_time = time.time()
    try:
        backend = RawSynrixBackend(str(lattice_path), max_nodes=max_nodes, evaluation_mode=args.evaluation)
    except Exception as e:
        print(f"[ERROR] Failed to create backend: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1
    print(f"Loaded DLL: {getattr(backend.lib, '_name', None)}")
    sys.stdout.flush()

    node_ids = []
    for i in range(total_nodes):
        data = (f"Node {i}: stress test payload " * 5)[:400]
        node_id = backend.add_node(f"TEST:node_{i:08d}", data, 5)
        node_ids.append(node_id)

        if (i + 1) % progress_interval == 0:
            elapsed = time.time() - start_time
            rate = (i + 1) / elapsed if elapsed > 0 else 0
            remaining = total_nodes - (i + 1)
            eta = remaining / rate if rate > 0 else 0
            print(
                f"  Progress: {format_number(i + 1)}/{format_number(total_nodes)} "
                f"({rate:.0f} nodes/sec, ETA: {eta:.0f}s)"
            )

    insert_time = time.time() - start_time
    print(f"  Insert time: {insert_time:.2f}s")
    print(f"  Insert rate: {total_nodes / insert_time:.0f} nodes/sec")
    print()

    print("Phase 2: Random lookups")
    random.seed(42)
    sample_count = min(1000, total_nodes // 10)
    indices = [random.randint(0, total_nodes - 1) for _ in range(sample_count)]
    start_time = time.time()
    found = 0
    for idx in indices:
        node = backend.get_node(node_ids[idx])
        if node:
            found += 1
    query_time = time.time() - start_time
    print(f"  Queries: {format_number(sample_count)}")
    print(f"  Found: {found}/{sample_count}")
    print(f"  Query rate: {sample_count / query_time:.0f} qps")
    print()

    print("Phase 3: Prefix query")
    start_time = time.time()
    results = backend.find_by_prefix("TEST:node_", limit=min(2000, total_nodes))
    prefix_time = time.time() - start_time
    print(f"  Results: {format_number(len(results))}")
    print(f"  Prefix query time: {prefix_time * 1000:.2f} ms")
    print()

    print("Phase 4: Save and reload")
    backend.flush()
    backend.checkpoint()
    backend.save()
    backend.close()

    backend2 = RawSynrixBackend(str(lattice_path), max_nodes=max_nodes, evaluation_mode=args.evaluation)
    reloaded = backend2.find_by_prefix("TEST:node_", limit=total_nodes + 10)
    backend2.close()
    print(f"  Reloaded nodes: {format_number(len(reloaded))}")
    print()

    remove_if_exists(lattice_path)
    remove_if_exists(wal_path)

    if len(reloaded) != total_nodes:
        print(f"[ERROR] Reload count mismatch: expected {total_nodes}, got {len(reloaded)}")
        return 1

    print("[OK] Stress test complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
