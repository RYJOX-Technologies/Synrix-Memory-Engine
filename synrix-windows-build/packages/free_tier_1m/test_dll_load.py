#!/usr/bin/env python3
"""
Quick diagnostic script to test DLL loading.
Run this from the synrix_unlimited directory to diagnose DLL loading issues.
"""

import os
import sys
from pathlib import Path

print("=== SYNRIX DLL Loading Diagnostic ===\n")

# Check current directory
print(f"Current directory: {os.getcwd()}")
print(f"Script location: {__file__}")
print()

# Check if synrix package exists
synrix_dir = Path(__file__).parent / "synrix"
print(f"Synrix package directory: {synrix_dir}")
print(f"Exists: {synrix_dir.exists()}")
print()

# Check for DLL
dll_path = synrix_dir / "libsynrix.dll"
print(f"DLL path: {dll_path}")
print(f"DLL exists: {dll_path.exists()}")
if dll_path.exists():
    print(f"DLL size: {dll_path.stat().st_size / 1024:.1f} KB")
print()

# Check for _native.py
native_path = synrix_dir / "_native.py"
print(f"_native.py path: {native_path}")
print(f"_native.py exists: {native_path.exists()}")
print()

# Try to import
print("=== Attempting Import ===")
try:
    # Add current directory to path if needed
    if str(Path(__file__).parent) not in sys.path:
        sys.path.insert(0, str(Path(__file__).parent))
        print(f"Added to sys.path: {Path(__file__).parent}")
    print()
    
    print("Importing synrix._native...")
    from synrix._native import load_synrix
    print("[OK] Import successful")
    print()
    
    print("Calling load_synrix()...")
    lib = load_synrix()
    print("[OK] DLL loaded successfully!")
    print(f"Library object: {lib}")
    print()
    print("=== SUCCESS ===")
    
except ImportError as e:
    print(f"[ERROR] Import failed: {e}")
    print()
    print("Troubleshooting:")
    print("  1. Make sure you're in the synrix_unlimited directory")
    print("  2. Check that synrix/ directory exists")
    print("  3. Check that synrix/_native.py exists")
    
except OSError as e:
    print(f"[ERROR] DLL load failed: {e}")
    print()
    print("Troubleshooting:")
    print("  1. Check that libsynrix.dll is in synrix/ directory")
    print("  2. Check that all MinGW runtime DLLs are present:")
    print("     - libgcc_s_seh-1.dll")
    print("     - libstdc++-6.dll")
    print("     - libwinpthread-1.dll")
    print("  3. Try setting SYNRIX_LIB_PATH environment variable:")
    print(f"     set SYNRIX_LIB_PATH={dll_path}")
    
except Exception as e:
    print(f"[ERROR] Unexpected error: {e}")
    import traceback
    traceback.print_exc()
