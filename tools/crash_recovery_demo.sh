#!/bin/bash
# Crash Recovery Demo - Shows WAL recovery in action
# This creates a good screenshot for the pitch deck

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

echo "=========================================="
echo "SYNRIX Crash Recovery Demo"
echo "=========================================="
echo ""
echo "This demo will:"
echo "  1. Write 500 nodes with WAL enabled"
echo "  2. Crash at node 500 (simulated)"
echo "  3. Recover from WAL and show nodes recovered"
echo ""

# Clean up old test files
rm -f /tmp/aion_crash_tests/crash_test.lattice
rm -f /tmp/aion_crash_tests/crash_test.lattice.wal

echo "Step 1: Writing nodes with WAL (will crash at node 500)..."
echo "------------------------------------------------------------"
./tools/crash_test 1 || true  # Expected to crash (exit code != 0)

echo ""
echo "Step 2: Recovering from WAL..."
echo "------------------------------------------------------------"
./tools/crash_test 10

echo ""
echo "=========================================="
echo "✅ Crash Recovery Demo Complete"
echo "=========================================="


