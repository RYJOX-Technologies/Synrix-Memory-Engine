#!/usr/bin/env python3
"""
SYNRIX Growth Demo - Shows How SYNRIX Becomes More Valuable Over Time
======================================================================
This demo shows how SYNRIX memory grows and becomes more valuable
as the AI agent uses it over time.

Run this periodically to see growth:
  python synrix_growth_demo.py
"""

import sys
import os
from pathlib import Path
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from synrix.agent_integration import get_synrix_stats, check_synrix_before_generate
    from synrix.track_growth import get_current_stats, SNAPSHOT_DIR
    SYNRIX_AVAILABLE = True
except ImportError:
    SYNRIX_AVAILABLE = False
    print("ERROR: SYNRIX not available")
    sys.exit(1)


def format_box(title, content_lines):
    """Format content in a box"""
    width = 70
    print("╔" + "═" * (width - 2) + "╗")
    print(f"║ {title:<{width-4}} ║")
    print("╠" + "═" * (width - 2) + "╣")
    for line in content_lines:
        print(f"║ {line:<{width-4}} ║")
    print("╚" + "═" * (width - 2) + "╝")


def show_current_state():
    """Show current SYNRIX state"""
    stats = get_current_stats()
    context = check_synrix_before_generate()
    
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX MEMORY - CURRENT STATE                                  ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    print(f"Timestamp: {stats.get('timestamp', 'N/A')}")
    print()
    
    # Memory counts
    constraints = stats.get('constraints', 0)
    patterns = stats.get('patterns', 0)
    failures = stats.get('failures', 0)
    total = stats.get('total', 0)
    
    print("┌────────────────────────────────────────────────────────────┐")
    print("│  Memory Statistics                                         │")
    print("├────────────────────────────────────────────────────────────┤")
    print(f"│  Constraints: {constraints:>3}  (project rules to follow)              │")
    print(f"│  Patterns:    {patterns:>3}  (successful code patterns)            │")
    print(f"│  Failures:    {failures:>3}  (mistakes to avoid)                  │")
    print(f"│  ──────────────────────────────────────────────────────── │")
    print(f"│  Total:        {total:>3}  (total memory nodes)                  │")
    print("└────────────────────────────────────────────────────────────┘")
    print()
    
    # Show recent items
    if stats.get('constraints_detail'):
        print("Recent Constraints:")
        for c in stats['constraints_detail'][:5]:
            name = c['name']
            data = c['data'][:50]
            print(f"  • {name}: {data}...")
        print()
    
    if stats.get('patterns_detail'):
        print("Recent Patterns:")
        for p in stats['patterns_detail'][:5]:
            name = p['name']
            rate = p['success_rate']
            print(f"  • {name}: {rate:.0%} success rate")
        print()
    
    if stats.get('failures_detail'):
        print("Recent Failures:")
        for f in stats['failures_detail'][:5]:
            name = f['name']
            error = f['error'][:50]
            print(f"  • {name}: {error}...")
        print()
    
    return stats


def show_growth_over_time():
    """Show growth from snapshots"""
    snapshots = sorted(SNAPSHOT_DIR.glob("snapshot_*.json"))
    
    if len(snapshots) < 2:
        print("Not enough snapshots to show growth.")
        print("Take snapshots over time to see growth.")
        print()
        print("To take a snapshot:")
        print("  python python-sdk/synrix/track_growth.py snapshot")
        return
    
    import json
    
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX GROWTH OVER TIME                                        ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    data_points = []
    for snapshot_file in snapshots:
        with open(snapshot_file, 'r') as f:
            data = json.load(f)
            data_points.append(data)
    
    first = data_points[0]
    last = data_points[-1]
    
    print("┌────────────────────────────────────────────────────────────┐")
    print("│  Growth Summary                                            │")
    print("├────────────────────────────────────────────────────────────┤")
    print(f"│  First snapshot:  {first.get('timestamp', 'N/A')[:19]}        │")
    print(f"│  Latest snapshot: {last.get('timestamp', 'N/A')[:19]}        │")
    print(f"│  Snapshots:       {len(data_points)}                                      │")
    print("├────────────────────────────────────────────────────────────┤")
    
    constraints_growth = last.get('constraints', 0) - first.get('constraints', 0)
    patterns_growth = last.get('patterns', 0) - first.get('patterns', 0)
    failures_growth = last.get('failures', 0) - first.get('failures', 0)
    total_growth = last.get('total', 0) - first.get('total', 0)
    
    print(f"│  Constraints: +{constraints_growth:>3}  (project rules learned)          │")
    print(f"│  Patterns:    +{patterns_growth:>3}  (successful patterns stored)      │")
    print(f"│  Failures:    +{failures_growth:>3}  (mistakes learned from)         │")
    print(f"│  ──────────────────────────────────────────────────────── │")
    print(f"│  Total Growth: +{total_growth:>3}  (total memory increase)            │")
    print("└────────────────────────────────────────────────────────────┘")
    print()
    
    # Show value proposition
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  HOW SYNRIX BECOMES MORE VALUABLE OVER TIME                    ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    print("Day 1 (Initial):")
    print("  • 0 constraints → Agent doesn't know your preferences")
    print("  • 0 patterns → Agent can't reuse successful code")
    print("  • 0 failures → Agent may repeat mistakes")
    print()
    print(f"Today ({len(data_points)} snapshots):")
    print(f"  • {last.get('constraints', 0)} constraints → Agent knows your preferences")
    print(f"  • {last.get('patterns', 0)} patterns → Agent can reuse successful code")
    print(f"  • {last.get('failures', 0)} failures → Agent avoids known mistakes")
    print()
    print("Value Increase:")
    print(f"  • {constraints_growth} more constraints = Better accuracy")
    print(f"  • {patterns_growth} more patterns = Faster development")
    print(f"  • {failures_growth} more failures = Fewer mistakes")
    print()
    print("The more you use SYNRIX, the smarter the AI agent becomes!")


def main():
    """Run the growth demo"""
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX GROWTH DEMO                                             ║")
    print("║  How SYNRIX Becomes More Valuable Over Time                    ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    
    if not SYNRIX_AVAILABLE:
        print("\nERROR: SYNRIX not available")
        sys.exit(1)
    
    # Show current state
    stats = show_current_state()
    
    # Show growth
    show_growth_over_time()
    
    print()
    print("=" * 70)
    print("To track growth over time:")
    print("  python python-sdk/synrix/track_growth.py snapshot")
    print()
    print("To view statistics:")
    print("  python python-sdk/synrix/track_growth.py stats")
    print()
    print("To view growth:")
    print("  python python-sdk/synrix/track_growth.py growth")
    print()


if __name__ == "__main__":
    main()
