#!/usr/bin/env python3
"""
AI Agent + SYNRIX Integration Demo
===================================
This demo shows how SYNRIX makes the AI agent smarter over time by:
1. Storing project constraints and patterns
2. Retrieving them before code generation
3. Learning from successes and failures
4. Building persistent memory across sessions
"""

import sys
import os
import json
from pathlib import Path

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

try:
    from synrix.auto_memory import AIMemoryHelper
    SYNRIX_AVAILABLE = True
except ImportError:
    SYNRIX_AVAILABLE = False
    print("❌ SYNRIX not available. Install with: pip install -e .")
    sys.exit(1)


def print_section(title: str):
    """Print a formatted section header"""
    print()
    print("=" * 70)
    print(f"  {title}")
    print("=" * 70)
    print()


def demo_before_generation():
    """Demo: What the AI agent checks before generating code"""
    print_section("BEFORE CODE GENERATION: Checking SYNRIX Memory")
    
    memory = AIMemoryHelper()
    context = memory.check_before_generate()
    
    print(f"📋 Found {len(context['constraints'])} constraints:")
    for c in context['constraints']:
        name = c['name'].replace('CONSTRAINT:', '')
        print(f"   • {name}: {c['data'][:60]}...")
    
    print(f"\n💡 Found {len(context['patterns'])} patterns:")
    for p in context['patterns']:
        name = p['name'].replace('PATTERN:', '')
        success_rate = p.get('data', {}).get('success_rate', 0.0)
        print(f"   • {name} (success rate: {success_rate:.0%})")
    
    print(f"\n⚠️  Found {len(context['failures'])} failures to avoid:")
    for f in context['failures']:
        name = f['name'].replace('FAILURE:', '')
        error = f.get('data', {}).get('error', '')[:60]
        print(f"   • {name}: {error}...")
    
    print("\n✅ AI Agent will now:")
    print("   1. Follow all constraints")
    print("   2. Reuse successful patterns")
    print("   3. Avoid known failure patterns")
    print("   4. Generate code that matches your project style")


def demo_after_success():
    """Demo: What the AI agent stores after successful code generation"""
    print_section("AFTER SUCCESSFUL CODE GENERATION: Storing Pattern")
    
    memory = AIMemoryHelper()
    
    # Example: Store a successful pattern
    pattern_code = """
def lattice_query_prefix(lattice, prefix: str, limit: int = 100):
    \"\"\"Query lattice by prefix - O(k) performance\"\"\"
    results = []
    for node in lattice.nodes:
        if node.name.startswith(prefix):
            results.append(node)
            if len(results) >= limit:
                break
    return results
"""
    
    node_id = memory.store_pattern(
        pattern_name="lattice_prefix_query",
        code=pattern_code,
        context="SYNRIX lattice query operations",
        success_rate=0.95
    )
    
    print(f"✅ Stored pattern: lattice_prefix_query")
    print(f"   Node ID: {node_id}")
    print(f"   Success rate: 95%")
    print(f"   Context: SYNRIX lattice query operations")
    print("\n💡 This pattern will be reused in future code generation!")


def demo_after_failure():
    """Demo: What the AI agent stores after a failure"""
    print_section("AFTER FAILURE: Learning from Mistakes")
    
    memory = AIMemoryHelper()
    
    # Example: Store a failure
    node_id = memory.store_failure(
        error_type="regex_approach",
        error="User explicitly rejected regex-based pattern matching",
        context="Codebase ingestion tool",
        avoid="Use semantic reasoning or AST parsing instead of regex"
    )
    
    print(f"✅ Stored failure: regex_approach")
    print(f"   Node ID: {node_id}")
    print(f"   Error: User explicitly rejected regex-based pattern matching")
    print(f"   Context: Codebase ingestion tool")
    print(f"   Avoid: Use semantic reasoning or AST parsing instead of regex")
    print("\n💡 AI Agent will avoid this approach in future!")


def demo_constraint_storage():
    """Demo: Storing project constraints"""
    print_section("STORING PROJECT CONSTRAINTS")
    
    memory = AIMemoryHelper()
    
    constraints = [
        ("kg_driven_architecture", "Knowledge graph is single source of truth. Synthesizer is thin layer."),
        ("no_regex", "User prefers semantic reasoning over regex processing"),
        ("300_line_limit", "Source files cannot exceed 300 lines (bare metal OS constraint)"),
        ("arm64_optimized", "Code optimized for ARM64, targeting Jetson Orin Nano"),
        ("stability_first", "Priority: Stability → Accuracy → Speed"),
    ]
    
    print("Storing key project constraints...")
    for name, description in constraints:
        node_id = memory.store_constraint(name, description)
        print(f"   ✅ {name} (node_id: {node_id})")
    
    print(f"\n💡 {len(constraints)} constraints stored. AI Agent will follow these in all future code generation!")


def demo_persistence():
    """Demo: Show that memory persists across sessions"""
    print_section("MEMORY PERSISTENCE: Survives Across Sessions")
    
    memory = AIMemoryHelper()
    context = memory.check_before_generate()
    
    print(f"📊 Current Memory State:")
    print(f"   • Constraints: {len(context['constraints'])}")
    print(f"   • Patterns: {len(context['patterns'])}")
    print(f"   • Failures: {len(context['failures'])}")
    print(f"\n💾 Memory file: ~/.cursor_ai_memory.lattice")
    print(f"   • Persists across AI agent sessions")
    print(f"   • Survives system reboots")
    print(f"   • Grows smarter over time")
    print(f"\n🚀 The AI Agent gets better with each interaction!")


def main():
    """Run the full demo"""
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  AI AGENT + SYNRIX INTEGRATION DEMO                          ║")
    print("║  How SYNRIX Makes the AI Agent Smarter Over Time            ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    
    if not SYNRIX_AVAILABLE:
        print("\n❌ SYNRIX not available. Install with: pip install -e .")
        sys.exit(1)
    
    # Run demos
    demo_before_generation()
    demo_constraint_storage()
    demo_after_success()
    demo_after_failure()
    demo_persistence()
    
    print_section("SUMMARY: How SYNRIX Makes the AI Agent Smarter")
    print("""
1. BEFORE CODE GENERATION:
   • AI Agent queries SYNRIX for constraints, patterns, failures
   • Follows project rules automatically
   • Reuses successful patterns
   • Avoids known failure patterns

2. AFTER SUCCESS:
   • Stores successful code patterns
   • Tracks success rates
   • Builds reusable code library

3. AFTER FAILURE:
   • Stores failures to avoid repeating
   • Learns from mistakes
   • Improves over time

4. PERSISTENCE:
   • Memory survives across sessions
   • Gets smarter with each interaction
   • Personalized to your codebase

RESULT: The AI Agent becomes more accurate, faster, and better
        aligned with your project over time!
    """)
    
    print("\n✅ Demo complete! The AI Agent is now integrated with SYNRIX.")
    print("   Try asking the AI Agent to generate code - it will use this memory!")


if __name__ == "__main__":
    main()
