#!/usr/bin/env python3
"""
Pre-load SYNRIX Lattice with Demo Data

Loads the lattice with sample data to demonstrate SYNRIX capabilities:
- User preferences and facts
- Past conversations
- Learned patterns
- Domain knowledge
"""

import sys
import os
import json
import time

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
    RAW_BACKEND_AVAILABLE = True
except ImportError:
    print("❌ RawSynrixBackend not available")
    sys.exit(1)


def preload_demo_data(lattice_path: str = "demo_data.lattice"):
    """Pre-load lattice with demo data"""
    backend = RawSynrixBackend(lattice_path)
    
    print("Loading demo data into SYNRIX...")
    print()
    
    # 1. User Preferences (Episodic Memory)
    print("1. Storing user preferences...")
    preferences = [
        ("favorite_color", "blue"),
        ("favorite_food", "pizza"),
        ("favorite_programming_language", "Python"),
        ("name", "Alice"),
        ("location", "San Francisco"),
        ("occupation", "software engineer"),
    ]
    
    for key, value in preferences:
        backend.add_node(f"episodic:{key}", value, node_type=LATTICE_NODE_LEARNING)
        print(f"   ✓ {key}: {value}")
    
    print()
    
    # 2. Past Conversations
    print("2. Storing past conversations...")
    conversations = [
        {
            "user": "What's the weather like?",
            "assistant": "It's sunny and 72°F in San Francisco today.",
            "timestamp": time.time() - 3600  # 1 hour ago
        },
        {
            "user": "What programming languages should I learn?",
            "assistant": "Python is great for beginners and widely used. JavaScript is essential for web development.",
            "timestamp": time.time() - 7200  # 2 hours ago
        },
        {
            "user": "Tell me about SYNRIX",
            "assistant": "SYNRIX is a knowledge graph system that provides persistent, fast memory for AI systems with sub-microsecond lookups.",
            "timestamp": time.time() - 10800  # 3 hours ago
        },
    ]
    
    for i, conv in enumerate(conversations):
        key = f"conversation:demo:{int(conv['timestamp'])}"
        data = json.dumps(conv)
        backend.add_node(key, data, node_type=LATTICE_NODE_LEARNING)
        print(f"   ✓ Conversation {i+1}: {conv['user'][:50]}...")
    
    print()
    
    # 3. Learned Patterns (Code/Error Fixes)
    print("3. Storing learned patterns...")
    patterns = [
        ("error:format_specifier", "Fix: Use correct format specifier (e.g., %d for int, %s for string)"),
        ("error:unused_variable", "Fix: Remove unused variable or use it in code"),
        ("error:import_error", "Fix: Check import path and ensure module is installed"),
        ("pattern:file_io", "Pattern: Use open(), read(), write(), close() for file operations"),
        ("pattern:list_comprehension", "Pattern: Use [x for x in iterable if condition] for efficient list operations"),
    ]
    
    for key, value in patterns:
        backend.add_node(f"pattern:{key}", value, node_type=LATTICE_NODE_LEARNING)
        print(f"   ✓ Pattern: {key}")
    
    print()
    
    # 4. Domain Knowledge (Facts)
    print("4. Storing domain knowledge...")
    facts = [
        ("fact:synrix_latency", "SYNRIX provides sub-microsecond (186ns) lookups"),
        ("fact:synrix_scale", "SYNRIX can handle 50+ million nodes on Jetson Orin Nano"),
        ("fact:synrix_persistence", "SYNRIX data persists across system restarts with WAL recovery"),
        ("fact:python_version", "Python 3.10.12 is installed on this system"),
        ("fact:jetson_model", "Jetson Orin Nano with 8GB RAM"),
    ]
    
    for key, value in facts:
        backend.add_node(f"fact:{key}", value, node_type=LATTICE_NODE_LEARNING)
        print(f"   ✓ Fact: {key}")
    
    print()
    
    # 5. Task History (What user has done)
    print("5. Storing task history...")
    tasks = [
        ("task:created_project", "Created a new Python project for SYNRIX integration"),
        ("task:built_llama", "Built llama.cpp from source on Jetson"),
        ("task:downloaded_model", "Downloaded Qwen3-0.6B Q5_K_M model (424MB)"),
        ("task:tested_integration", "Tested LLM + SYNRIX integration successfully"),
    ]
    
    for key, value in tasks:
        backend.add_node(f"task:{key}", value, node_type=LATTICE_NODE_LEARNING)
        print(f"   ✓ Task: {key}")
    
    print()
    
    # Get stats
    all_nodes = backend.find_by_prefix("", limit=1000)
    print(f"✅ Loaded {len(all_nodes)} nodes into SYNRIX")
    print()
    
    backend.close()
    return lattice_path


if __name__ == "__main__":
    lattice_path = preload_demo_data()
    print(f"Demo data loaded into: {lattice_path}")
    print()
    print("Ready for testing!")

