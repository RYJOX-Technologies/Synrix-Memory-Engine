# Aligning Joseph’s Backend with the C++ Engine

Joseph’s overview (Supabase, Stripe, Resend, Edge Functions) vs. the current C++ engine and key format. What must match so keys work end-to-end.

---

## 1. Key format: must match the engine

The **C++ engine only accepts one format**. Joseph’s backend must emit keys in this format (or the engine will reject them).

**Engine expects (exactly):**

- **One base64 string** (no `SYNRIX.` prefix, no dots).
- Decoded bytes = **6-byte payload** + **64-byte Ed25519 signature** (70 bytes total).
- **Payload:** `version(1 byte) | tier(1 byte) | expiry(4 bytes LE)`  
  - version = 1  
  - tier = 0–4 (see tier table below)  
  - expiry = Unix timestamp, or 0 = no expiry
- **Signature:** Ed25519 over the 6-byte payload. Same keypair as the public key embedded in the engine.

**Example (conceptual):**  
`base64( payload_6_bytes + signature_64_bytes )` → one line, no spaces, no prefix.

**What does *not* work:**  
- `SYNRIX.<payload>.<signature>`  
- JSON in the payload  
- Any other encoding or layout  

So in Supabase Edge Functions, when generating a license key you must:

1. Build the 6-byte payload: `[1, tier_byte, expiry_4_bytes_le]`.
2. Sign those 6 bytes with the **same** Ed25519 private key whose public key is in the C++ engine.
3. Output `base64.encode(payload + signature)` (single string). That’s what the customer puts in `SYNRIX_LICENSE_KEY`.

**Private key:**  
Either (a) store the same `synrix_license_private.key` (32 bytes) in Supabase secrets and implement Ed25519 signing in TypeScript/Deno, or (b) call out to a service that runs the Python keygen and returns that one-line base64. The public key in the C++ engine must match.

---

## 2. Tier mapping: Joseph ↔ engine

**Joseph’s tiers:** starter (25k), indie (1m), growth (10m), business (50m), scale (unlimited).

**Engine tier bytes and limits (after alignment):**

| Tier byte | Node limit | Joseph tier |
|-----------|------------|-------------|
| 0         | 25,000     | starter     |
| 1         | 1,000,000  | indie       |
| 2         | 10,000,000 | growth      |
| 3         | 50,000,000 | business    |
| 4         | unlimited  | scale       |

**Change in C++:**  
Today the engine has tier 0 = 100k. To support Joseph’s “starter = 25k” and “free = key (25k)”, we set **tier 0 = 25,000** in the engine (and add a “starter”/“25k” option in the Python keygen). Then Joseph can issue a key for starter and the engine will enforce 25k.

**Summary:**  
- Backend: when creating a key for starter → tier byte **0**; indie → **1**; growth → **2**; business → **3**; scale → **4**.  
- Engine: tier 0 = 25k, 1 = 1m, 2 = 10m, 3 = 50m, 4 = unlimited.

---

## 3. Free tier: two valid approaches

- **Option A (current engine behavior):** Free = **no key**. Engine defaults to 25k. Joseph doesn’t send a key to free users; they use the app without `SYNRIX_LICENSE_KEY`.
- **Option B (after tier 0 = 25k):** Free = **key with tier 0 (starter)**. Joseph sends a signed key for 25k. Engine accepts it and enforces 25k.

Both are consistent with “starter = 25k”. Option B needs the tier-0 = 25k change in the engine and keygen (see below).

---

## 4. What the engine does and doesn’t do

**Engine does (offline):**

- Reads `SYNRIX_LICENSE_KEY` from the environment.
- Decodes the single base64 string → 6-byte payload + 64-byte signature.
- Verifies Ed25519 signature with the embedded public key.
- Checks version = 1, tier 0–4, expiry (if non-zero).
- Applies node limit (or unlimited) for that process.

**Engine does not (today):**

- No online validation (no call to `check-license` or `validate-license`).
- No device/hardware binding; no device count limit in the engine.
- No revocation check (revocation would require an online check or a new key format).

So:

- **Offline:** Keys work exactly as in Joseph’s “License validation (C++ engine)” – verify signature locally, read tier/expiry, apply limit.
- **Online / device limits:** Joseph’s `validate-license` (license_key + hardware_id) and device limits live entirely in the backend. The engine does not call them. If you later want “online check” or “device binding” in the engine, that would be a separate C++ feature (e.g. optional HTTP call or hardware_id in the payload).

---

## 5. What to change where

**In the C++ engine (Ryan):**

- In `license_verify.c`: set `TIER_LIMITS[0] = 25000u` (instead of 100000u) so tier 0 = starter = 25k.
- Rebuild the DLL and ship the updated `synrix-windows.zip`.

**In the Python keygen (Ryan):**

- Add a tier for starter/25k, e.g. `"25k"` or `"starter"` → tier byte 0, so Joseph (or you) can issue starter keys from the script too.

**In Joseph’s backend (Supabase):**

- Generate keys in the **exact** format above: 6-byte payload + 64-byte Ed25519 signature, then base64 (one string).
- Use the **same** Ed25519 keypair as the engine (public key in `license_verify.c`; private key in Supabase secrets or secure signer).
- Map: starter → tier 0, indie → 1, growth → 2, business → 3, scale → 4.
- Store that one-line key in `profiles.license_key` and send it in emails (welcome, upgrade, resend). No `SYNRIX.` prefix in the value the customer sets.

**In Joseph’s docs/overview:**

- “License key format” for the C++ engine: describe the **single base64 string** (6-byte payload + 64-byte sig), not the `SYNRIX.<payload>.<signature>` / JSON form, for the key that goes into `SYNRIX_LICENSE_KEY`. (Legacy/online-only format can stay as a separate thing if needed for other tooling.)

---

## 6. One-page “what to implement” for Joseph

- **Key format:** One base64 string = `base64( payload_6_bytes + ed25519_signature_64_bytes )`. Payload: `[version=1, tier_byte, expiry_4_bytes_le]`. Tier: 0=25k, 1=1m, 2=10m, 3=50m, 4=unlimited.
- **Keypair:** Use the same Ed25519 key as the C++ engine (private in Supabase; public key in `license_verify.c`).
- **Tiers:** starter=0, indie=1, growth=2, business=3, scale=4.
- **Free:** Either no key (engine defaults to 25k) or key with tier 0 (after we set tier 0 = 25k).
- **Device limits / revocation:** Handled in the backend only; engine does not call APIs or enforce device count.

---

## 7. Files that define the format (for Joseph’s dev)

- **Payload + signature layout:** `synrix-windows-build/build/windows/src/license_verify.c` (PAYLOAD_LEN 6, SIG_LEN 64, decode and verify).
- **Keygen reference:** `synrix-windows-build/build/windows/tools/synrix_license_keygen.py` (payload packing, signing, base64 output).
- **Public key to match:** In `license_verify.c`, `SYNRIX_LICENSE_PUBLIC_KEY` (32 bytes). Any key Joseph’s backend signs with must use the matching private key.

Once the backend emits keys in this format and the engine has tier 0 = 25k, Joseph’s flows (signup, purchase, upgrade, emails, DB) and the C++ engine are aligned.
