#!/bin/bash
#
# SYNRIX Capabilities Demo
# ========================
#
# Shows clear capabilities that SYNRIX enables with pre-loaded data
#

set -e

DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DEMO_DIR"

# Colors
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   SYNRIX Capabilities Demo                               ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Set library path
export LD_LIBRARY_PATH="..:$LD_LIBRARY_PATH"
export PYTHONPATH="..:$PYTHONPATH"

# Step 1: Pre-load demo data
echo -e "${BLUE}Step 1: Pre-loading demo data into SYNRIX...${NC}"
echo ""
python3 preload_demo_data.py
echo ""

# Step 2: Run capability tests
echo -e "${BLUE}Step 2: Testing SYNRIX capabilities...${NC}"
echo ""
python3 test_synrix_capabilities.py

echo ""
echo -e "${GREEN}✅ Demo complete!${NC}"
echo ""
echo "This demonstrates:"
echo "  • Persistent memory (survives restarts)"
echo "  • Fast retrieval (sub-microsecond)"
echo "  • Context awareness (past conversations)"
echo "  • Learning (stored patterns)"
echo "  • Multi-domain knowledge (unified storage)"
echo ""

