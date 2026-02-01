#!/bin/bash
# Start SYNRIX Server for Agent Demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SERVER_DIR="$ROOT_DIR/integrations/qdrant_mimic"
LATTICE_PATH="/tmp/synrix_agent_demo.lattice"
SERVER_LOG="/tmp/synrix_server.log"

echo "Starting SYNRIX server for agent demo..."
echo ""

# Check if server exists
if [ ! -f "$SERVER_DIR/synrix_mimic_qdrant" ]; then
    echo "Building SYNRIX server..."
    cd "$SERVER_DIR"
    if [ -f Makefile ]; then
        make synrix_mimic_qdrant 2>&1 | tail -20
    else
        echo "❌ Makefile not found in $SERVER_DIR"
        exit 1
    fi
fi

# Kill any existing server
if pgrep -f "synrix_mimic_qdrant.*6334" > /dev/null; then
    echo "Stopping existing server..."
    pkill -f "synrix_mimic_qdrant.*6334"
    sleep 1
fi

# Clean up old lattice for fresh start
if [ -f "$LATTICE_PATH" ]; then
    echo "Removing old lattice file..."
    rm -f "$LATTICE_PATH" "$LATTICE_PATH.wal" 2>/dev/null || true
fi

# Start server
echo "Starting server on port 6334..."
cd "$SERVER_DIR"
./synrix_mimic_qdrant --port 6334 \
    --lattice-path "$LATTICE_PATH" \
    --max-nodes 100000 \
    > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!

# Wait for server to be ready
echo "Waiting for server to initialize..."
for i in {1..30}; do
    sleep 0.5
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "❌ Server failed to start"
        cat "$SERVER_LOG" | tail -20
        exit 1
    fi
    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:6334/collections 2>/dev/null || echo "000")
    if [ "$HTTP_CODE" = "200" ]; then
        echo "✅ Server ready (PID: $SERVER_PID)"
        echo "   Lattice: $LATTICE_PATH"
        echo "   Log: $SERVER_LOG"
        echo ""
        echo "Server is running. Press Ctrl+C to stop."
        echo ""
        # Keep server running
        wait $SERVER_PID
        exit 0
    fi
done

echo "⚠️  Server may still be initializing (check $SERVER_LOG)"
echo "   PID: $SERVER_PID"
echo ""
echo "To stop: kill $SERVER_PID"

