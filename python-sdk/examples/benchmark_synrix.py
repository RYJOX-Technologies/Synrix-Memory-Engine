#!/usr/bin/env python3
"""
Synrix vs. Competitors — Reproducible Benchmark
=================================================
Measures storage-layer performance head-to-head:
  - Synrix (raw_backend, direct .so)
  - ChromaDB (local persistent)
  - Qdrant (local Docker — optional)

Run:
  python3 benchmark_synrix.py           # quick (1K nodes, Synrix + Chroma)
  python3 benchmark_synrix.py --full    # full (100K nodes, all backends)

Requirements:
  pip install synrix chromadb            # minimum
  pip install qdrant-client              # optional, needs Docker qdrant running
"""

import sys, os, time, json, platform, argparse, statistics, tempfile, shutil
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

try:
    import psutil
    HAS_PSUTIL = True
except ImportError:
    HAS_PSUTIL = False

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

PREFIXES = [
    "ISA_", "PATTERN_", "LEARNING_", "FUNC_", "TASK_",
    "DOMAIN_", "INTERFACE_", "OBJECTIVE_", "COMPONENT_", "SYSTEM_",
    "DOC_PYTHON_", "DOC_RUST_", "DOC_KUBERNETES_", "DOC_DATABASE_",
    "DOC_NETWORKING_", "DOC_SECURITY_", "DOC_ML_", "DOC_FRONTEND_",
    "DOC_BACKEND_", "DOC_DEVOPS_",
]

def make_payload(i):
    prefix = PREFIXES[i % len(PREFIXES)]
    name = f"{prefix}{i:08d}"
    data = (f"Node {i}: knowledge about {prefix.rstrip('_').lower()} topic {i}. "
            f"Contains structured information for retrieval benchmarking. ") * 3
    return name, data[:500]

def rss_mb():
    if not HAS_PSUTIL:
        return 0.0
    return psutil.Process(os.getpid()).memory_info().rss / (1024 * 1024)

