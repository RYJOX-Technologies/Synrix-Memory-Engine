#!/usr/bin/env python3
"""
Encrypt the Synrix license private key (PKCS8 base64) for secure transfer.
You send the OUTPUT (Part A) on Discord. Send the PASSWORD (Part B) separately
(e.g. Signal, phone, or a different Discord message after Joseph confirms ready).

Usage:
  1. Export PKCS8 base64 first (to a file):
       python export_private_key_pkcs8_base64.py synrix_license_private.key > pkcs8.txt
  2. Encrypt it (you will be prompted for a password):
       python encrypt_key_for_transfer.py pkcs8.txt
  3. Copy the printed base64 blob and send it to Joseph (Part A).
  4. Send Joseph the password via a different channel (Part B).
  5. Give Joseph the decrypt script (decrypt_key_for_transfer.py) or the instructions below.
"""

import base64
import getpass
import os
import sys
from pathlib import Path

def main():
    if len(sys.argv) != 2:
        print("Usage: python encrypt_key_for_transfer.py <pkcs8_base64_file>", file=sys.stderr)
        print("  First run: python export_private_key_pkcs8_base64.py synrix_license_private.key > pkcs8.txt", file=sys.stderr)
        sys.exit(1)
    path = Path(sys.argv[1])
    if not path.is_file():
        print("File not found:", path, file=sys.stderr)
        sys.exit(1)
    payload = path.read_text().strip()
    if not payload:
        print("File is empty.", file=sys.stderr)
        sys.exit(1)
    password = getpass.getpass("Enter a strong password (Joseph will need this to decrypt): ")
    if len(password) < 12:
        print("Use at least 12 characters.", file=sys.stderr)
        sys.exit(1)
    password2 = getpass.getpass("Enter the same password again: ")
    if password != password2:
        print("Passwords do not match.", file=sys.stderr)
        sys.exit(1)
    try:
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        from cryptography.hazmat.backends import default_backend
        from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
        from cryptography.hazmat.primitives import hashes
    except ImportError:
        print("Requires: pip install cryptography", file=sys.stderr)
        sys.exit(1)
    salt = os.urandom(16)
    kdf = PBKDF2HMAC(algorithm=hashes.SHA256(), length=32, salt=salt, iterations=480000, backend=default_backend())
    key = kdf.derive(password.encode("utf-8"))
    aes = AESGCM(key)
    nonce = os.urandom(12)
    ct = aes.encrypt(nonce, payload.encode("utf-8"), None)
    blob = salt + nonce + ct
    b64 = base64.b64encode(blob).decode("ascii")
    print("\n--- Part A: Send this to Joseph (e.g. paste in Discord) ---\n")
    print(b64)
    print("\n--- End Part A ---")
    print("\nSend the PASSWORD (Part B) to Joseph via a DIFFERENT channel (Signal, phone, etc.).")
    print("Give him decrypt_key_for_transfer.py and the instructions in DECRYPT_INSTRUCTIONS.md")

if __name__ == "__main__":
    main()
