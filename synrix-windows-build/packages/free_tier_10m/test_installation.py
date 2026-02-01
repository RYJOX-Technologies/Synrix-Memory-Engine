#!/usr/bin/env python3
"""
Quick test to verify SYNRIX installation works
Run this after installing to confirm everything is set up correctly
"""

import os
from pathlib import Path

print("=" * 60)
print("  SYNRIX Installation Test")
print("=" * 60)
print()

try:
    print("1. Testing import...")
    from synrix.ai_memory import get_ai_memory
    print("   [OK] Import successful!")
    print()
    
    print("2. Testing memory creation...")
    package_dir = Path(__file__).parent
    lattice_path = package_dir / "test_installation.lattice"
    memory = get_ai_memory(lattice_path=str(lattice_path))
    print("   [OK] Memory created!")
    print()
    
    print("3. Testing add operation...")
    memory.add("TEST:installation", "SYNRIX is working!")
    print("   [OK] Added test node!")
    print()
    
    print("4. Testing query operation...")
    results = memory.query("TEST:")
    print(f"   [OK] Query successful! Found {len(results)} node(s)")
    print()
    
    print("5. Testing count operation...")
    count = memory.count()
    print(f"   [OK] Count successful! Total nodes: {count}")
    print()
    
    print("=" * 60)
    print("  [OK] ALL TESTS PASSED - SYNRIX IS WORKING!")
    print("=" * 60)
    print()
    print("You can now use SYNRIX in your projects:")
    print()
    print("  from synrix.ai_memory import get_ai_memory")
    print("  memory = get_ai_memory()")
    print("  memory.add('Name', 'Data')")
    print()
    
except ImportError as e:
    print("   [ERROR] Import failed!")
    print(f"   Error: {e}")
    print()
    print("   Make sure you ran: pip install -e .")
    print("   (with the dot at the end!)")
    print()
    exit(1)
    
except Exception as e:
    print("   [ERROR] Test failed!")
    print(f"   Error: {e}")
    print()
    print("   See IF_YOU_HAVE_ISSUES.md for troubleshooting")
    print()
    exit(1)
