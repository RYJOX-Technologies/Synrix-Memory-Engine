#!/usr/bin/env python3
"""Demonstrate querying SYNRIX memory lattice"""

import sys
sys.path.insert(0, 'synrix_unlimited')

from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

print("=" * 60)
print("Querying SYNRIX Knowledge Base")
print("=" * 60)
print()

# Query architecture knowledge
print("Query: 'ARCHITECTURE:'")
results = memory.query('ARCHITECTURE:')
print(f"Found {len(results)} results")
for r in results[:3]:
    print(f"  - {r['name']}")
    print(f"    {r['data'][:150]}...")
    print()

# Query API knowledge
print("Query: 'API:'")
results = memory.query('API:')
print(f"Found {len(results)} results")
for r in results[:2]:
    print(f"  - {r['name']}")
    print(f"    {r['data'][:120]}...")
    print()

# Query Windows implementation
print("Query: 'WINDOWS:'")
results = memory.query('WINDOWS:')
print(f"Found {len(results)} results")
for r in results:
    print(f"  - {r['name']}")
    print(f"    {r['data'][:120]}...")
    print()

# Query implementation details
print("Query: 'IMPLEMENTATION:'")
results = memory.query('IMPLEMENTATION:')
print(f"Found {len(results)} results")
for r in results[:2]:
    print(f"  - {r['name']}")
    print(f"    {r['data'][:120]}...")
    print()

# Query patterns
print("Query: 'PATTERN:'")
results = memory.query('PATTERN:')
print(f"Found {len(results)} results")
for r in results:
    print(f"  - {r['name']}")
    print(f"    {r['data'][:120]}...")
    print()

print("=" * 60)
print(f"Total nodes in knowledge base: {memory.count()}")
print("=" * 60)
