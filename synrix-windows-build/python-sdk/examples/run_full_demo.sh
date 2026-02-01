#!/bin/bash
# Full Monday Demo - End to End
# Ensures server is running and runs the complete demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║        SYNRIX Agent Memory Demo - Full End-to-End Test         ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Kill any existing server
echo "1. Checking for existing server..."
if pgrep -f synrix_mimic_qdrant > /dev/null; then
    echo "   ⚠️  Existing server found, stopping it..."
    pkill -f synrix_mimic_qdrant
    sleep 2
fi

# Start server
echo "2. Starting SYNRIX server..."
cd ../integrations/qdrant_mimic
if [ ! -f ./synrix_mimic_qdrant ]; then
    echo "   ❌ Server not built. Building..."
    make clean && make
fi

./synrix_mimic_qdrant --port 6334 --lattice-path /tmp/synrix_agent_demo.lattice --max-nodes 100000 > /tmp/synrix_server.log 2>&1 &
SERVER_PID=$!
echo "   Server PID: $SERVER_PID"

# Wait for server to be ready
echo "3. Waiting for server to be ready..."
for i in {1..20}; do
    sleep 0.5
    if curl -s http://localhost:6334/collections > /dev/null 2>&1; then
        echo "   ✅ Server is ready!"
        break
    fi
    if [ $i -eq 20 ]; then
        echo "   ❌ Server failed to start after 10 seconds"
        echo "   Log:"
        tail -20 /tmp/synrix_server.log
        exit 1
    fi
done

# Check shared memory
echo "4. Checking shared memory..."
if [ -f /dev/shm/synrix_qdrant_shm ]; then
    echo "   ✅ Shared memory initialized"
else
    echo "   ⚠️  Shared memory not found (may still be initializing)"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  RUNNING FULL DEMO"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Run the demo
cd "$SCRIPT_DIR/.."
python3 examples/agent_demo_real_server.py

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  OPTIONAL: Run Persistence Demo"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "  To see persistence in action (server restart demo), run:"
echo "    python3 examples/agent_demo_with_persistence.py"
echo ""

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  DEMO COMPLETE"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Server is still running (PID: $SERVER_PID)"
echo "To stop: pkill -f synrix_mimic_qdrant"
echo ""

