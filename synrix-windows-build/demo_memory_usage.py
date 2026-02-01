#!/usr/bin/env python3
"""Demonstrate how AI memory helps as a coding assistant"""

import sys
sys.path.insert(0, 'python-sdk')
from synrix.ai_memory import get_ai_memory

m = get_ai_memory()

print("=" * 70)
print("HOW AI MEMORY HELPS AS A CODING ASSISTANT")
print("=" * 70)

# Example 1: Recall critical fixes
print("\n1. RECALLING CRITICAL FIXES:")
print("-" * 70)
results = m.query('FIX:')
for r in results[:2]:
    print(f"  • {r['name']}")
    print(f"    {r['data'][:100]}...")

# Example 2: Check performance expectations
print("\n2. CHECKING PERFORMANCE METRICS:")
print("-" * 70)
results = m.query('PERFORMANCE:')
for r in results[:2]:
    print(f"  • {r['name']}")
    print(f"    {r['data'][:100]}...")

# Example 3: Understand user preferences
print("\n3. RECALLING USER PREFERENCES:")
print("-" * 70)
results = m.query('AI_PREFERENCE:')
for r in results[:2]:
    print(f"  • {r['name']}")
    print(f"    {r['data'][:100]}...")

# Example 4: Find architecture decisions
print("\n4. UNDERSTANDING ARCHITECTURE:")
print("-" * 70)
results = m.query('ARCHITECTURE:')
for r in results[:2]:
    print(f"  • {r['name']}")
    print(f"    {r['data'][:100]}...")

print("\n" + "=" * 70)
print("BENEFITS:")
print("=" * 70)
print("""
+ Context Persistence: Remember project details across sessions
+ Faster Onboarding: Query memory instead of re-reading files
+ Consistency: Recall previous decisions and fixes
+ Reduced Redundancy: Don't ask the same questions twice
+ Project-Specific Knowledge: Store domain knowledge about this codebase
+ Semantic Search: Find relevant info even with partial keywords
+ O(1) Lookups: Instant access to specific memories by ID
+ O(k) Queries: Fast semantic search that scales with results, not data size
""")

# Debug: Show what's actually stored
print("\n" + "=" * 70)
print("DEBUG: All stored memories (first 5):")
print("=" * 70)
all_memories = m.list_all()
for i, r in enumerate(all_memories[:5]):
    print(f"{i+1}. {r['name']}")
    print(f"   Data: {r['data'][:80]}...")
