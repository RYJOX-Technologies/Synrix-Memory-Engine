#!/usr/bin/env python3
"""One-off: zip key file with the given password. Uses pyminizip (pip install pyminizip)."""
import os
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))
key_file = os.path.join(script_dir, "key_for_joseph_output", "LICENSE_SIGNING_PRIVATE_KEY.txt")
zip_path = os.path.join(script_dir, "key_for_joseph.zip")
password = "MNy9aPWdk3PHFCvu8Nz7UUfS"

try:
    import pyminizip
except ImportError:
    print("Run: pip install pyminizip")
    sys.exit(1)

if not os.path.isfile(key_file):
    print("Key file not found:", key_file)
    sys.exit(1)

if os.path.exists(zip_path):
    os.remove(zip_path)

# compression 5 = default
pyminizip.compress(key_file, None, zip_path, password, 5)

print("Created:", zip_path)
print("Password for Joseph (Part 2):", password)
