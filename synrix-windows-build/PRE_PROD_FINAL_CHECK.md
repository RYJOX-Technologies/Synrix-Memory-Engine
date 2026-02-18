# Pre-Prod Final Check – License + Engine + Backend

One-time verification before rolling out to production. Run through this so nothing surprises you.

---

## 1. Engine behavior (verified in code)

| Check | Status | Notes |
|-------|--------|--------|
| **No key / invalid key** | OK | Falls back to 25k (hard-coded in `persistent_lattice.c` when no `SYNRIX_FREE_TIER_LIMIT` define). |
| **Valid key** | OK | `synrix_license_parse` runs at lattice init; tier 0–4 → 25k, 1m, 10m, 50m, unlimited. |
| **Expired key** | OK | Rejected (expiry != 0 and expiry < time(NULL)); fallback to 25k. |
| **Tier byte > 4** | OK | Rejected (version != 1 or tier > 4). |
| **Bad base64 / wrong length** | OK | Decode or length check fails → 25k. |
| **License read only at init** | OK | Env var is read once when the lattice is created; changing `SYNRIX_LICENSE_KEY` later requires restart. |

---

## 2. Key format alignment

| Item | Engine | Keygen / Joseph |
|------|--------|-----------------|
| Payload | 6 bytes: version(1), tier(1), expiry(4 LE) | Same (`struct.pack("<BBI", ...)` in keygen). |
| Signature | 64 bytes Ed25519 over payload | Same. |
| Output | Single base64(70 bytes) | Same; no prefix, no dots. |
| Tier map | 0=25k, 1=1m, 2=10m, 3=50m, 4=unlimited | Same. |

**Expiry:** Stored little-endian. Engine uses `memcpy` to `uint32_t` (fine on Windows/LE). If you ever port to a big-endian platform, expiry would need to be byte-swapped.

---

## 3. Build and packaging

| Check | Action |
|-------|--------|
| **OpenSSL at build** | Build must find OpenSSL (e.g. MSYS2: `pacman -S mingw-w64-x86_64-openssl`). If not found, license verification is **disabled** and every key is rejected → 25k. Confirm CMake output shows "OpenSSL found". |
| **Single DLL for prod** | Ship the **license-key** build (no `-DSYNRIX_FREE_TIER_50K=ON`). That build does not define `SYNRIX_FREE_TIER_LIMIT`, so default 25k + license override. Do **not** ship `libsynrix_free_tier.dll` as the main prod binary. |
| **OpenSSL at runtime** | Copy `libcrypto*.dll` and `libssl*.dll` into every package `synrix/` folder (and SDKs). Without them, the process can exit silently when the DLL loads. |
| **Private key** | Never in repo or in any artifact. Only in Supabase secrets (Joseph) and your secure keygen environment. |

---

## 4. Backend (Joseph)

| Check | Notes |
|-------|--------|
| **Same keypair** | Joseph must use the private key you exported (PKCS8 base64) in `LICENSE_SIGNING_PRIVATE_KEY`. Keys signed with a different keypair will fail verification. |
| **Payload layout** | 6 bytes: [1, tier_byte, expiry_le_4]. No JSON. |
| **One base64 string** | No `SYNRIX.` prefix; no second segment after a dot. |

---

## 5. Edge cases (no surprises)

| Scenario | Result |
|----------|--------|
| User revokes key in dashboard | Engine does **not** call online. Revoked key still works offline until expiry (or forever if expiry=0). Revocation = “don’t renew” / “block at next online check” if you add that later. |
| User shares key | No detection in engine. Device limits are enforced only if the app calls Joseph’s `validate-license` with `hardware_id`. |
| User sets key with trailing newline/space | Engine’s base64 decoder skips spaces and `\r`/`\n`, so it still works. |
| User upgrades tier | New key in email; user sets new `SYNRIX_LICENSE_KEY` and restarts. No engine change needed. |

---

## 6. Before you ship

1. **Build** the engine with OpenSSL (license-key build, not free_tier).
2. **Run** one no-key test in a **new** PowerShell: unset `SYNRIX_LICENSE_KEY`, run app, add nodes → should cap at 25k.
3. **Run** one key test: set a key (e.g. indie), add 1M+ nodes → should succeed.
4. **Confirm** every distributable package (zip, SDKs) contains `libsynrix.dll` **and** OpenSSL DLLs in the same folder.
5. **Send** Joseph the private key (PKCS8 base64) and one test key (e.g. indie) via a secure channel.
6. **Document** for users: set `SYNRIX_LICENSE_KEY='<key>'` (single quotes on Unix so shell doesn’t mangle base64).

---

## 7. Summary

- **Engine:** No key or bad key → 25k. Valid key → tier limit. Expiry and version enforced. OpenSSL required at build and runtime.
- **Key:** One base64 string (6 + 64 bytes). Same keypair everywhere. Joseph’s backend must match this format.
- **Packaging:** Single `libsynrix.dll` (license build) + OpenSSL DLLs in every package; no private key in artifacts.

Once the above is done, rollout is consistent and there are no hidden surprises.
