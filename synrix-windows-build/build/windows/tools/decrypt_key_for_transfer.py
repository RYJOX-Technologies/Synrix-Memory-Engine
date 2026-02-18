#!/usr/bin/env python3
"""
Decrypt the Synrix license key (Part A) using the password (Part B) from Ryan.
Output is the PKCS8 base64 string to set in Supabase:
  supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=<paste output here>

Usage:
  python decrypt_key_for_transfer.py part_a.txt
  (Then enter password when prompted.)
  Or: python decrypt_key_for_transfer.py
  (Paste the Part A blob, then Ctrl+Z / Ctrl+D, then enter password.)
"""

import base64
import getpass
import sys
from pathlib import Path

def main():
    try:
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        from cryptography.hazmat.backends import default_backend
        from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
        from cryptography.hazmat.primitives import hashes
    except ImportError:
        print("Requires: pip install cryptography", file=sys.stderr)
        sys.exit(1)
    if len(sys.argv) >= 2:
        path = Path(sys.argv[1])
        if path.is_file():
            b64 = path.read_text().replace(" ", "").replace("\n", "")
        else:
            print("File not found:", path, file=sys.stderr)
            sys.exit(1)
    else:
        print("Paste the Part A blob (base64 from Ryan), then press Enter and Ctrl+Z (Windows) or Ctrl+D (Mac/Linux):", file=sys.stderr)
        lines = []
        for line in sys.stdin:
            lines.append(line.rstrip())
        b64 = "".join(lines).replace(" ", "").replace("\n", "")
    if not b64:
        print("No input.", file=sys.stderr)
        sys.exit(1)
    try:
        blob = base64.b64decode(b64)
    except Exception as e:
        print("Invalid base64:", e, file=sys.stderr)
        sys.exit(1)
    if len(blob) < 16 + 12 + 16:
        print("Blob too short.", file=sys.stderr)
        sys.exit(1)
    salt = blob[:16]
    nonce = blob[16:28]
    ct = blob[28:]
    password = getpass.getpass("Enter the password Ryan sent you (Part B): ")
    kdf = PBKDF2HMAC(algorithm=hashes.SHA256(), length=32, salt=salt, iterations=480000, backend=default_backend())
    key = kdf.derive(password.encode("utf-8"))
    aes = AESGCM(key)
    try:
        pt = aes.decrypt(nonce, ct, None)
    except Exception:
        print("Decryption failed. Wrong password or corrupted blob.", file=sys.stderr)
        sys.exit(1)
    pkcs8_base64 = pt.decode("utf-8").strip()
    print("\n--- Set this in Supabase (one line) ---\n")
    print(pkcs8_base64)
    print("\n--- End ---")
    print("\nRun: supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=<paste above>", file=sys.stderr)

if __name__ == "__main__":
    main()
