#!/usr/bin/env python3
"""
Cursor AI Agent - SYNRIX Integration
====================================
Use SYNRIX as persistent memory for AI coding agents.

This script shows how I (Cursor AI) can use SYNRIX to:
1. Remember code patterns that work
2. Store project constraints
3. Learn from successful/failed attempts
4. Query similar solutions instantly

Usage:
    # In your Cursor rules or agent code:
    from synrix.agent_backend import get_synrix_backend
    
    backend = get_synrix_backend()
    backend.write("pattern:async_handler", {"code": "...", "success": True})
    results = backend.query_prefix("pattern:")
"""

import os
import sys
import json

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.agent_backend import get_synrix_backend, SynrixAgentBackend
    from synrix.raw_backend import RawSynrixBackend
    SDK_AVAILABLE = True
except ImportError:
    SDK_AVAILABLE = False
    print("⚠️  SYNRIX SDK not found. Install with: pip install -e .")
    sys.exit(1)


def demo_agent_memory():
    """Demonstrate how I can use SYNRIX for persistent memory"""
    
    print("=" * 70)
    print("Cursor AI Agent - SYNRIX Integration Demo")
    print("=" * 70)
    print()
    
    # Option 1: Use agent backend (auto-detects best option)
    print("1. Initializing SYNRIX backend...")
    try:
        # Try direct backend first (uses lattice file directly, no server needed)
        lattice_path = os.path.expanduser("~/.cursor_ai_memory.lattice")
        backend = RawSynrixBackend(lattice_path, max_nodes=100000)
        print(f"   ✅ Using direct lattice backend: {lattice_path}")
        print(f"   💡 No server needed - direct file access!")
    except Exception as e:
        print(f"   ⚠️  Direct backend failed: {e}")
        print("   Trying agent backend (requires server)...")
        backend = get_synrix_backend(use_direct=True, use_mock=False)
        print(f"   ✅ Using agent backend: {backend.backend_type}")
    
    print()
    
    # 2. Store a code pattern I learned
    print("2. Storing a code pattern I learned...")
    pattern_code = """
def handle_async_request(req):
    try:
        result = await process_request(req)
        return {"success": True, "data": result}
    except Exception as e:
        return {"success": False, "error": str(e)}
"""
    pattern_id = backend.add_node(
        name="PATTERN:async_http_handler",
        data=json.dumps({
            "code": pattern_code,
            "context": "HTTP server async handlers",
            "success_rate": 0.95,
            "last_used": "2025-01-07"
        }),
        node_type=3  # LATTICE_NODE_PATTERN
    )
    print(f"   ✅ Stored pattern (ID: {pattern_id})")
    print()
    
    # 3. Store project constraints
    print("3. Storing project constraints...")
    constraints = [
        ("CONSTRAINT:no_regex", "User prefers semantic reasoning over regex"),
        ("CONSTRAINT:300_line_limit", "Source files cannot exceed 300 lines"),
        ("CONSTRAINT:arm64_optimized", "Target: Jetson Orin Nano ARM64"),
    ]
    for name, data in constraints:
        cid = backend.add_node(name, data, node_type=6)  # LATTICE_NODE_ANTI_PATTERN
        print(f"   ✅ Stored: {name}")
    print()
    
    # 4. Query patterns I've learned
    print("4. Querying patterns I've learned...")
    patterns = backend.find_by_prefix("PATTERN:", limit=10)
    print(f"   ✅ Found {len(patterns)} patterns:")
    for p in patterns:
        name = p.get('name', 'unknown')
        print(f"      • {name}")
    print()
    
    # 5. Query constraints
    print("5. Querying project constraints...")
    constraints_found = backend.find_by_prefix("CONSTRAINT:", limit=10)
    print(f"   ✅ Found {len(constraints_found)} constraints:")
    for c in constraints_found:
        name = c.get('name', 'unknown')
        data = c.get('data', '')
        print(f"      • {name}: {data[:50]}...")
    print()
    
    # 6. Store a successful code generation
    print("6. Storing successful code generation...")
    success_id = backend.add_node(
        name="TASK:fix_bug:2025-01-07:001",
        data=json.dumps({
            "task": "Fix syntax error in async handler",
            "solution": "Added missing colon after try statement",
            "success": True,
            "timestamp": "2025-01-07T12:00:00Z"
        }),
        node_type=5  # LATTICE_NODE_LEARNING
    )
    print(f"   ✅ Stored success (ID: {success_id})")
    print()
    
    # 7. Query similar tasks
    print("7. Querying similar tasks...")
    tasks = backend.find_by_prefix("TASK:", limit=5)
    print(f"   ✅ Found {len(tasks)} tasks:")
    for t in tasks:
        name = t.get('name', 'unknown')
        print(f"      • {name}")
    print()
    
    print("=" * 70)
    print("✅ Integration complete!")
    print()
    print("How to use in Cursor:")
    print("  1. Import: from synrix.raw_backend import RawSynrixBackend")
    print("  2. Initialize: backend = RawSynrixBackend('~/.cursor_ai_memory.lattice')")
    print("  3. Store: backend.add_node('PATTERN:name', code_json)")
    print("  4. Query: backend.find_by_prefix('PATTERN:')")
    print()
    print("💡 The lattice file persists across sessions!")
    print("   Next time I load, I'll remember all these patterns.")
    print("=" * 70)
    
    backend.close()


if __name__ == "__main__":
    demo_agent_memory()
