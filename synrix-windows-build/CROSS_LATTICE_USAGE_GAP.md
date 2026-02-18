# Cross-lattice usage gap (and workarounds)

Joseph raised this; it affects both **reporting** and **enforcement**.

---

## The gap

1. **Reporting:** Each SDK reports only **this** lattice’s node count. The backend overwrites `current_usage` with the last report. So if the user has three projects (10k + 8k + 5k = 23k), the backend might only see 5k. Warning emails can be wrong or never fire.

2. **Enforcement:** The engine enforces the limit **per lattice file**, not per user/license. So one user on 25k free could run:
   - Project A: 25k nodes  
   - Project B: 25k nodes  
   - Project C: 25k nodes  
   = 75k on a 25k plan. Each lattice is under 25k, so the engine allows it.

---

## Short-term workaround (no engine change): backend sums by instance

**Idea:** Stop overwriting a single `current_usage`. Store **one row per report** (per lattice/instance) and **sum by license key** for “total usage.”

- SDK sends: `license_key`, `current_usage`, `hardware_id`, and an **instance id** (e.g. hash of lattice path or a UUID per process). So each “project” or lattice is one row.
- Backend stores: e.g. `(license_key, hardware_id, instance_id, current_usage, updated_at)`.
- **Total usage** for a license = sum of `current_usage` over all rows for that `license_key`.
- Warning / over-limit emails use this **total**. No engine change; reporting and emails become correct for multi-project users.

**What we added in the SDK:** All three SDKs now send an **instance_id** in the report payload (RAG: hash of collection name; robotics/agent: hash of lattice path). Backend can store per (license_key, hardware_id, instance_id) and sum for total usage.

**What Joseph does:** Change backend to “accumulate” usage per (license_key, hardware_id, instance_id) and use the sum for limits/emails. Optional: periodic cleanup of stale instance_ids (e.g. no report in 30 days).

---

## Proper fix (engine V2): global cap per license

**Idea:** Engine enforces a **global** node count for the license key across **all** lattice files on that machine.

- On the machine, keep a **shared store** (e.g. file or small daemon) keyed by license key: “total nodes across all lattices using this key.”
- When a process opens a lattice with `SYNRIX_LICENSE_KEY` set:
  - Register this lattice’s current count in the shared total (with a lock).
  - On each write: check shared total &lt; tier limit; if so, increment shared total and allow the write.
- SDK reports that **same** global total to the backend, so backend and engine agree.

This needs C changes (cross-process lock, shared counter). Doable as V2; not required for launch.

---

## What works today without changes

- **Single project / single lattice:** Reporting and per-lattice enforcement are correct.
- **Upgrades:** New key, higher limit, same flow.
- **Emails:** Correct whenever the reported number is the only lattice (most users).

So for launch this is acceptable. The short-term workaround (backend sums by instance + SDK sends `instance_id`) improves reporting and emails for multi-project users; the full fix is global enforcement in the engine later.
