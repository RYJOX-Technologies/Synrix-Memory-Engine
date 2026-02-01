#!/usr/bin/env python3
"""
SYNRIX Integration Example - Copy/Paste Ready

This shows exactly how to integrate SYNRIX into your agent.
Copy this code and you're done.
"""

import os
import sys

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
except ImportError:
    # Try direct import
    sys.path.insert(0, os.path.join(parent_dir, 'synrix'))
    from raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING

import json


# ============================================================================
# BEFORE: Agent without persistent memory
# ============================================================================

class AgentWithoutMemory:
    """Your current agent - memory lost on restart"""
    
    def __init__(self):
        self.memory = {}  # ❌ Lost on restart
    
    def remember(self, key: str, value: str):
        self.memory[key] = value  # ❌ Gone when process dies
    
    def recall(self, key: str):
        return self.memory.get(key)  # ❌ Empty after restart


# ============================================================================
# AFTER: Agent with SYNRIX (3 lines changed)
# ============================================================================

class AgentWithSynrix:
    """Your agent with persistent memory - plug in SYNRIX"""
    
    def __init__(self, lattice_path: str = "agent_memory.lattice"):
        # ✅ Line 1: Import (already done above)
        # ✅ Line 2: Initialize backend
        self.backend = RawSynrixBackend(lattice_path)
        # ✅ Line 3: Use it (that's it!)
    
    def remember(self, key: str, value: str, metadata: dict = None):
        """Store in persistent memory"""
        data = json.dumps({
            "value": value,
            "metadata": metadata or {}
        })
        return self.backend.add_node(key, data, node_type=LATTICE_NODE_LEARNING)
    
    def recall(self, key: str):
        """Recall from persistent memory"""
        # O(1) lookup by exact key
        results = self.backend.find_by_prefix(key, limit=1)
        if results:
            return json.loads(results[0]["data"])
        return None
    
    def search(self, prefix: str, limit: int = 10):
        """Search by prefix (semantic queries)"""
        # O(k) semantic search
        results = self.backend.find_by_prefix(prefix, limit=limit)
        return [json.loads(r["data"]) for r in results]
    
    def close(self):
        """Save and close (call before exit)"""
        self.backend.save()
        self.backend.close()


# ============================================================================
# Usage Example
# ============================================================================

if __name__ == "__main__":
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║           SYNRIX Integration Example                          ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    # Create agent with SYNRIX
    agent = AgentWithSynrix("example_agent.lattice")
    
    # Store something
    print("Storing: 'fix:api_error' = 'Use retry logic'")
    agent.remember("fix:api_error", "Use retry logic", {"error_type": "timeout"})
    
    # Recall it
    print("Recalling: 'fix:api_error'")
    result = agent.recall("fix:api_error")
    print(f"  Result: {result}")
    print()
    
    # Search by prefix
    print("Searching: 'fix:'")
    results = agent.search("fix:", limit=5)
    print(f"  Found {len(results)} results")
    print()
    
    # Close (saves to disk)
    agent.close()
    print("✅ Memory saved to disk - survives restart!")
    print()
    print("═══════════════════════════════════════════════════════════════")
    print("  THAT'S IT - 3 LINES TO ADD PERSISTENT MEMORY")
    print("═══════════════════════════════════════════════════════════════")

