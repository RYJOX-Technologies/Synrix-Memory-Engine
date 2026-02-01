#!/bin/bash
# Run HumanEval test with Phase 1 enhanced patterns
# Expected: 73.2% → 82-85% pass rate

cd "$(dirname "$0")"

export LD_LIBRARY_PATH="/mnt/nvme/aion-omega/NebulOS-Scaffolding/src/storage/lattice:$LD_LIBRARY_PATH"
export PYTHONPATH="/mnt/nvme/aion-omega/NebulOS-Scaffolding/python-sdk:$PYTHONPATH"
export HUMANEVAL_USE_PHASE1=true

echo "======================================================================"
echo "HumanEval Test - Phase 1 Enhanced Patterns"
echo "======================================================================"
echo ""
echo "Using Phase 1 patterns (~60 atomic fundamentals)"
echo "Expected: 82-85% pass rate (up from 73.2%)"
echo ""
echo "Starting test..."
echo ""

# Remove old lattice
rm -f humaneval_poc.lattice

# Run test
python3 -u test_humaneval_poc.py 2>&1 | tee /tmp/humaneval_phase1.log

echo ""
echo "======================================================================"
echo "Test complete. Results saved to /tmp/humaneval_phase1.log"
echo "======================================================================"

