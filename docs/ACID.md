# Synrix ACID Guarantees

## What We Prove

We don't just claim ACID. We validate it under worst-case scenarios.

Synrix uses **Jepsen-style crash injection testing** to prove durability:

- Crash injection (SIGKILL mid-write)
- WAL recovery verification
- 100+ corruption scenarios
- Snapshot isolation under concurrent load

## ACID Properties

| Property | Implementation |
|----------|-----------------|
| **Atomicity** | WAL: all-or-nothing. Incomplete writes are rolled back on recovery. |
| **Consistency** | Checkpoint + replay. No partial state. |
| **Isolation** | Snapshot isolation via seqlocks. Readers see consistent view. |
| **Durability** | WAL fsync. Checkpointed data survives power loss. |

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
| **Concurrent Writes** | Snapshot isolation under load |

## Further Reading

- [Jepsen-Style Crash Testing](JEPSEN_STYLE_CRASH_TESTING.md)
- [Tools README](../tools/README.md)
