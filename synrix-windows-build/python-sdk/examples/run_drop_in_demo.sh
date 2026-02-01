#!/bin/bash
# Run the drop-in replacement demo with NVME Python environment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NVME_ENV="/mnt/nvme/aion-omega/python-env"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Drop-In Replacement Demo - Setup                              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if NVME environment exists
if [ ! -d "$NVME_ENV" ]; then
    echo "❌ NVME Python environment not found at $NVME_ENV"
    echo "   Run: cd /mnt/nvme/aion-omega && python3 -m venv python-env"
    exit 1
fi

echo "✓ NVME Python environment found"
echo ""

# Activate environment
source "$NVME_ENV/bin/activate"

# Set library path for SYNRIX
export LD_LIBRARY_PATH="$SCRIPT_DIR/..:$LD_LIBRARY_PATH"

# Check LangChain
if ! python3 -c "import langchain_community" 2>/dev/null; then
    echo "⚠️  LangChain not installed in NVME environment"
    echo "   Installing..."
    pip install langchain langchain-community qdrant-client requests
fi

echo "✓ Environment ready"
echo "  Python: $(which python3)"
echo "  LangChain: $(python3 -c 'import langchain; print(langchain.__version__)' 2>/dev/null || echo 'installed')"
echo ""

# Run demo
echo "🚀 Starting demo..."
echo ""
cd "$SCRIPT_DIR"
python3 vc_demo_drop_in_replacement.py
