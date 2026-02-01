#!/bin/bash
# Quick Start Script for SYNRIX
# Run this to start SYNRIX and show how to use it

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SYNRIX_SERVER="$PROJECT_ROOT/integrations/qdrant_mimic/synrix-server-evaluation"
LATTICE_PATH="$HOME/.synrix_quickstart.lattice"
PORT=6334

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  SYNRIX Quick Start                                            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if server exists
if [ ! -f "$SYNRIX_SERVER" ]; then
    echo "❌ SYNRIX server not found at: $SYNRIX_SERVER"
    echo ""
    echo "Building SYNRIX server..."
    cd "$PROJECT_ROOT/integrations/qdrant_mimic"
    make
    cd - > /dev/null
fi

# Check if port is in use
if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "⚠️  Port $PORT is already in use"
    echo "   Using existing SYNRIX server..."
    SERVER_RUNNING=true
else
    echo "🚀 Starting SYNRIX server on port $PORT..."
    "$SYNRIX_SERVER" --port $PORT --lattice-path "$LATTICE_PATH" > /tmp/synrix_quickstart.log 2>&1 &
    SYNRIX_PID=$!
    sleep 2
    
    # Check if server started
    if ! kill -0 $SYNRIX_PID 2>/dev/null; then
        echo "❌ Failed to start SYNRIX server"
        echo "   Check logs: /tmp/synrix_quickstart.log"
        exit 1
    fi
    SERVER_RUNNING=false
    echo "✅ SYNRIX server started (PID: $SYNRIX_PID)"
fi

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Your SYNRIX Server is Ready                                   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "📍 Server URL: http://localhost:$PORT"
echo "📁 Lattice: $LATTICE_PATH"
echo ""
echo "📝 To use in your LangChain app, change:"
echo ""
echo "   url='http://localhost:6333'  # Qdrant"
echo "   ↓"
echo "   url='http://localhost:$PORT'  # SYNRIX"
echo ""
echo "💡 That's the entire migration!"
echo ""

# Show example
cat << 'EOF'
Example:

from langchain_community.vectorstores import Qdrant
from langchain_community.embeddings import FakeEmbeddings

vectorstore = Qdrant.from_texts(
    texts=["Your documents here"],
    embedding=FakeEmbeddings(size=128),
    url='http://localhost:6334'  # ← SYNRIX
)

results = vectorstore.similarity_search("your query", k=3)
EOF

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Press Ctrl+C to stop the server when done"
echo ""

# Wait for interrupt
if [ "$SERVER_RUNNING" = false ]; then
    trap "echo ''; echo '🛑 Stopping SYNRIX server...'; kill $SYNRIX_PID 2>/dev/null; wait $SYNRIX_PID 2>/dev/null; echo '✅ Server stopped'; exit 0" INT TERM
    wait $SYNRIX_PID
fi
