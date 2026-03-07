# Synrix — Privacy and Data Collection

This document answers the specific questions people have about what Synrix collects, where it goes, and what it's used for.

---

## Hardware Fingerprinting (HWID)

### What is collected

On Windows, Synrix reads one value from the Windows registry:

```
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography\MachineGuid
```

This is a GUID that Windows generates during OS installation. It's a stable, unique identifier for the machine. Synrix then computes a SHA256 hash of that GUID.

On Linux/ARM64, equivalent stable system identifiers are used (machine-id from `/etc/machine-id` or similar).

### Why it's collected

License keys are bound to a specific machine at issue time. The HWID hash is embedded in your license key when it's generated. On every startup, Synrix computes the local HWID hash and compares it against the hash stored in your key. If they match, the key is valid. If they don't, the key is rejected.

This prevents a single license key from being shared across unlimited machines.

### Where the HWID goes

**The HWID hash never leaves your machine after a license key is issued.**

The verification is entirely local:

```
1. Read MachineGuid from registry
2. SHA256(MachineGuid) → local_hwid_hash
3. Decode your license key (base64)
4. Compare local_hwid_hash against hwid_hash embedded in key
5. If match + Ed25519 signature valid + not expired → license active
```

No network call is made during this process. No data is sent to Synrix servers. The comparison happens in memory and the result is used locally to set a node limit.

### What we see on our end

When you request a license key, we receive your HWID hash (not the raw MachineGuid — we never see the original GUID, only the hash). We embed that hash into your key. We store the hash alongside your license record so we know which machine a key was issued for.

We do not store your MachineGuid. We store the SHA256 hash of it, which is not reversible to the original value.

---

## Telemetry

The Python SDK includes a `telemetry.py` module. It is **opt-in only** and disabled by default.

```python
# Telemetry is off unless explicitly enabled
# It writes to local files only — no network calls in the default configuration
```

No usage data, query patterns, node counts, or any other operational data is sent to Synrix servers by default.

---

## Network Activity

**`libsynrix.dll` / `libsynrix.so` makes no outbound network connections.**

The engine library is a local storage engine. It reads and writes `.lattice` files on your disk. It has no network code. You can verify this by monitoring network traffic while the library is in use — you will see no outbound connections.

The `synrix-server-evaluation` binary opens a local HTTP port (default 6334) for the Python SDK's `SynrixClient` to connect to. That port is local-only (`localhost`). No external connections are made by the server process during normal operation.

---

## What Stays on Your Machine

Everything. Your `.lattice` files, your node data, your prefix indexes — all of it is stored locally. Synrix has no cloud sync, no backup service, no remote logging. If you delete the `.lattice` file, the data is gone. We have no copy of it.

---

## Summary Table

| Data | Collected? | Sent anywhere? | Stored by us? |
|------|:----------:|:--------------:|:-------------:|
| MachineGuid (raw) | No — only its hash | No | No |
| SHA256(MachineGuid) | Yes — at license issue time | Only at key issuance | Yes, with your license record |
| Node data / query content | No | No | No |
| Usage patterns / query counts | No (opt-in telemetry only) | No | No |
| License key validation result | Local only | No | No |
| `.lattice` file contents | Local only | No | No |

---

## Verification

You can independently verify these claims:

**Check for network activity (Windows):**
```powershell
# While running your application with libsynrix.dll loaded:
netstat -an | findstr ESTABLISHED
# Should show only local connections (127.0.0.1) if using the HTTP server
# libsynrix.dll itself makes no connections
```

**Check what registry key is read (Windows):**
```powershell
# The MachineGuid value Synrix reads:
reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid
# This is a stable, OS-generated value — not personally identifiable information
```

**Check the license key structure:**
The `.lattice` format spec documents the license key payload layout. Your key contains: magic bytes, version, tier, node limit, expiry timestamp, HWID hash (32 bytes), CRC32C, and Ed25519 signature. No personally identifying information beyond the HWID hash.

---

## Questions

If you have questions about data handling not covered here, contact support or open an issue on the GitHub repository.
