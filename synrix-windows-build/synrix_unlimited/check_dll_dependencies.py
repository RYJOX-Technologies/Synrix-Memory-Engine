#!/usr/bin/env python3
"""
Check if all required DLLs are present and diagnose loading issues
"""

import os
import sys
from pathlib import Path

print("=== SYNRIX DLL Dependency Check ===\n")

# Find synrix package
try:
    import synrix
    synrix_path = Path(synrix.__file__).parent
    print(f"Synrix package location: {synrix_path}\n")
except ImportError:
    # Try to find it manually
    current_dir = Path.cwd()
    synrix_path = current_dir / "synrix"
    if not synrix_path.exists():
        print("ERROR: Cannot find synrix package!")
        sys.exit(1)
    print(f"Using current directory: {synrix_path}\n")

# Required DLLs
required_dlls = [
    "libsynrix.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
]

print("Checking for required DLLs:")
print("-" * 60)
all_present = True
for dll_name in required_dlls:
    dll_path = synrix_path / dll_name
    exists = dll_path.exists()
    status = "[OK]" if exists else "[MISSING]"
    print(f"{status} {dll_name:30s} {'Found' if exists else 'MISSING'}")
    if not exists:
        all_present = False
        print(f"   Path checked: {dll_path}")

print()

if not all_present:
    print("[ERROR] MISSING DLLs detected!")
    print("\nThe MinGW runtime DLLs must be in the same directory as libsynrix.dll")
    print(f"\nExpected location: {synrix_path}")
    print("\nThese DLLs should have been included in the package.")
    print("If they're missing, the package may be incomplete.")
    sys.exit(1)

print("[OK] All required DLLs are present!")
print()

# Try to load the DLL
print("=== Testing DLL Load ===")
try:
    import ctypes
    
    # Add the directory to DLL search path
    os.add_dll_directory(str(synrix_path))
    
    # Try to load
    dll_path = synrix_path / "libsynrix.dll"
    print(f"Attempting to load: {dll_path}")
    
    lib = ctypes.WinDLL(str(dll_path))
    print("[OK] DLL loaded successfully!")
    print(f"Library object: {lib}")
    
except OSError as e:
    print(f"[ERROR] Failed to load DLL: {e}")
    print()
    print("Common causes:")
    print("  1. Missing MinGW runtime DLLs (but we checked - they're there)")
    print("  2. Architecture mismatch (32-bit vs 64-bit)")
    print("  3. Corrupted DLL file")
    print("  4. Windows security blocking the DLL")
    print()
    print("Try this fix:")
    print(f'   set SYNRIX_LIB_PATH={dll_path}')
    print("   Then test again")
    sys.exit(1)
    
except Exception as e:
    print(f"[ERROR] Unexpected error: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("\n=== SUCCESS ===")
print("[OK] Everything is working correctly!")
