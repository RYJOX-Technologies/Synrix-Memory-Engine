#!/usr/bin/env python3
"""
Cursor IDE Diagnostic Script
Run this in Cursor to diagnose DLL loading issues.
"""

import sys
import os
from pathlib import Path

print("=" * 60)
print("CURSOR IDE SYNRIX DIAGNOSTIC")
print("=" * 60)
print()

print("1. Python Environment")
print(f"   Executable: {sys.executable}")
print(f"   Version: {sys.version}")
print(f"   Platform: {sys.platform}")
print()

print("2. Current Directory")
print(f"   CWD: {os.getcwd()}")
print()

print("3. Python Path (sys.path)")
for i, p in enumerate(sys.path[:10]):  # First 10
    exists = "[OK]" if Path(p).exists() else "[ERROR]"
    print(f"   [{i}] {exists} {p}")
if len(sys.path) > 10:
    print(f"   ... and {len(sys.path) - 10} more")
print()

print("4. Looking for synrix package...")
synrix_found = False
synrix_location = None

# Check sys.path
for path_str in sys.path:
    if path_str:
        try:
            path = Path(path_str).resolve()
            # Check if synrix is directly here
            synrix_dir = path / "synrix"
            if synrix_dir.exists() and (synrix_dir / "__init__.py").exists():
                synrix_found = True
                synrix_location = synrix_dir
                print(f"   [OK] Found in sys.path: {synrix_dir}")
                break
            # Check if path is the synrix package itself
            if path.name == "synrix" and (path / "__init__.py").exists():
                synrix_found = True
                synrix_location = path
                print(f"   [OK] Found in sys.path: {path}")
                break
        except (OSError, ValueError):
            continue

# Check current directory
if not synrix_found:
    cwd = Path.cwd()
    synrix_dir = cwd / "synrix"
    if synrix_dir.exists() and (synrix_dir / "__init__.py").exists():
        synrix_found = True
        synrix_location = synrix_dir
        print(f"   [OK] Found in current directory: {synrix_dir}")

if not synrix_found:
    print("   [ERROR] synrix package not found in sys.path or current directory")
    print()
    print("   SOLUTION: Add the synrix_unlimited directory to sys.path:")
    print("   import sys")
    print("   sys.path.insert(0, r'C:\\path\\to\\synrix_unlimited')")
else:
    print()
    print("5. Checking DLL files...")
    dll_files = [
        "libsynrix.dll",
        "libgcc_s_seh-1.dll",
        "libstdc++-6.dll",
        "libwinpthread-1.dll"
    ]
    
    all_dlls_present = True
    for dll_name in dll_files:
        dll_path = synrix_location / dll_name
        exists = dll_path.exists()
        status = "[OK]" if exists else "[ERROR]"
        size = f"({dll_path.stat().st_size / 1024:.1f} KB)" if exists else ""
        print(f"   {status} {dll_name} {size}")
        if not exists:
            all_dlls_present = False
    
    if not all_dlls_present:
        print()
        print("   [ERROR] Some DLL files are missing!")
        print(f"   Expected location: {synrix_location}")
    else:
        print()
        print("   [OK] All DLL files present")
print()

print("6. Testing Import...")
try:
    # Try to import
    import synrix
    print(f"   [OK] Import successful!")
    print(f"   Location: {synrix.__file__}")
    print()
    
    # Try to load the backend
    print("7. Testing DLL Load...")
    try:
        from synrix._native import load_synrix
        lib = load_synrix()
        print(f"   [OK] DLL loaded successfully!")
        print(f"   Library object: {lib}")
        print()
        print("=" * 60)
        print("SUCCESS - Everything is working.")
        print("=" * 60)
    except Exception as e:
        print(f"   [ERROR] DLL load failed: {e}")
        print()
        print("   TROUBLESHOOTING:")
        print("   1. Set SYNRIX_LIB_PATH environment variable:")
        if synrix_location:
            dll_path = synrix_location / "libsynrix.dll"
            print(f"      $env:SYNRIX_LIB_PATH = '{dll_path}'")
        print("   2. See CURSOR_FIX.md for more solutions")
        print()
        print("=" * 60)
        print("PARTIAL SUCCESS - Package found but DLL won't load")
        print("=" * 60)
        
except ImportError as e:
    print(f"   [ERROR] Import failed: {e}")
    print()
    print("   TROUBLESHOOTING:")
    print("   1. Add synrix_unlimited to sys.path:")
    print("      import sys")
    print("      sys.path.insert(0, r'C:\\path\\to\\synrix_unlimited')")
    print("   2. Or install the package:")
    print("      pip install -e .")
    print()
    print("=" * 60)
    print("FAILED - Package not found")
    print("=" * 60)

print()
print("For more help, see:")
print("  - CURSOR_FIX.md (Cursor-specific solutions)")
print("  - TROUBLESHOOTING.md (general troubleshooting)")
