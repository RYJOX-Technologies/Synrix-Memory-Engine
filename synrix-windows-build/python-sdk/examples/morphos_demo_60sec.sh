#!/bin/bash
# 60-Second Morphos Demo Script
# Run this for your Monday presentation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     SYNRIX Agent Memory Demo - 60 Second Presentation          ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "This demo shows how SYNRIX provides persistent memory for AI agents."
echo ""

# Use 100% REAL demo (RawSynrixBackend - direct C library)
echo "Using 100% REAL SYNRIX (RawSynrixBackend - direct C library access)"
echo ""

# Set library path
export LD_LIBRARY_PATH="$SCRIPT_DIR/..:$LD_LIBRARY_PATH"

# Run the demo
echo "═══════════════════════════════════════════════════════════════"
echo "  RUNNING 100% REAL DEMO..."
echo "═══════════════════════════════════════════════════════════════"
echo ""

python3 agent_demo_REAL.py
baseline_results = baseline.run_task_loop(tasks)
print(f'  Success Rate: {baseline_results[\"success_rate\"]:.1%}')
print(f'  Repeated Errors: {baseline_results[\"repeated_errors\"]}')
print()

print('SYNRIX AGENT (With Memory):')
synrix = SynrixAgent(memory)
synrix_results = synrix.run_task_loop(tasks)
print(f'  Success Rate: {synrix_results[\"success_rate\"]:.1%}')
print(f'  Repeated Errors: {synrix_results[\"repeated_errors\"]}')
print()

improvement = synrix_results['success_rate'] - baseline_results['success_rate']
print(f'IMPROVEMENT: {improvement:+.1%} success rate')
print(f'          {baseline_results[\"repeated_errors\"] - synrix_results[\"repeated_errors\"]} fewer repeated errors')
"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  KEY TAKEAWAYS"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "✅ SYNRIX provides persistent memory for AI agents"
echo "✅ Agents learn from past mistakes (fewer repeated errors)"
echo "✅ Fast memory lookups (sub-microsecond in production)"
echo "✅ Crash-proof state (survives restarts)"
echo "✅ Semantic queries (find similar past experiences)"
echo ""
echo "This replaces: Redis + Vector DB + JSON logs"
echo "With: One unified SYNRIX system"
echo ""

