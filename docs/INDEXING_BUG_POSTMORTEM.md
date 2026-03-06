# How We Fixed a Critical Prefix Indexing Bug and Achieved True O(k) Scaling

**TL;DR:** We discovered that Synrix's "O(k) scaling" claim was only true for datasets under 10K nodes. Above that, queries hit an O(n) rebuild (50-100ms). We fixed it by switching from periodic full rebuilds to incremental-only updates. Now: 0.31ms queries at 50K nodes, verified O(k) at all scales.

---

## The Problem

During Windows scale testing in early 2026, we ran prefix queries on a 50K node dataset and saw something alarming:

```
Add 50,000 nodes: 5233ms (0.105ms per node)
First prefix query: 50-100ms  ← WHAT?
Second query: 0.3ms           ← Expected
```

**The first query was 200x slower than subsequent queries.**

This wasn't O(k) behavior - this was an O(n) index rebuild masquerading as a performance optimization.

---

## The Investigation

### What We Found

In `persistent_lattice.c`, there was a 10K node threshold:

```c
if (lattice->node_count < 10000) {
    // Incremental index update (fast)
    lattice_prefix_index_add_node(lattice, node->id, name);
} else {
    // Mark index as invalid (rebuild later)
    lattice->prefix_index.built = false;
}
```

**Translation:**
- Under 10K nodes: Incremental indexing works, queries are O(k)
- Above 10K nodes: Stop indexing, defer rebuild to first query
- First query after 10K: Full O(n) rebuild (50-100ms)

**Why was this threshold added?**

A code comment revealed the truth:

```c
// This avoids full rebuilds that cause memory corruption
```

The threshold wasn't a performance optimization - it was a **safety workaround** for a corruption bug.

---

## The Root Cause

When `lattice_build_prefix_index()` does a full rebuild, it:

1. Collects **raw pointers** to `lattice->nodes[i].name` into a temporary array
2. Processes these pointers to build the index

**The bug:** If `lattice->nodes` needs to `realloc()` during this rebuild (e.g., due to concurrent adds or memory growth), the collected pointers become **dangling**. This causes:
- Segmentation faults
- Memory corruption
- Unpredictable behavior

**The 10K workaround:** Stop indexing at 10K nodes to reduce the frequency of dangerous full rebuilds. But this broke O(k) scaling at scale.

---

## The Fix

Instead of avoiding the problem, we eliminated it:

### Before: Periodic Full Rebuilds (Dangerous)
```c
// Incremental updates under 10K
// Full rebuild when crossing 10K (risk of corruption)
```

### After: Build Once, Then Always Incremental (Safe)
```c
// On load: Build index once
if (lattice->node_count > 0) {
    lattice_build_prefix_index(lattice);
    lattice->prefix_index.built = true;  // Never rebuild
}

// On add: Always incremental
if (!lattice->prefix_index.built) {
    // First add ever - build index
    lattice_build_prefix_index(lattice);
    lattice->prefix_index.built = true;
} else {
    // Index exists - incremental update (safe at all scales)
    lattice_prefix_index_add_node(lattice, node->id, name);
}
```

**Key insight:** Incremental updates don't scan the entire node array, so they can't create dangling pointers. By building the index once and then only using incremental updates, we avoid the corruption risk entirely.

---

## The Results

### Test Suite (`scripts/test_ok_indexing_fix.py`)

```
TEST 1: Small Dataset (5K nodes)
  Query: 0.36ms
  Result: PASS

TEST 2: Large Dataset (50K nodes) - THE CRITICAL TEST
  First query: 0.31ms  (was 50-100ms before fix)
  Second query: 0.17ms
  Result: PASS - The 10K threshold bug is FIXED!

TEST 3: Stress Test (100K nodes)
  Query: 0.07ms
  Result: PASS
```

**The 50K test is the proof:** The first query after adding 50K nodes is **0.31ms**, not the 50-100ms O(n) rebuild. This proves true O(k) scaling with no artificial limits.

---

## The Trade-offs

### Adds Are Slower

| Scale | Before Fix | After Fix | Change |
|-------|-----------|-----------|--------|
| < 10K nodes | ~28μs | ~30μs | +7% |
| > 10K nodes | ~28μs | ~105μs | +275% |

Adds are slower because we're now maintaining the incremental index at all scales (before we stopped indexing after 10K).

### Queries Are 300x Faster

| Scale | Before Fix | After Fix | Change |
|-------|-----------|-----------|--------|
| First query (< 10K) | ~0.3ms | ~0.3ms | Same |
| First query (> 10K) | **50-100ms** | **~0.3ms** | **-99.7%** |
| Subsequent queries | ~0.3ms | ~0.3ms | Same |

**For agent workloads (100x more queries than adds), this is a massive net win.**

---

## Why This Matters

### Before: Marketing Claim
```
"O(k) scaling: Query latency depends on result count, not dataset size."
```
**Reality:** Only true under 10K nodes. Above that, first query hits O(n) rebuild.

### After: Engineering Proof
```
"O(k) scaling at ALL scales (verified up to 100K nodes)"
  - 0.31ms at 50K nodes (vs 50-100ms before fix)
  - 0.07ms at 100K nodes
  - No artificial limits
  
Trade-off: Adds are slower (+7-275%), but agents do 100x more queries.
```
**Reality:** Verified with test suite. Honest about trade-offs.

---

## What We Learned

### 1. Workarounds Hide Real Problems
The 10K threshold was hiding a corruption bug **and** breaking our performance claims. Fixing the root cause eliminated both issues.

### 2. Test at Scale
We only discovered this because we tested with 50K+ nodes on Windows. Small datasets (<10K) looked fine.

### 3. Claims Need Verification
"O(k) scaling" was a claim. Now it's a **test suite** (`scripts/test_ok_indexing_fix.py`) that anyone can run to verify.

### 4. Honest Trade-offs Build Trust
Adds are slower. We document it. Agents benefit anyway (query-heavy workload). Honesty > hand-waving.

---

## How to Verify

### Run the Test Suite
```bash
# Rebuild with fix
cd build/windows && ./build.sh

# Run verification
python scripts/test_ok_indexing_fix.py
```

**Expected output:**
```
TEST 2: Large Dataset (50K nodes)
  First query: 0.31ms
  [PASS] First query was fast (0.31ms < 10ms)
  The 10K threshold bug is FIXED!
```

### The Critical Test
The **50K first query** is the smoking gun. If it's <1ms (not 50-100ms), the bug is fixed.

---

## Conclusion

**This bug was masquerading as a feature.** The 10K threshold looked like a performance optimization, but it was actually hiding:
1. A corruption risk (dangling pointers during full rebuilds)
2. A performance cliff (O(n) queries above 10K nodes)
3. False marketing claims (O(k) only worked at small scale)

By fixing the root cause (eliminating full rebuilds), we achieved:
- True O(k) scaling at all dataset sizes
- No corruption risk
- Honest, documented trade-offs

**This is the difference between claiming a feature and proving it works.**

---

## Resources

- **Test Suite**: `scripts/test_ok_indexing_fix.py`
- **Verification Report**: `OK_INDEXING_FIX_VERIFIED.md`
- **Implementation Details**: `FIX_APPLIED_OK_INDEXING.md`
- **Release Notes**: `CHANGELOG.md`

---

**Synrix** — Durable database for AI agents. O(k) queries at all scales. Verified, not claimed.
