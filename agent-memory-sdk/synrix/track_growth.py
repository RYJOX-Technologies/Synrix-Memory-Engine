#!/usr/bin/env python3
"""
SYNRIX Growth Tracker
=====================
Tracks how SYNRIX memory grows over time for demo purposes.

Usage:
    python track_growth.py stats    # Show current stats
    python track_growth.py snapshot # Take a snapshot for demo
"""

import os
import sys
import json
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from synrix.agent_integration import get_synrix_stats, check_synrix_before_generate
    SYNRIX_AVAILABLE = True
except ImportError:
    SYNRIX_AVAILABLE = False


SNAPSHOT_DIR = Path.home() / ".synrix_snapshots"
SNAPSHOT_DIR.mkdir(exist_ok=True)


def get_current_stats():
    """Get current SYNRIX statistics"""
    if not SYNRIX_AVAILABLE:
        return {"error": "SYNRIX not available"}
    
    stats = get_synrix_stats()
    context = check_synrix_before_generate()
    
    # Get detailed breakdown
    constraints_detail = []
    for c in context.get("constraints", [])[:10]:
        constraints_detail.append({
            "name": c.get("name", "").replace("CONSTRAINT:", ""),
            "data": c.get("data", "")[:60]
        })
    
    patterns_detail = []
    for p in context.get("patterns", [])[:10]:
        patterns_detail.append({
            "name": p.get("name", "").replace("PATTERN:", ""),
            "success_rate": p.get("data", {}).get("success_rate", 0.0)
        })
    
    failures_detail = []
    for f in context.get("failures", [])[:10]:
        failures_detail.append({
            "name": f.get("name", "").replace("FAILURE:", ""),
            "error": f.get("data", {}).get("error", "")[:60]
        })
    
    return {
        "timestamp": datetime.now().isoformat(),
        "constraints": stats.get("constraints", 0),
        "patterns": stats.get("patterns", 0),
        "failures": stats.get("failures", 0),
        "total": stats.get("constraints", 0) + stats.get("patterns", 0) + stats.get("failures", 0),
        "constraints_detail": constraints_detail,
        "patterns_detail": patterns_detail,
        "failures_detail": failures_detail
    }


def take_snapshot():
    """Take a snapshot of current SYNRIX state"""
    stats = get_current_stats()
    
    snapshot_file = SNAPSHOT_DIR / f"snapshot_{int(time.time())}.json"
    with open(snapshot_file, 'w') as f:
        json.dump(stats, f, indent=2)
    
    print(f"Snapshot saved: {snapshot_file}")
    return snapshot_file


def show_stats():
    """Show current statistics"""
    stats = get_current_stats()
    
    print("=" * 70)
    print("SYNRIX MEMORY STATISTICS")
    print("=" * 70)
    print()
    print(f"Timestamp: {stats.get('timestamp', 'N/A')}")
    print()
    print(f"Total Memory:")
    print(f"  • Constraints: {stats.get('constraints', 0)}")
    print(f"  • Patterns: {stats.get('patterns', 0)}")
    print(f"  • Failures: {stats.get('failures', 0)}")
    print(f"  • Total: {stats.get('total', 0)}")
    print()
    
    if stats.get('constraints_detail'):
        print("Recent Constraints:")
        for c in stats['constraints_detail'][:5]:
            print(f"  • {c['name']}: {c['data']}...")
        print()
    
    if stats.get('patterns_detail'):
        print("Recent Patterns:")
        for p in stats['patterns_detail'][:5]:
            print(f"  • {p['name']}: {p['success_rate']:.0%} success rate")
        print()
    
    if stats.get('failures_detail'):
        print("Recent Failures:")
        for f in stats['failures_detail'][:5]:
            print(f"  • {f['name']}: {f['error']}...")
        print()


def show_growth():
    """Show growth over time from snapshots"""
    snapshots = sorted(SNAPSHOT_DIR.glob("snapshot_*.json"))
    
    if not snapshots:
        print("No snapshots found. Take a snapshot first with: python track_growth.py snapshot")
        return
    
    print("=" * 70)
    print("SYNRIX GROWTH OVER TIME")
    print("=" * 70)
    print()
    
    data_points = []
    for snapshot_file in snapshots:
        with open(snapshot_file, 'r') as f:
            data = json.load(f)
            data_points.append(data)
    
    if len(data_points) < 2:
        print("Need at least 2 snapshots to show growth")
        return
    
    first = data_points[0]
    last = data_points[-1]
    
    print(f"First snapshot: {first.get('timestamp', 'N/A')}")
    print(f"  Constraints: {first.get('constraints', 0)}")
    print(f"  Patterns: {first.get('patterns', 0)}")
    print(f"  Failures: {first.get('failures', 0)}")
    print(f"  Total: {first.get('total', 0)}")
    print()
    
    print(f"Latest snapshot: {last.get('timestamp', 'N/A')}")
    print(f"  Constraints: {last.get('constraints', 0)}")
    print(f"  Patterns: {last.get('patterns', 0)}")
    print(f"  Failures: {last.get('failures', 0)}")
    print(f"  Total: {last.get('total', 0)}")
    print()
    
    print("Growth:")
    print(f"  Constraints: +{last.get('constraints', 0) - first.get('constraints', 0)}")
    print(f"  Patterns: +{last.get('patterns', 0) - first.get('patterns', 0)}")
    print(f"  Failures: +{last.get('failures', 0) - first.get('failures', 0)}")
    print(f"  Total: +{last.get('total', 0) - first.get('total', 0)}")
    print()
    
    print(f"Total snapshots: {len(data_points)}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        show_stats()
    elif sys.argv[1] == "stats":
        show_stats()
    elif sys.argv[1] == "snapshot":
        take_snapshot()
    elif sys.argv[1] == "growth":
        show_growth()
    else:
        print("Usage: track_growth.py [stats|snapshot|growth]")
        sys.exit(1)
