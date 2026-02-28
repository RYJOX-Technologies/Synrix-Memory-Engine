#!/usr/bin/env python3
"""
Reasoning Chain Benchmark
=========================
An agent solving a multi-step problem queries memory N times during reasoning.
We run it three ways:

  A) Synrix  -- real sub-millisecond memory
  B) Artificial 50ms latency per query  -- simulates cloud/vector DB round-trip
  C) Artificial 200ms latency per query -- simulates cold Mem0/embedding pipeline

Shows: wall-clock time, queries/sec, and what "250x faster" actually means
       when an agent is doing 50+ memory lookups per reasoning chain.

Run:
  python demos/reasoning_chain_benchmark.py
"""

import sys, os, time, tempfile, shutil, statistics

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "python-sdk"))

from synrix.raw_backend import RawSynrixBackend


# ── Knowledge base: things an agent would store and retrieve ─────────────────

KNOWLEDGE = {
    "BUG_NULL_POINTER": [
        ("BUG_NULL_POINTER_1", "NullPointerException in UserService.getProfile() -- root cause: lazy-loaded relation accessed outside transaction scope."),
        ("BUG_NULL_POINTER_2", "NPE in PaymentProcessor.charge() -- Optional.get() called without isPresent() check on nullable card field."),
        ("BUG_NULL_POINTER_3", "Null dereference in CacheManager.evict() -- concurrent modification: entry removed between containsKey and get."),
    ],
    "BUG_RACE_CONDITION": [
        ("BUG_RACE_CONDITION_1", "Race condition in OrderService: two threads read inventory count, both decrement, oversold by 1. Fix: optimistic locking with version column."),
        ("BUG_RACE_CONDITION_2", "Race in SessionManager: session created in thread A, accessed in thread B before write completes. Fix: ConcurrentHashMap + putIfAbsent."),
    ],
    "BUG_MEMORY_LEAK": [
        ("BUG_MEMORY_LEAK_1", "Memory leak in EventBus: listeners registered in onCreate never unregistered. Fix: weak references or explicit removeListener in onDestroy."),
        ("BUG_MEMORY_LEAK_2", "Leak in ConnectionPool: connections borrowed but not returned on exception path. Fix: try-with-resources for all borrows."),
    ],
    "FIX_FAILED": [
        ("FIX_FAILED_1", "Attempted fix: adding synchronized to getProfile(). Failed: caused deadlock with transaction manager. Reverted."),
        ("FIX_FAILED_2", "Attempted fix: global lock on inventory. Failed: throughput dropped 90%. Replaced with row-level optimistic lock."),
        ("FIX_FAILED_3", "Attempted fix: increasing pool size for leak. Failed: just delayed OOM. Root cause was unreturned connections."),
    ],
    "PATTERN_RETRY": [
        ("PATTERN_RETRY_1", "Retry pattern: exponential backoff with jitter. Max 3 retries. Circuit breaker after 5 consecutive failures in 60s window."),
        ("PATTERN_RETRY_2", "Retry with idempotency key: generate UUID before first attempt, pass on all retries. Server deduplicates by key."),
    ],
    "PATTERN_CACHE": [
        ("PATTERN_CACHE_1", "Cache-aside pattern: check cache -> miss -> load from DB -> populate cache. TTL 5 min. Invalidate on write."),
        ("PATTERN_CACHE_2", "Write-through cache: write to cache and DB atomically. Slower writes, but cache is always consistent. Use for critical data."),
    ],
    "PATTERN_CIRCUIT_BREAKER": [
        ("PATTERN_CIRCUIT_BREAKER_1", "Circuit breaker states: CLOSED (normal) -> OPEN (failing, reject calls) -> HALF_OPEN (test one call). Threshold: 5 failures in 30s."),
    ],
    "CODEBASE_AUTH": [
        ("CODEBASE_AUTH_1", "AuthService uses JWT with 15min access + 7d refresh tokens. Middleware validates on every request. Token refresh is atomic."),
        ("CODEBASE_AUTH_2", "RBAC: roles stored in user table, permissions in role_permissions join. Checked via @RequiresPermission annotation."),
    ],
    "CODEBASE_DB": [
        ("CODEBASE_DB_1", "PostgreSQL 15. Connection pool: HikariCP, max 20 connections. Read replicas for analytics queries. Flyway migrations."),
        ("CODEBASE_DB_2", "Critical tables: users (10M rows, indexed on email), orders (50M rows, partitioned by month), inventory (100K rows, optimistic locking)."),
    ],
    "CODEBASE_DEPLOY": [
        ("CODEBASE_DEPLOY_1", "CI/CD: GitHub Actions -> Docker build -> push to ECR -> ECS Fargate deploy. Blue/green deployment. Rollback: revert to previous task definition."),
    ],
    "RECENT_INCIDENT": [
        ("RECENT_INCIDENT_1", "2024-01-15: Production OOM. Root cause: unbounded in-memory queue in event processor. Fix: bounded queue with backpressure. Deployed same day."),
        ("RECENT_INCIDENT_2", "2024-01-20: 502 errors on /api/orders. Root cause: DB connection exhaustion from leaked connections in error path. Fix: connection pool monitoring + try-with-resources audit."),
    ],
}

