#!/usr/bin/env python3
"""
Quick fix script for DLL path issues after pip install -e .
This sets the SYNRIX_LIB_PATH environment variable to point to the DLL location.
"""

import os
import sys
from pathlib import Path

print("=== SYNRIX DLL Path Fix ===\n")

# Find where synrix package is actually installed
try:
    import synrix
    synrix_path = Path(synrix.__file__).parent
    print(f"Found synrix package at: {synrix_path}")
except ImportError:
    print("ERROR: synrix package not found!")
    print("Make sure you ran: pip install -e .")
    sys.exit(1)

# Check for DLL in package directory
dll_path = synrix_path / "libsynrix.dll"
print(f"DLL path: {dll_path}")
print(f"DLL exists: {dll_path.exists()}")

if not dll_path.exists():
    print("\n[ERROR] DLL not found in package directory!")
    print("\nThis usually means the DLLs weren't copied during installation.")
    print("\nQuick fix options:")
    print("\n1. Set environment variable manually:")
    print(f'   set SYNRIX_LIB_PATH={synrix_path / "libsynrix.dll"}')
    print("\n2. Or in PowerShell:")
    print(f'   $env:SYNRIX_LIB_PATH = "{synrix_path / "libsynrix.dll"}"')
    print("\n3. Or in your Python script, add at the top:")
    print(f'   import os')
    print(f'   os.environ["SYNRIX_LIB_PATH"] = r"{synrix_path / "libsynrix.dll"}"')
    sys.exit(1)

# Set the environment variable for this session
os.environ["SYNRIX_LIB_PATH"] = str(dll_path)
    print(f"\n[OK] Set SYNRIX_LIB_PATH={dll_path}")

# Test if it works now
print("\n=== Testing Import ===")
try:
    from synrix._native import load_synrix
    lib = load_synrix()
    print("[OK] SUCCESS! DLL loads correctly now.")
    print("\nTo make this permanent, add this to your Python script:")
    print(f'   import os')
    print(f'   os.environ["SYNRIX_LIB_PATH"] = r"{dll_path}"')
    print("\nOr set it as a system environment variable:")
    print(f'   [System.Environment]::SetEnvironmentVariable("SYNRIX_LIB_PATH", "{dll_path}", "User")')
except Exception as e:
    print(f"[ERROR] Still failed: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
