# Synrix License – User Journey & Exact Flow

For Joseph: exact user flows and what to send customers.

**Verified:** Flow matches engine behavior (no key → 25k default; key read from `SYNRIX_LICENSE_KEY` at startup; invalid/expired → 25k fallback).

---

## User journey (exact flow)

### Flow 1: Free user (no key)

1. User downloads **synrix-windows.zip** from the Releases page.
2. User unzips and runs their app (or our Python example) **without** setting any license key.
3. Engine starts; no key → default **25k node limit** (free tier).
4. User can use Synrix up to 25,000 nodes. No signup, no key, no payment.

**Outcome:** Free tier = 25k nodes, no key required.

---

### Flow 2: Paid user (first-time key)

1. Customer purchases a tier (e.g. 100k, 1m, 10m, 50m, or unlimited).
2. **We** run keygen from `build/windows/`: `python tools/synrix_license_keygen.py --tier 1m --private synrix_license_private.key` → one line of output (the key).
3. **We** send the customer that key (email, dashboard, etc.).
4. **Customer** downloads the same **synrix-windows.zip** (same binary as everyone else).
5. **Customer** sets the license key:
   - **Option A (dev):** Set env var `SYNRIX_LICENSE_KEY` to the key we sent (PowerShell / cmd / profile).
   - **Option B (non-technical, future):** Run our “paste your key” script or use an installer that writes the key for them.
6. **Customer** runs their app. Engine reads the key at startup → tier applied (e.g. 1m nodes).
7. **Outcome:** Customer has 1m nodes (or whatever tier we issued).

---

### Flow 3: Upgrade (more nodes)

1. Customer wants to upgrade (e.g. 100k → 1m).
2. **We** issue a new key for the new tier (same keygen, different tier).
3. **We** send the new key to the customer.
4. **Customer** replaces the old key with the new key (same place they set it: env var or script/installer).
5. **Customer** restarts their app (or restarts the process that uses Synrix).
6. Engine reads the new key at startup → new tier applied.
7. **Outcome:** No re-download, no new binary. “Paste new key → more nodes.”

---

### Flow 4: Invalid or expired key

1. Customer has a key that’s invalid (typo, tampered, wrong key) or expired.
2. Customer runs their app. Engine verifies the key at startup → verification fails or expiry hit.
3. Engine **falls back to default**: 25k nodes (same as no key).
4. **Current behavior:** No custom message (we could add “License invalid or expired; using default 25k nodes” in a future build).
5. **Outcome:** App still works at 25k; customer needs a valid key for higher tier.

---

## One-line summary for Joseph

**Free:** Download zip, no key → 25k nodes.  
**Paid:** We send key → customer pastes key (env var or script) → restart app → tier applied.  
**Upgrade:** We send new key → customer pastes new key → restart app → more nodes. No new download.

---

## What we send the customer

| When | What we send |
|------|----------------|
| Purchase / signup | **One license key** (single line of base64 text) for their tier (100k / 1m / 10m / 50m / unlimited). |
| Download | **Same link** for everyone: synrix-windows.zip from GitHub Releases (or our site). |
| Upgrade | **New key** for the new tier; same zip, no re-download. |

We do **not** send different builds or different zips per tier. One zip, one binary; the key sets the tier.

---

## What the customer does (minimal)

1. Download **synrix-windows.zip** (once).
2. Unzip and install/use the SDK (per our docs).
3. **Set license key** (env var or “paste key” script): `SYNRIX_LICENSE_KEY = "<key we sent>"`.
4. Run their app. Restart after any key change.

That’s the full user journey.
