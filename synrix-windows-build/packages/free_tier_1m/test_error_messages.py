#!/usr/bin/env python3
"""
Test that error messages include helpful dependency information
"""

import sys
import os
from pathlib import Path

# Add synrix to path
sys.path.insert(0, str(Path(__file__).parent))

# Remove SYNRIX_LIB_PATH to force error
os.environ.pop('SYNRIX_LIB_PATH', None)

try:
    from synrix.raw_backend import RawSynrixBackend
    backend = RawSynrixBackend('test.lattice')
except RuntimeError as e:
    msg = str(e)
    print("=== Error Message Analysis ===")
    print()
    print("Error message includes:")
    print(f"  - VC++ Runtime mentioned: {'Visual C++' in msg or 'VC++' in msg}")
    print(f"  - zlib mentioned: {'zlib' in msg.lower()}")
    print(f"  - One-click fix mentioned: {'installer_v2' in msg or 'install_vc2013' in msg}")
    print()
    print("Full error message:")
    print("-" * 70)
    print(msg)
    print("-" * 70)
    
    # Check if all helpful info is present
    has_vc = 'Visual C++' in msg or 'VC++' in msg
    has_zlib = 'zlib' in msg.lower()
    has_fix = 'installer_v2' in msg or 'install_vc2013' in msg
    
    if has_vc and has_zlib and has_fix:
        print()
        print("[OK] Error message includes all helpful information!")
        sys.exit(0)
    else:
        print()
        print("[WARN] Error message missing some helpful information:")
        if not has_vc:
            print("  - Missing VC++ Runtime mention")
        if not has_zlib:
            print("  - Missing zlib mention")
        if not has_fix:
            print("  - Missing one-click fix mention")
        sys.exit(1)
except Exception as e:
    print(f"[ERROR] Unexpected error: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
