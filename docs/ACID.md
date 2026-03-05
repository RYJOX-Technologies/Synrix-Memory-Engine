# Synrix: Durability and Crash Recovery

## What We Prove

We validate **durability** and **crash recovery** under worst-case scenarios. We do not claim full ACID (no multi-operation transactions, no serializable isolation).

Synrix uses **Jepsen-style crash injection testing** to prove:

- **Durability:** WAL + fsync; checkpointed data survives power loss.
- **Single-operation atomicity:** Each add is all-or-nothing; incomplete writes are rolled back on recovery.
- **Crash recovery:** SIGKILL mid-write → replay WAL → zero data loss for checkpointed operations.
- **Consistency:** Checkpoint + replay; no partial state on disk.

We do **not** provide:

- Multi-operation transactions (no `BEGIN; add(); add(); COMMIT;`).
- Serializable isolation (readers can see intermediate states; seqlock gives lock-free reads, not full isolation levels).
- Rollback of multi-step operations.

For agent workloads (store/retrieve patterns, single-op writes), this tradeoff is intentional: we prioritize speed and simplicity.

## What We Guarantee

| Property | Implementation |
|----------|-----------------|
| **Durability** | WAL fsync. Checkpointed data survives power loss. |
| **Single-op atomicity** | Each write is all-or-nothing; recovery rolls back incomplete writes. |
| **Consistency** | Checkpoint + replay. No partial state on disk. |
| **Isolation** | Lock-free reads (seqlock). Not serializable; readers may see intermediate states. |

## Crash Recovery

1. **Write**: Node added → WAL entry appended
2. **Checkpoint** (every N nodes): WAL → main file, truncate WAL
3. **Crash**: Process killed (SIGKILL)
4. **Recovery**: Load main file + replay WAL from last checkpoint

**Result**: Zero data loss for checkpointed operations.

## Run the Proof

```bash
./tools/crash_recovery_demo.sh
```

Output:
```
[CRASH-TEST] 💥 CRASHING NOW after node 500...
...
[CRASH-TEST] ✅ ZERO DATA LOSS: All nodes recovered from WAL after crash
```

## Test Suite

| Test | Proves |
|------|--------|
| **WAL Recovery** | Data survives clean shutdown + restart |
| **Crash at 500** | Partial writes rolled back correctly |
| **WAL Truncation** | Incomplete writes don't corrupt checkpoints |
| **Byte Flips** | Checkpointed data survives corruption injection |
| **Concurrent Writes** | Behavior under load (not full transaction isolation) |

## Further Reading

- [Jepsen-Style Crash Testing](JEPSEN_STYLE_CRASH_TESTING.md)
- [Tools README](../tools/README.md)
