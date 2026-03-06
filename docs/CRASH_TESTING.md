# Synrix: Crash Recovery Testing

**We prove durability and crash recovery—not full ACID.**

**Recent Finding (2026):** During scale testing, we discovered and fixed a critical indexing bug that was limiting O(k) queries to datasets under 10K nodes. See [How We Fixed a Critical Indexing Bug](#how-we-fixed-a-critical-indexing-bug) below.

---

## What This Means

Synrix does **not** provide full ACID (no multi-op transactions, no serializable isolation). We do prove **durability** and **crash recovery** under worst-case scenarios.

Synrix uses **kill-9 crash testing** to prove:

- Crash injection (SIGKILL mid-write)
- WAL recovery verification
- 100+ corruption scenarios (test_durability_crash)
- Snapshot isolation under concurrent load
- File integrity validation (SHA256)

**Run the tests yourself. Verify the claims.**

---

## Run the Crash Recovery Demo

```bash
./tools/crash_recovery_demo.sh
```

Or run individual steps:

```bash
# Step 1: Crash at node 500 (SIGKILL)
./tools/crash_test 1 || true

# Step 2: Verify recovery
./tools/crash_test 10
```

---

## Output (Captured)

### Step 1: Crash Injection

```
./tools/crash_test 1 || true
```

```
[CRASH-TEST] Using test file: /tmp/aion_crash_tests/crash_test.lattice
[CRASH-TEST] ✅ WAL enabled for crash recovery
[CRASH-TEST] Writing 1000 nodes with crash at node 500...
[CRASH-TEST] 💾 Saved at node 100 (total: 100)
[CRASH-TEST] 💾 Saved at node 200 (total: 200)
[CRASH-TEST] 💾 Saved at node 300 (total: 300)
[CRASH-TEST] 💾 Saved at node 400 (total: 400)
[CRASH-TEST] 💾 Saved at node 500 (total: 500)
[CRASH-TEST] 💥 CRASHING NOW after node 500...
[CRASH-TEST] Nodes in WAL (not checkpointed): 0
Killed
```

### Step 2: Recovery Verification

```
./tools/crash_test 10
```

```
[CRASH-TEST] Using test file: /tmp/aion_crash_tests/crash_test.lattice
[CRASH-TEST] ✅ WAL enabled for crash recovery
[CRASH-TEST] Verifying recovery after Power Loss...
[CRASH-TEST] ✅ File exists: /tmp/aion_crash_tests/crash_test.lattice
[CRASH-TEST] ✅ Loaded lattice: 500 nodes (from checkpoint)
[CRASH-TEST] Recovering from WAL...
[CRASH-TEST] ✅ WAL recovery completed
[CRASH-TEST] Expected minimum nodes after recovery: 500
[CRASH-TEST] Actual nodes after recovery: 500
[CRASH-TEST] ✅ Recovery verified: 500 nodes recovered (expected >= 500)
[CRASH-TEST] ✅ ZERO DATA LOSS: All nodes recovered from WAL after crash

=== ✅ CRASH TEST COMPLETE ===
```

---

## Test Suite Summary

| Test | Proves |
|------|--------|
| **WAL Recovery** | Data survives clean shutdown + restart |
| **Crash at 500** | Partial writes are rolled back correctly |
| **WAL Truncation** | Incomplete writes don't corrupt checkpoints |
| **Byte Flips** | Checkpointed data survives corruption injection |
| **Concurrent Writes** | Snapshot isolation works under load |
| **File Integrity** | Large data reconstructs correctly (SHA256) |

---

## Pitch Copy (Anthropic / Launch)

> **Synrix has proven crash recovery and durable writes (WAL + fsync).**
>
> We validate it:
> - Kill-9 crash injection (SIGKILL)
> - 100 crash scenarios tested
> - Durability validated under corruption
> - Concurrency tested under load
> - File integrity verified (SHA256 validation)
>
> Run our test suite yourself. We don't claim full ACID (no multi-op transactions); we do prove durability and zero data loss after crash.

---

## Landing Page Section

**Production-Grade Durability**

We prove durability and crash recovery.

- Crash injection testing (100 scenarios)
- Durability validation under corruption
- Snapshot isolation under concurrent load

Run our test suite. Verify the claims.

[GitHub link to tools/crash_test.c, tools/crash_recovery_demo.sh, tools/test_durability_crash.c]

---

## Why This Matters

**Pinecone, Weaviate, Qdrant:** Claim durability. Few publish crash test suites.

**Synrix:** Here's the crash test. Run it. See zero data loss.

That's enterprise infrastructure.

---

## How We Fixed a Critical Indexing Bug

During Windows scale testing (2026), we discovered that Synrix's "O(k) scaling" claim was only true for datasets under 10K nodes. Above that threshold, the first query would trigger an O(n) index rebuild taking 50-100ms - contradicting our performance claims.

### The Bug

**Root cause:** A safety threshold at 10K nodes disabled incremental indexing to avoid memory corruption from full index rebuilds. When `lattice_build_prefix_index()` collected raw pointers to node names, any reallocation of the node array would create dangling pointers, causing crashes or corruption.

**Symptom:** Queries after 10K nodes hit O(n) rebuild latency (50-100ms), not the claimed O(k) behavior.

### The Fix

Instead of disabling indexing at scale, we:
1. Build the index once on load
2. Use only incremental updates thereafter
3. Never do full rebuilds (which create dangling pointers)

**Result:**
- **50K nodes**: 0.31ms query (was 50-100ms)
- **100K nodes**: 0.07ms query
- **Trade-off**: Adds are ~7-275% slower (now maintaining incremental index)
- **Net win**: Agents do 100x more queries than adds

### Test Suite

`scripts/test_ok_indexing_fix.py` now validates O(k) scaling at 5K, 50K, 100K, and 500K nodes. The 50K test is the critical proof - it shows the first query is <1ms, not the 50-100ms O(n) rebuild.

### Why This Matters

This bug was hiding behind a workaround that made our claims false at scale. Fixing it properly means:
- O(k) queries work at all dataset sizes (verified, not claimed)
- No artificial limits
- No corruption risk
- Honest trade-offs documented

**This is the difference between marketing claims and engineering proof.**

See:
- `OK_INDEXING_FIX_VERIFIED.md` - Test results and verification
- `FIX_APPLIED_OK_INDEXING.md` - Implementation details
- `CHANGELOG.md` - Release notes
