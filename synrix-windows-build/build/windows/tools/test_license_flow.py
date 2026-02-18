#!/usr/bin/env python3
"""
Test signed license key flow (keygen + format validation).
Run from build/windows: python tools/test_license_flow.py

Full engine test (DLL + SYNRIX_LICENSE_KEY) requires building the engine
with zlib and optionally OpenSSL (e.g. in MSYS2).
"""

import base64
import struct
import subprocess
import sys
from pathlib import Path

# Payload: legacy 6 bytes or unique 14 bytes (version tier expiry [nonce]). Tier: 0=25k/100k, 1=1m, 2=10m, 3=50m, 4=unlimited
TIER_NAMES = ["100k", "1m", "10m", "50m", "unlimited"]
TIER_LIMITS = [100_000, 1_000_000, 10_000_000, 50_000_000, 0]


def test_key_format(key_b64: str, expected_tier_index: int) -> bool:
    """Decode key and check payload format and tier (no crypto verify). Accepts legacy 70-byte or unique 78-byte keys."""
    raw = base64.b64decode(key_b64)
    if len(raw) not in (6 + 64, 14 + 64):
        print(f"  FAIL: key raw length {len(raw)} not in (70, 78)")
        return False
    payload = raw[:6]
    version, tier, expiry = struct.unpack("<BBI", payload)
    if version != 1:
        print(f"  FAIL: version {version} != 1")
        return False
    if tier != expected_tier_index:
        print(f"  FAIL: tier {tier} != expected {expected_tier_index}")
        return False
    kind = "unique (14-byte)" if len(raw) == 78 else "legacy (6-byte)"
    print(f"  OK: version=1 tier={TIER_NAMES[tier]} expiry={expiry} {kind}")
    return True


def main():
    script_dir = Path(__file__).resolve().parent
    build_windows = script_dir.parent
    private_key = build_windows / "synrix_license_private.key"

    print("1. Keygen: generate keypair (if not exists)...")
    if not private_key.is_file():
        subprocess.run(
            [sys.executable, str(script_dir / "synrix_license_keygen.py"), "--generate", "--private", str(private_key)],
            check=True,
            cwd=str(build_windows),
        )
        print("   OK: keypair generated")
    else:
        print("   OK: private key already exists")

    print("\n2. Keygen: issue keys for each tier...")
    keys = {}
    for tier_name in TIER_NAMES:
        out = subprocess.run(
            [sys.executable, str(script_dir / "synrix_license_keygen.py"), "--tier", tier_name, "--private", str(private_key)],
            capture_output=True,
            text=True,
            cwd=str(build_windows),
        )
        if out.returncode != 0:
            print(f"   FAIL: keygen --tier {tier_name}: {out.stderr}")
            return 1
        key = out.stdout.strip()
        keys[tier_name] = key
        print(f"   {tier_name}: {key[:40]}...")

    print("\n3. Validate key format (payload + length)...")
    for i, tier_name in enumerate(TIER_NAMES):
        if not test_key_format(keys[tier_name], i):
            return 1

    print("\n4. Invalid key: bad base64 -> expect reject in engine")
    print("   (Engine would reject; we only check decode here)")
    try:
        base64.b64decode("not-valid-base64!!!")
    except Exception as e:
        print(f"   OK: bad base64 raises {type(e).__name__}")

    print("\nAll keygen/format tests passed.")
    print("\nTo test the engine with a key:")
    print("  1. Build the engine (e.g. in MSYS2 with zlib and OpenSSL).")
    print("  2. Set SYNRIX_LICENSE_KEY to one of the keys above, e.g.:")
    print(f"     set SYNRIX_LICENSE_KEY={keys['unlimited'][:50]}...")
    print("  3. Run the Python SDK or synrix CLI; tier should be overridden.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
