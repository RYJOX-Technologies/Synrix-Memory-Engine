# License key format – so Joseph’s side matches our system

## What our system uses

- **Algorithm:** **Ed25519** (not RSA).
- **Key length:** The **full** private key in PKCS#8 base64 is **64 characters**. That is correct and complete.
- **Engine:** `license_verify.c` verifies signatures with the **public** key that matches this private key. The engine only supports Ed25519.
- **Keygen:** `synrix_license_keygen.py` signs licenses with this **private** key.

So the “full key” for our system is the 64‑character string below. There is no longer key for Ed25519 in this format.

---

## Format 1: One line (for Supabase secrets)

Use this when setting the secret (e.g. `supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=...`). Copy the **entire** line with no spaces or newlines.

```
MC4CAQAwBQYDK2VwBCIEIFXfeC1UKs8yb2pwqhnciptuP5l3GLL8yHeUVNudwUKf
```

**Character count:** 64. If Joseph’s side only sees 24 (or any shorter length), the string was truncated when copying or pasting.

---

## Format 2: PEM block (if his tooling expects PEM)

Some tools expect a PEM block (header + base64 lines + footer). Same key, same key length, different presentation:

```
-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIFXfeC1UKs8yb2pwqhnciptuP5l3GLL8yHeUVNudwUKf
-----END PRIVATE KEY-----
```

Use either Format 1 or Format 2 depending on what his backend/Cursor agent expects. Both represent the **same** Ed25519 private key.

---

## If his Cursor agent says “key is incorrect” or “wants the full key”

1. **Truncation:** Make sure he copies/pastes the **entire** 64‑character string (or the full PEM block). No line break in the middle of the base64 when using Format 1.
2. **“Full” = PEM:** If the agent expects a “full key” as in “full PEM file,” give him **Format 2** above.
3. **RSA vs Ed25519:** If his backend or docs assume an RSA key (often 200+ characters), that’s a **different** algorithm. Our engine and keygen use **Ed25519** only. The backend must use this Ed25519 key to sign; keys from an RSA keypair will not verify in our engine.

---

## Quick check for Joseph

- **One-line format:** `LICENSE_SIGNING_PRIVATE_KEY` = the 64‑char string, no spaces.
- **PEM format:** `LICENSE_SIGNING_PRIVATE_KEY` = the full PEM block (including `-----BEGIN PRIVATE KEY-----` and `-----END PRIVATE KEY-----`), or his tool may accept a single line that contains the PEM (with `\n` where the newlines are, if required by his API).

No other “full key” is needed for our system; 64 characters is the complete Ed25519 PKCS#8 base64.
