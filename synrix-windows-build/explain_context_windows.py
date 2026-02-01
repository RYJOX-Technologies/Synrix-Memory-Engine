#!/usr/bin/env python3
"""Explain how SYNRIX works with context windows"""

import sys
sys.path.insert(0, 'synrix_unlimited')

from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

print("=" * 70)
print("SYNRIX and Context Windows: How It Works")
print("=" * 70)
print()

# Query tokenless concept
print("1. SYNRIX IS TOKENLESS")
print("-" * 70)
results = memory.query('CONCEPT:tokenless')
for r in results:
    print(r['data'])
print()

# Query indexing
print("2. O(k) QUERY SYSTEM (Selective Retrieval)")
print("-" * 70)
results = memory.query('IMPLEMENTATION:indexing')
for r in results:
    print(r['data'])
print()

# Query performance
print("3. PERFORMANCE CHARACTERISTICS")
print("-" * 70)
results = memory.query('PERF:scaling')
for r in results:
    print(r['data'])
print()

print("=" * 70)
print("KEY DIFFERENCE FROM TRADITIONAL LLMs")
print("=" * 70)
print()
print("Traditional LLM Context Window:")
print("  - Fixed size (e.g., 128k tokens)")
print("  - Everything must fit in context")
print("  - Expensive to include all data")
print("  - Limited by token count")
print()
print("SYNRIX Approach:")
print("  - Tokenless: No tokenization overhead")
print("  - O(k) queries: Only retrieve what you need")
print("  - Persistent storage: Data lives in .lattice file")
print("  - Selective loading: Query specific prefixes/categories")
print("  - No context window limit: Store millions of nodes")
print()
print("Example:")
print("  Instead of loading 148 nodes into context (expensive),")
print("  I query 'ARCHITECTURE:' and get only 12 relevant nodes.")
print("  Then query 'API:' for 6 more nodes when needed.")
print("  Total: Only 18 nodes loaded, not all 148!")
print()