TOTAL_FACTS = sum(len(v) for v in KNOWLEDGE.values())


# ── Simulated reasoning chain ────────────────────────────────────────────────

REASONING_QUERIES = [
    # Step 1: Understand the bug
    ("BUG_NULL_POINTER", "Check memory for similar null pointer bugs"),
    ("BUG_RACE_CONDITION", "Check for race conditions that look similar"),
    ("BUG_MEMORY_LEAK", "Check for memory leak patterns"),
    # Step 2: Check what already failed
    ("FIX_FAILED", "What fixes have been tried and failed before?"),
    # Step 3: Look for applicable patterns
    ("PATTERN_RETRY", "Retrieve retry patterns from knowledge base"),
    ("PATTERN_CACHE", "Retrieve caching patterns"),
    ("PATTERN_CIRCUIT_BREAKER", "Retrieve circuit breaker patterns"),
    # Step 4: Understand the codebase context
    ("CODEBASE_AUTH", "How does auth work in this codebase?"),
    ("CODEBASE_DB", "What's the database setup?"),
    ("CODEBASE_DEPLOY", "How is deployment configured?"),
    # Step 5: Check recent incidents
    ("RECENT_INCIDENT", "Any recent production incidents related to this?"),
    # Step 6: Cross-reference (second pass -- agent adapts based on earlier findings)
    ("BUG_RACE_CONDITION", "Re-check race conditions now that I know about the connection pool"),
    ("FIX_FAILED", "Re-check failed fixes -- am I about to repeat one?"),
    ("PATTERN_RETRY", "Does the retry pattern apply to the connection issue?"),
    ("CODEBASE_DB", "Re-check DB config for connection pool details"),
    # Step 7: Final verification
    ("RECENT_INCIDENT", "Was the recent OOM related to this same pattern?"),
    ("BUG_MEMORY_LEAK", "Could this be a leak disguised as a race condition?"),
    ("PATTERN_CACHE", "Should I add caching to reduce DB pressure?"),
]


