#!/usr/bin/env python3
"""
Synrix signed license key generator (Ed25519).

Each key includes a unique 8-byte nonce (or derived from --customer-id) so every
issued key is different; same tier can be issued to many customers with different keys.

Usage:
  # First time: generate keypair, save private key, print public key for embedding in C
  python synrix_license_keygen.py --generate --private synrix_license_private.key

  # Issue a license key for a tier (unique key each time; use same private key)
  python synrix_license_keygen.py --tier 25k --private synrix_license_private.key
  python synrix_license_keygen.py --tier unlimited --private synrix_license_private.key

  # Optional: bind key to a customer id (for your records; engine does not validate it)
  python synrix_license_keygen.py --tier 1m --customer-id "jrob@example.com" --private synrix_license_private.key

  # Optional: set expiry (Unix timestamp; 0 = no expiry)
  python synrix_license_keygen.py --tier 1m --expiry 0 --private synrix_license_private.key

Tiers: 25k|starter, 1m|indie, 10m|growth, 50m|business, unlimited|scale.

The engine reads SYNRIX_LICENSE_KEY from the environment and verifies the signature
with the embedded public key, then applies the tier limit. Keep the private key secure.
"""

import argparse
import base64
import hashlib
import os
import struct
import sys
from pathlib import Path

# Payload format v1 (legacy): version(1) tier(1) expiry(4) = 6 bytes
# Payload format v2 (unique): version(1) tier(1) expiry(4) nonce(8) = 14 bytes
# Tier: 0=25k(starter), 1=1m(indie), 2=10m(growth), 3=50m(business), 4=unlimited(scale)
PAYLOAD_VERSION = 1
NONCE_LEN = 8
TIER_MAP = {
    "25k": 0,
    "starter": 0,
    "100k": 0,   # legacy alias for 25k slot
    "1m": 1,
    "indie": 1,
    "10m": 2,
    "growth": 2,
    "50m": 3,
    "business": 3,
    "unlimited": 4,
    "scale": 4,
}


def main():
    ap = argparse.ArgumentParser(description="Synrix signed license key generator")
    ap.add_argument("--generate", action="store_true", help="Generate new keypair and print public key for C")
    ap.add_argument("--private", type=str, help="Path to private key file (from --generate)")
    ap.add_argument("--tier", type=str, choices=list(TIER_MAP), help="Tier: 25k, starter, 1m, indie, 10m, growth, 50m, business, unlimited, scale")
    ap.add_argument("--expiry", type=int, default=0, help="Expiry Unix timestamp (0 = no expiry)")
    ap.add_argument("--customer-id", type=str, default=None, help="Optional: bind key to this id (hashed to 8 bytes); if omitted, random nonce so each key is unique")
    args = ap.parse_args()

    try:
        import nacl.signing
        import nacl.encoding
    except ImportError:
        print("Requires PyNaCl: pip install pynacl", file=sys.stderr)
        sys.exit(1)

    if args.generate:
        key = nacl.signing.SigningKey.generate()
        private_path = Path(args.private or "synrix_license_private.key")
        private_path.write_bytes(key.encode())
        verify_key = key.verify_key
        pub = bytes(verify_key)
        print("Private key saved to:", private_path)
        print("Embed this public key in license_verify.c (SYNRIX_LICENSE_PUBLIC_KEY):")
        print("static const unsigned char SYNRIX_LICENSE_PUBLIC_KEY[32] = {")
        print("    " + ", ".join(f"0x{b:02x}" for b in pub))
        print("};")
        return

    if not args.private or not args.tier:
        ap.print_help()
        sys.exit(1)

    private_path = Path(args.private)
    if not private_path.is_file():
        print("Private key file not found:", private_path, file=sys.stderr)
        sys.exit(1)

    key = nacl.signing.SigningKey(private_path.read_bytes())
    tier_byte = TIER_MAP[args.tier]
    if args.customer_id:
        nonce = hashlib.sha256(args.customer_id.encode("utf-8")).digest()[:NONCE_LEN]
    else:
        nonce = os.urandom(NONCE_LEN)
    payload = struct.pack("<BBI", PAYLOAD_VERSION, tier_byte, args.expiry & 0xFFFFFFFF) + nonce
    sig = key.sign(payload).signature  # detached signature, 64 bytes
    raw = payload + bytes(sig)
    license_key = base64.b64encode(raw).decode("ascii")
    print(license_key)


if __name__ == "__main__":
    main()