def percentiles(times):
    s = sorted(times)
    n = len(s)
    return {
        "p50_us": s[n // 2] * 1e6,
        "p95_us": s[int(n * 0.95)] * 1e6 if n > 1 else s[-1] * 1e6,
        "p99_us": s[int(n * 0.99)] * 1e6 if n > 1 else s[-1] * 1e6,
        "mean_us": statistics.mean(times) * 1e6,
        "min_us": min(times) * 1e6,
        "max_us": max(times) * 1e6,
    }

def hw_info():
    info = {
        "platform": platform.platform(),
        "machine": platform.machine(),
        "python": platform.python_version(),
    }
    if HAS_PSUTIL:
        info["cpu_count"] = psutil.cpu_count(logical=True)
        mem = psutil.virtual_memory()
        info["ram_gb"] = round(mem.total / (1024**3), 1)
    return info


# ── Synrix Backend ────────────────────────────────────────────────────────────

class SynrixBench:
    name = "Synrix (raw_backend)"

    def __init__(self, n):
        from synrix.raw_backend import RawSynrixBackend
        self.path = os.path.join(tempfile.mkdtemp(), "bench.lattice")
        self.backend = RawSynrixBackend(self.path, max_nodes=max(n + 1000, 26000))

    def write_one(self, name, data):
        self.backend.add_node(name, data)

    def read_prefix(self, prefix, limit=1000):
        return self.backend.find_by_prefix(prefix, limit=limit)

    def close(self):
        self.backend.close()
        shutil.rmtree(os.path.dirname(self.path), ignore_errors=True)


# ── ChromaDB Backend ─────────────────────────────────────────────────────────

class ChromaBench:
    name = "ChromaDB (local)"

    def __init__(self, n):
        try:
            import chromadb
        except ImportError:
            raise ImportError("pip install chromadb")
        self.tmpdir = tempfile.mkdtemp()
        self.client = chromadb.PersistentClient(path=self.tmpdir)
        self.col = self.client.get_or_create_collection(
            "bench", metadata={"hnsw:space": "cosine"}
        )

    def write_one(self, name, data):
        self.col.add(ids=[name], documents=[data])

    def read_prefix(self, prefix, limit=1000):
        return self.col.get(where={"$or": []}, limit=limit) if False else \
               self.col.get(ids=[], limit=limit)

    def read_by_where(self, prefix, limit=1000):
        """ChromaDB doesn't support prefix queries natively; use get with ids."""
        try:
            res = self.col.query(query_texts=[prefix], n_results=min(limit, self.col.count()))
            return res.get("documents", [[]])[0]
        except Exception:
            return []

    def close(self):
        del self.client
        shutil.rmtree(self.tmpdir, ignore_errors=True)


# ── Qdrant Backend ───────────────────────────────────────────────────────────

class QdrantBench:
    name = "Qdrant (local Docker)"

    def __init__(self, n):
        try:
            from qdrant_client import QdrantClient
            from qdrant_client.models import Distance, VectorParams, PointStruct
        except ImportError:
            raise ImportError("pip install qdrant-client")
        import socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if sock.connect_ex(("localhost", 6333)) != 0:
            sock.close()
            raise ConnectionError("Qdrant not running on localhost:6333 (start with: docker run -p 6333:6333 qdrant/qdrant)")
        sock.close()
        self.client = QdrantClient(url="http://localhost:6333", timeout=60)
        try:
            self.client.delete_collection("bench")
        except Exception:
            pass
        self.client.create_collection(
            "bench", vectors_config=VectorParams(size=384, distance=Distance.COSINE)
        )
        self.PointStruct = PointStruct
        self._id = 0

    def write_one(self, name, data):
        import random
        vec = [random.gauss(0, 1) for _ in range(384)]
        self._id += 1
        self.client.upsert("bench", [self.PointStruct(
            id=self._id, vector=vec, payload={"name": name, "data": data}
        )])

    def read_prefix(self, prefix, limit=1000):
        from qdrant_client.models import Filter, FieldCondition, MatchText
        return self.client.scroll(
            "bench", scroll_filter=Filter(must=[
                FieldCondition(key="name", match=MatchText(text=prefix))
            ]), limit=limit
        )

    def close(self):
        try:
            self.client.delete_collection("bench")
        except Exception:
            pass


# ── Benchmark Runner ─────────────────────────────────────────────────────────

def run_benchmark(backend_cls, n_nodes, n_read_queries=200):
    results = {"backend": backend_cls.name, "n_nodes": n_nodes}

    # Init
    t0 = time.perf_counter()
    rss0 = rss_mb()
    try:
        backend = backend_cls(n_nodes)
    except (ImportError, ConnectionError) as e:
        print(f"  SKIP {backend_cls.name}: {e}")
        return None
    results["cold_start_ms"] = (time.perf_counter() - t0) * 1000

    # Write
    write_times = []
    for i in range(n_nodes):
        name, data = make_payload(i)
        t = time.perf_counter()
        backend.write_one(name, data)
        write_times.append(time.perf_counter() - t)
        if (i + 1) % max(1, n_nodes // 5) == 0:
            print(f"    write {i+1}/{n_nodes}")
    results["write"] = percentiles(write_times)
    results["write_ops_per_sec"] = n_nodes / sum(write_times)

    rss1 = rss_mb()
    results["rss_delta_mb"] = round(rss1 - rss0, 1)

    # Read (prefix queries — only meaningful for Synrix)
    if hasattr(backend, "read_prefix") and backend_cls != ChromaBench:
        read_times = []
        for i in range(n_read_queries):
            prefix = PREFIXES[i % len(PREFIXES)]
            t = time.perf_counter()
            backend.read_prefix(prefix, limit=100)
            read_times.append(time.perf_counter() - t)
        results["read_prefix"] = percentiles(read_times)

    # Read (similarity/doc query — for vector DBs)
    if hasattr(backend, "read_by_where"):
        read_times = []
        for i in range(min(n_read_queries, 50)):
            prefix = PREFIXES[i % len(PREFIXES)]
            t = time.perf_counter()
            backend.read_by_where(prefix.rstrip("_").lower(), limit=100)
            read_times.append(time.perf_counter() - t)
        results["read_query"] = percentiles(read_times)

    backend.close()
    return results


def fmt_us(v):
    if v < 1000:
        return f"{v:.1f} us"
    return f"{v / 1000:.2f} ms"


def print_comparison(all_results, n_nodes):
    print()
    print("=" * 78)
    print(f"  BENCHMARK RESULTS — {n_nodes:,} nodes")
    print("=" * 78)

    hw = hw_info()
    print(f"  {hw.get('platform', '?')}  |  {hw.get('machine', '?')}  |  "
          f"CPUs: {hw.get('cpu_count', '?')}  |  RAM: {hw.get('ram_gb', '?')} GB")
    print()

    valid = [r for r in all_results if r is not None]
    if not valid:
        print("  No backends completed.")
        return

    # Header
    names = [r["backend"] for r in valid]
    col_w = max(22, max(len(n) for n in names) + 2)
    hdr = f"{'Metric':<28}" + "".join(f"{n:>{col_w}}" for n in names)
    print(hdr)
    print("-" * len(hdr))

    def row(label, key, sub=None, formatter=fmt_us):
        vals = []
        for r in valid:
            v = r.get(key)
            if v is None:
                vals.append("—")
            elif sub:
                vals.append(formatter(v.get(sub, 0)))
            else:
                vals.append(formatter(v))
        line = f"{label:<28}" + "".join(f"{v:>{col_w}}" for v in vals)
        print(line)

    row("Cold start", "cold_start_ms", formatter=lambda v: f"{v:.1f} ms")
    row("Write p50", "write", "p50_us")
    row("Write p99", "write", "p99_us")
    row("Write ops/sec", "write_ops_per_sec", formatter=lambda v: f"{v:,.0f}")
    row("Read prefix p50", "read_prefix", "p50_us")
    row("Read prefix p99", "read_prefix", "p99_us")
    row("Read query p50", "read_query", "p50_us")
    row("Read query p99", "read_query", "p99_us")
    row("RSS delta", "rss_delta_mb", formatter=lambda v: f"{v:.1f} MB")

    print()
    print("Notes:")
    print("  - Synrix: prefix-semantic retrieval (exact match, no embeddings)")
    print("  - ChromaDB/Qdrant: embedding-based similarity (requires model)")
    print("  - Qdrant write includes random 384-dim vector generation")
    print("  - Times are per-operation. us = microseconds, ms = milliseconds.")
    print()

    # Write JSON
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "benchmark_results")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "latest.json")
    with open(out_path, "w") as f:
        json.dump({"hw": hw, "n_nodes": n_nodes, "results": valid}, f, indent=2, default=str)
    print(f"  Results saved to {out_path}")


def main():
    parser = argparse.ArgumentParser(description="Synrix vs. competitors benchmark")
    parser.add_argument("--full", action="store_true", help="Full benchmark (100K nodes, all backends)")
    parser.add_argument("--nodes", type=int, default=None, help="Custom node count")
    parser.add_argument("--skip-qdrant", action="store_true", help="Skip Qdrant backend")
    parser.add_argument("--skip-chroma", action="store_true", help="Skip ChromaDB backend")
    args = parser.parse_args()

    if args.nodes:
        n = args.nodes
    elif args.full:
        n = 100_000
    else:
        n = 1_000

    print(f"Synrix Benchmark — {n:,} nodes")
    print(f"Mode: {'full' if args.full else 'quick'}")
    print()

    backends = [SynrixBench]
    if not args.skip_chroma:
        backends.append(ChromaBench)
    if not args.skip_qdrant:
        backends.append(QdrantBench)

    all_results = []
    for cls in backends:
        print(f"  Running: {cls.name} ...")
        result = run_benchmark(cls, n)
        all_results.append(result)

    print_comparison(all_results, n)


if __name__ == "__main__":
    main()