class MemoryWithLatency:
    """Wraps a Synrix backend and adds artificial latency per query."""

    def __init__(self, backend, artificial_latency_ms=0):
        self.backend = backend
        self.latency_s = artificial_latency_ms / 1000.0
        self.query_times = []

    def query(self, prefix, limit=50):
        t0 = time.perf_counter()
        results = self.backend.find_by_prefix(prefix, limit=limit)
        if self.latency_s > 0:
            time.sleep(self.latency_s)
        elapsed = time.perf_counter() - t0
        self.query_times.append(elapsed)
        return results

    def stats(self):
        if not self.query_times:
            return {}
        total = sum(self.query_times)
        return {
            "total_ms": total * 1000,
            "count": len(self.query_times),
            "avg_ms": (total / len(self.query_times)) * 1000,
            "p50_ms": sorted(self.query_times)[len(self.query_times) // 2] * 1000,
            "p99_ms": sorted(self.query_times)[int(len(self.query_times) * 0.99)] * 1000,
            "qps": len(self.query_times) / total if total > 0 else 0,
        }


def run_reasoning_chain(memory, label):
    """Simulate an agent reasoning through a bug fix using memory queries."""
    print(f"\n{'=' * 70}")
    print(f"  {label}")
    print(f"{'=' * 70}")

    total_results = 0
    steps = [
        (0, 3, "Understand the bug"),
        (3, 4, "Check failed approaches"),
        (4, 7, "Find applicable patterns"),
        (7, 10, "Understand codebase context"),
        (10, 11, "Check recent incidents"),
        (11, 15, "Cross-reference (adaptive re-query)"),
        (15, 18, "Final verification"),
    ]

    chain_start = time.perf_counter()

    for step_start, step_end, step_name in steps:
        step_t0 = time.perf_counter()
        step_results = 0
        for i in range(step_start, min(step_end, len(REASONING_QUERIES))):
            prefix, description = REASONING_QUERIES[i]
            results = memory.query(prefix)
            step_results += len(results)
            total_results += len(results)
        step_ms = (time.perf_counter() - step_t0) * 1000
        n_queries = min(step_end, len(REASONING_QUERIES)) - step_start
        print(f"  Step: {step_name:<38} {n_queries} queries  {step_ms:>8.2f} ms  ({step_results} facts)")

    chain_ms = (time.perf_counter() - chain_start) * 1000
    stats = memory.stats()

    print(f"\n  {'-' * 66}")
    print(f"  Total queries:     {stats['count']}")
    print(f"  Total facts used:  {total_results}")
    print(f"  Total memory time: {stats['total_ms']:.2f} ms")
    print(f"  Avg per query:     {stats['avg_ms']:.3f} ms")
    print(f"  p50 per query:     {stats['p50_ms']:.3f} ms")
    print(f"  Queries/sec:       {stats['qps']:,.0f}")
    print(f"  Chain wall-clock:  {chain_ms:.2f} ms")

    return stats


def main():
    print("Synrix -- Reasoning Chain Benchmark")
    print("=" * 70)
    print(f"Scenario: Agent debugging a production issue.")
    print(f"          {len(REASONING_QUERIES)} memory queries across 7 reasoning steps.")
    print(f"          {TOTAL_FACTS} facts in knowledge base.")
    print(f"          Includes adaptive re-queries (agent refines based on findings).")
    print()

    tmpdir = tempfile.mkdtemp()
    lattice_path = os.path.join(tmpdir, "reasoning_bench.lattice")

    try:
        backend = RawSynrixBackend(lattice_path, max_nodes=26000)

        # Populate knowledge base
        print(f"Populating knowledge base ({TOTAL_FACTS} facts) ...", end=" ")
        t0 = time.perf_counter()
        for prefix, facts in KNOWLEDGE.items():
            for name, data in facts:
                backend.add_node(name, data)
        print(f"done ({(time.perf_counter() - t0) * 1000:.1f} ms)")

        # Warm up (one throwaway query so first real query isn't cold)
        backend.find_by_prefix("BUG_", limit=1)

        # ── Version A: Synrix (real speed) ───────────────────────────────
        mem_a = MemoryWithLatency(backend, artificial_latency_ms=0)
        stats_a = run_reasoning_chain(mem_a, "Version A: Synrix (sub-millisecond memory)")

        # ── Version B: 50ms artificial latency (typical vector DB) ───────
        mem_b = MemoryWithLatency(backend, artificial_latency_ms=50)
        stats_b = run_reasoning_chain(mem_b, "Version B: 50ms latency (vector DB / cloud round-trip)")

        # ── Version C: 200ms artificial latency (Mem0 / embedding pipeline)
        mem_c = MemoryWithLatency(backend, artificial_latency_ms=200)
        stats_c = run_reasoning_chain(mem_c, "Version C: 200ms latency (Mem0 / embedding + retrieval)")

        # ── Comparison table ─────────────────────────────────────────────
        print(f"\n{'=' * 70}")
        print(f"  COMPARISON")
        print(f"{'=' * 70}")
        print()
        print(f"  {'Metric':<28} {'Synrix':>14} {'50ms (VDB)':>14} {'200ms (Mem0)':>14}")
        print(f"  {'-' * 70}")
        print(f"  {'Total memory time':<28} {stats_a['total_ms']:>11.2f} ms {stats_b['total_ms']:>11.2f} ms {stats_c['total_ms']:>11.2f} ms")
        print(f"  {'Avg per query':<28} {stats_a['avg_ms']:>11.3f} ms {stats_b['avg_ms']:>11.3f} ms {stats_c['avg_ms']:>11.3f} ms")
        print(f"  {'Queries/sec':<28} {stats_a['qps']:>11,.0f}    {stats_b['qps']:>11,.0f}    {stats_c['qps']:>11,.0f}   ")
        print()

        speedup_b = stats_b['total_ms'] / max(stats_a['total_ms'], 0.001)
        speedup_c = stats_c['total_ms'] / max(stats_a['total_ms'], 0.001)

        print(f"  Synrix vs vector DB:   {speedup_b:,.0f}x faster memory retrieval")
        print(f"  Synrix vs Mem0:        {speedup_c:,.0f}x faster memory retrieval")
        print()

        # ── The punchline ────────────────────────────────────────────────
        print(f"  {'-' * 70}")
        print()
        print(f"  What this means for agents:")
        print()
        print(f"  With Synrix: {len(REASONING_QUERIES)} queries cost {stats_a['total_ms']:.1f} ms total.")
        print(f"    -> Agent can do {int(stats_a['qps'])} queries/sec.")
        print(f"    -> Memory is invisible -- reasoning speed is the bottleneck, not retrieval.")
        print(f"    -> Agent can re-query mid-chain (adaptive reasoning) at zero cost.")
        print()
        print(f"  With 50ms latency: same {len(REASONING_QUERIES)} queries cost {stats_b['total_ms']:.0f} ms.")
        print(f"    -> Every re-query costs 50ms. Agent designers minimize queries.")
        print(f"    -> Adaptive reasoning becomes expensive. You pre-fetch instead.")
        print(f"    -> \"How many queries can I afford?\" becomes a design constraint.")
        print()
        print(f"  With 200ms latency: same {len(REASONING_QUERIES)} queries cost {stats_c['total_ms']:.0f} ms.")
        print(f"    -> Each query costs more than most LLM tokens.")
        print(f"    -> Agents batch everything upfront. No mid-chain adaptation.")
        print(f"    -> Memory becomes a bottleneck, not a tool.")
        print()
        print(f"  Sub-millisecond memory doesn't just make agents faster.")
        print(f"  It makes them smarter -- they can query freely, adapt mid-thought,")
        print(f"  and cross-reference without budgeting retrieval time.")

        backend.close()

    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
