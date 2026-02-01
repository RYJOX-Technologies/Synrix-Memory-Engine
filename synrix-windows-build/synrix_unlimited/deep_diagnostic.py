#!/usr/bin/env python3
"""
Deep diagnostic - shows exactly what's happening with DLL loading
"""

import os
import sys
import ctypes
from pathlib import Path

print("=" * 70)
print("  SYNRIX Deep Diagnostic - What's Really Happening?")
print("=" * 70)
print()

# 1. Check Python version and architecture
print("1. Python Environment:")
print(f"   Python: {sys.version}")
print(f"   Executable: {sys.executable}")
print(f"   Platform: {sys.platform}")
print(f"   Architecture: {ctypes.sizeof(ctypes.c_void_p) * 8} bit")
print()

# 2. Check if synrix is installed
print("2. SYNRIX Package Location:")
try:
    import synrix
    synrix_file = synrix.__file__
    synrix_path = Path(synrix_file).parent
    print(f"   [OK] synrix.__file__ = {synrix_file}")
    print(f"   [OK] Package directory = {synrix_path}")
    print(f"   [OK] Package exists = {synrix_path.exists()}")
except ImportError as e:
    print(f"   [ERROR] Cannot import synrix: {e}")
    print("   Make sure you ran: pip install -e .")
    sys.exit(1)
print()

# 3. Check for DLLs
print("3. DLL Files:")
dlls_to_check = [
    "libsynrix.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
]

all_dlls_found = True
for dll_name in dlls_to_check:
    dll_path = synrix_path / dll_name
    exists = dll_path.exists()
    status = "[OK]" if exists else "[MISSING]"
    print(f"   {status} {dll_name:30s} = {dll_path}")
    if exists:
        size = dll_path.stat().st_size
        print(f"        Size: {size:,} bytes ({size/1024:.1f} KB)")
    else:
        all_dlls_found = False
print()

if not all_dlls_found:
    print("[ERROR] Some DLLs are missing!")
    print("This is the problem - the DLLs aren't where Python expects them.")
    print()
    print("Possible causes:")
    print("  1. DLLs weren't included in the package")
    print("  2. DLLs are in a different location")
    print("  3. pip install -e . didn't copy DLLs (editable install)")
    print()
    sys.exit(1)

# 4. Check DLL architecture
print("4. DLL Architecture Check:")
try:
    # Try to read DLL header to check architecture
    dll_path = synrix_path / "libsynrix.dll"
    with open(dll_path, 'rb') as f:
        header = f.read(64)
        # Check for PE signature
        if header[0:2] == b'MZ':
            # Check machine type at offset 4 (after PE signature)
            pe_offset = int.from_bytes(header[60:64], 'little')
            f.seek(pe_offset)
            pe_header = f.read(6)
            if pe_header[0:2] == b'PE':
                machine = int.from_bytes(pe_header[4:6], 'little')
                if machine == 0x8664:  # IMAGE_FILE_MACHINE_AMD64
                    print(f"   [OK] DLL is 64-bit (matches Python)")
                elif machine == 0x014c:  # IMAGE_FILE_MACHINE_I386
                    print(f"   [ERROR] DLL is 32-bit but Python is 64-bit!")
                    print("   This is the problem - architecture mismatch!")
                else:
                    print(f"   [WARN] Unknown architecture: 0x{machine:x}")
except Exception as e:
    print(f"   [WARN] Could not check DLL architecture: {e}")
print()

# 5. Try to load DLL manually
print("5. Manual DLL Load Test:")
dll_path = synrix_path / "libsynrix.dll"
print(f"   Attempting to load: {dll_path}")

# Add directory to DLL search path
try:
    os.add_dll_directory(str(synrix_path))
    print(f"   [OK] Added {synrix_path} to DLL search path")
except Exception as e:
    print(f"   [WARN] Could not add DLL directory: {e}")

# Try loading
try:
    lib = ctypes.WinDLL(str(dll_path))
    print(f"   [OK] DLL loaded successfully!")
    print(f"   Library object: {lib}")
    print()
    print("=" * 70)
    print("  [OK] DLL LOAD WORKS - The problem is in _native.py logic!")
    print("=" * 70)
except OSError as e:
    print(f"   [ERROR] Failed to load DLL: {e}")
    print()
    print("   This is the actual error from Windows:")
    print(f"   {str(e)}")
    print()
    print("   Common causes:")
    error_str = str(e).lower()
    if "dependent" in error_str:
        print("   - Missing DLL dependencies (MinGW runtime)")
        print("   - Even though DLLs are present, Windows can't find them")
    elif "not found" in error_str:
        print("   - DLL or its dependencies not in PATH")
        print("   - Windows security blocking DLL loading")
    elif "invalid" in error_str or "format" in error_str:
        print("   - Architecture mismatch (32-bit vs 64-bit)")
        print("   - Corrupted DLL file")
    else:
        print("   - Unknown error - see message above")
    print()
    print("=" * 70)
    print("  [ERROR] THIS IS THE PROBLEM - DLL won't load")
    print("=" * 70)
    sys.exit(1)
except Exception as e:
    print(f"   [ERROR] Unexpected error: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

# 6. Test _native.py loading
print()
print("6. Testing _native.py load_synrix():")
try:
    from synrix._native import load_synrix
    lib2 = load_synrix()
    print(f"   [OK] _native.py load_synrix() works!")
    print(f"   Library object: {lib2}")
    print()
    print("=" * 70)
    print("  [OK] EVERYTHING WORKS!")
    print("=" * 70)
except Exception as e:
    print(f"   [ERROR] _native.py load_synrix() failed: {e}")
    print()
    print("   This means _native.py has a bug in its path resolution.")
    print("   The DLL loads manually, but _native.py can't find it.")
    import traceback
    traceback.print_exc()
    sys.exit(1)
