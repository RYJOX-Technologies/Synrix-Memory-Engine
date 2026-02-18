#!/usr/bin/env python3
"""
Export Synrix license private key as PKCS8 DER, base64-encoded, for Supabase
LICENSE_SIGNING_PRIVATE_KEY. Reads the same raw 32-byte file used by synrix_license_keygen.py.

Usage (from this folder):
  python export_private_key_pkcs8_base64.py synrix_license_private.key
  python export_private_key_pkcs8_base64.py <full-path-to-key-file>

Output: one line of base64 (paste into supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=...)
"""

import base64
import sys
from pathlib import Path

def main():
    if len(sys.argv) != 2:
        print("Usage: python export_private_key_pkcs8_base64.py <path_to_synrix_license_private.key>", file=sys.stderr)
        sys.exit(1)
    path = Path(sys.argv[1])
    if not path.is_file():
        print("File not found:", path, file=sys.stderr)
        sys.exit(1)
    raw32 = path.read_bytes()
    if len(raw32) != 32:
        print("Expected 32-byte raw Ed25519 key, got", len(raw32), file=sys.stderr)
        sys.exit(1)
    try:
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        from cryptography.hazmat.primitives.serialization import (
            Encoding,
            PrivateFormat,
            NoEncryption,
        )
    except ImportError:
        print("Requires: pip install cryptography", file=sys.stderr)
        sys.exit(1)
    pk = Ed25519PrivateKey.from_private_bytes(raw32)
    der = pk.private_bytes(encoding=Encoding.DER, format=PrivateFormat.PKCS8, encryption_algorithm=NoEncryption())
    b64 = base64.b64encode(der).decode("ascii")
    print(b64)

if __name__ == "__main__":
    main()
