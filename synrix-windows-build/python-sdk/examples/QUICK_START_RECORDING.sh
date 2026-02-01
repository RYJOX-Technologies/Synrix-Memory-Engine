#!/bin/bash
# Quick Start for Recording - Sets up everything and runs demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     Morphos Demo - Quick Start for Recording                   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check prerequisites
echo "Checking prerequisites..."
if [ ! -f "../libsynrix.so" ]; then
    echo "❌ libsynrix.so not found!"
    echo "   Build it with: cd .. && ./build_raw_backend.sh"
    exit 1
fi
echo "✅ libsynrix.so found"

if [ ! -f "agent_demo_REAL.py" ]; then
    echo "❌ agent_demo_REAL.py not found!"
    exit 1
fi
echo "✅ agent_demo_REAL.py found"

if [ ! -f "INTEGRATION_EXAMPLE.py" ]; then
    echo "❌ INTEGRATION_EXAMPLE.py not found!"
    exit 1
fi
echo "✅ INTEGRATION_EXAMPLE.py found"

echo ""
echo "✅ All prerequisites met!"
echo ""

# Set library path
export LD_LIBRARY_PATH="$SCRIPT_DIR/..:$LD_LIBRARY_PATH"
echo "✅ LD_LIBRARY_PATH set"
echo ""

# Clear screen
clear

echo "═══════════════════════════════════════════════════════════════"
echo "  READY TO RECORD"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "  1. Start your screen recorder"
echo "  2. Press Enter to run the demo"
echo ""
read -p "Press Enter when ready..."

clear

# Run demo
echo "Running demo..."
echo ""
./morphos_demo_60sec.sh

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  DEMO COMPLETE"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Stop your screen recorder."
echo ""

