#!/usr/bin/env python3
"""
Export Synrix license private key as PKCS8 for Supabase LICENSE_SIGNING_PRIVATE_KEY.
Reads the same raw 32-byte file used by synrix_license_keygen.py.

Usage:
  python export_private_key_pkcs8_base64.py synrix_license_private.key
  python export_private_key_pkcs8_base64.py --pem synrix_license_private.key

Default: one line of base64 (~88 chars for Ed25519 – that is correct; RSA keys are longer).
--pem: full PEM block (if backend expects PEM with header/footer).
"""

import base64
import sys
from pathlib import Path

def main():
    argv = sys.argv[1:]
    pem_mode = "--pem" in argv
    if pem_mode:
        argv = [a for a in argv if a != "--pem"]
    if len(argv) != 1:
        print("Usage: python export_private_key_pkcs8_base64.py [--pem] <path_to_synrix_license_private.key>", file=sys.stderr)
        sys.exit(1)
    path = Path(argv[0])
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
    if pem_mode:
        # PEM: header, base64 (64 chars/line), footer
        lines = [b64[i:i+64] for i in range(0, len(b64), 64)]
        print("-----BEGIN PRIVATE KEY-----")
        print("\n".join(lines))
        print("-----END PRIVATE KEY-----")
    else:
        print(b64)

if __name__ == "__main__":
    main()
