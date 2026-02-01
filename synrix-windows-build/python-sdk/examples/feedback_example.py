#!/usr/bin/env python3
"""
SYNRIX Feedback and Telemetry Example

Demonstrates how to:
- Enable telemetry collection
- Record performance metrics
- Submit feedback with hardware information
"""

# Try to import synrix - works if installed via pip
try:
    from synrix import SynrixClient, SynrixMockClient
    from synrix.telemetry import enable_telemetry, get_telemetry, TelemetryCollector
    from synrix.feedback import submit_feedback
except ImportError:
    # If not installed, try importing from parent directory (repo layout)
    import sys
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)  # python-sdk directory
    if parent_dir not in sys.path:
        sys.path.insert(0, parent_dir)
    from synrix import SynrixClient, SynrixMockClient
    from synrix.telemetry import enable_telemetry, get_telemetry, TelemetryCollector
    from synrix.feedback import submit_feedback


def main():
    print("=" * 60)
    print("SYNRIX Feedback and Telemetry Example")
    print("=" * 60)
    print()
    
    # Enable telemetry (opt-in)
    print("1. Enabling telemetry collection (opt-in)...")
    enable_telemetry()
    print("   ✅ Telemetry enabled")
    print()
    
    # Use mock client for this example
    print("2. Using mock client (no server required)...")
    client = SynrixMockClient()
    print("   ✅ Mock client ready")
    print()
    
    # Perform some operations (telemetry will be recorded automatically)
    print("3. Performing operations (telemetry recorded automatically)...")
    client.create_collection("test")
    client.add_node("ISA_ADD", "Addition", collection="test")
    client.add_node("ISA_SUBTRACT", "Subtraction", collection="test")
    results = client.query_prefix("ISA_", collection="test")
    print(f"   ✅ Operations completed ({len(results)} nodes found)")
    print()
    
    # View telemetry summary
    print("4. Telemetry Summary:")
    telemetry = get_telemetry()
    if telemetry:
        summary = telemetry.get_telemetry_summary()
        print(f"   Hardware: {summary['hardware']['platform']} {summary['hardware']['architecture']}")
        print(f"   Python: {summary['hardware']['python_version']}")
        if 'cpu_count' in summary['hardware']:
            print(f"   CPU: {summary['hardware']['cpu_count']} cores")
        if 'ram_total_gb' in summary['hardware']:
            print(f"   RAM: {summary['hardware']['ram_total_gb']} GB")
        print(f"   Operations recorded: {summary['operations']['total']}")
        if summary['operations']['latency_stats']:
            print("   Performance metrics:")
            for op, stats in summary['operations']['latency_stats'].items():
                print(f"     {op}: avg {stats['avg_ms']:.2f}ms, p95 {stats['p95_ms']:.2f}ms")
    print()
    
    # Submit feedback
    print("5. Submitting feedback (example)...")
    print("   Note: This is a demo. In production, you would:")
    print("   - Submit via GitHub issues")
    print("   - Submit via API endpoint")
    print("   - Export to JSON file")
    print()
    
    feedback_text = "SYNRIX works great on my Jetson Orin Nano! Fast and reliable."
    result = submit_feedback(
        feedback=feedback_text,
        email=None,  # Optional
        include_telemetry=True,
        method="export"  # Options: "github", "api", "export"
    )
    
    print(f"   Status: {result['status']}")
    print(f"   Message: {result['message']}")
    if 'url' in result:
        print(f"   URL: {result['url']}")
    if 'filename' in result:
        print(f"   File: {result['filename']}")
    print()
    
    print("=" * 60)
    print("✅ Feedback and telemetry example complete!")
    print("=" * 60)
    print()
    print("To enable telemetry in your code:")
    print("  from synrix.telemetry import enable_telemetry")
    print("  enable_telemetry()")
    print()
    print("To submit feedback:")
    print("  from synrix.feedback import submit_feedback")
    print("  submit_feedback('Your feedback here', method='github')")


if __name__ == "__main__":
    main()

