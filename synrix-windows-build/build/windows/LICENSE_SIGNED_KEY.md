# Signed License Key (Ed25519)

The engine supports a **signed license key** so tier (100k / 1m / 10m / 50m / unlimited) is set at runtime without redownloading. Keys are **Ed25519-signed**; only you can issue valid keys.

## For Joseph (security)

- **Verification:** Engine embeds your **public key** (32 bytes). It reads `SYNRIX_LICENSE_KEY` from the environment, decodes base64, and verifies the **signature** over the payload (version, tier, expiry). If verification fails or the key is expired, the engine falls back to the default tier (compile-time or 25k).
- **Keygen:** Only someone with the **private key** can create valid keys. Keep the private key secure (e.g. on a build server or secure machine). The keygen script (`tools/synrix_license_keygen.py`) uses PyNaCl (Ed25519) to sign; the C engine uses OpenSSL 1.1.1+ to verify.
- **Tampering:** Changing the tier in the key without the private key breaks the signature; the engine rejects it.

## One-time setup (generate keypair and embed public key)

1. Install PyNaCl: `pip install pynacl`
2. From `build/windows/` run:
   ```bash
   python tools/synrix_license_keygen.py --generate --private synrix_license_private.key
   ```
3. **Save the private key** (`synrix_license_private.key`) somewhere secure. Do not commit it.
4. **Replace the public key in C:** Open `src/license_verify.c` and replace the 32 bytes in `SYNRIX_LICENSE_PUBLIC_KEY` with the array printed by the script (the script outputs a C array you can paste).
5. Rebuild the engine. All builds (single binary) will now accept only keys signed by that private key.

## Issuing a license key for a customer

Using the **same** private key file:

```bash
# 100k nodes (free tier)
python tools/synrix_license_keygen.py --tier 100k --private synrix_license_private.key

# 1M nodes
python tools/synrix_license_keygen.py --tier 1m --private synrix_license_private.key

# 10M nodes
python tools/synrix_license_keygen.py --tier 10m --private synrix_license_private.key

# 50M nodes
python tools/synrix_license_keygen.py --tier 50m --private synrix_license_private.key

# Unlimited
python tools/synrix_license_keygen.py --tier unlimited --private synrix_license_private.key
```

Each command prints one line: the license key (base64). Send that to the customer.

Optional: set an expiry (Unix timestamp; 0 = no expiry):

```bash
python tools/synrix_license_keygen.py --tier 1m --expiry 0 --private synrix_license_private.key
```

## Customer usage

They set the environment variable before running the app (or in their shell/profile):

- **Windows (PowerShell):** `$env:SYNRIX_LICENSE_KEY = "paste_key_here"`
- **Windows (cmd):** `set SYNRIX_LICENSE_KEY=paste_key_here`
- **Linux/macOS:** `export SYNRIX_LICENSE_KEY="paste_key_here"`

They use **one** engine binary (e.g. the same zip for everyone). The tier is determined by the key; no redownload for upgrades. For an upgrade, issue a new key for the new tier and send it to them.

## Build requirement

- **OpenSSL 1.1.1 or later** (for Ed25519 verify). On MSYS2: `pacman -S mingw-w64-x86_64-openssl`. If OpenSSL is not found at build time, the engine still builds but license verification is disabled (any key is rejected; default tier is used).

## Summary

| Step | Who | Action |
|------|-----|--------|
| 1 | You | Run keygen `--generate`, save private key, paste public key into `license_verify.c`, rebuild |
| 2 | You | For each customer/tier, run keygen `--tier X --private synrix_license_private.key`, send the printed key |
| 3 | Customer | Set `SYNRIX_LICENSE_KEY` to the key you sent; run the app (single binary) |

This gives Joseph the signed-key level: keys are tamper-proof and only you can issue them.
