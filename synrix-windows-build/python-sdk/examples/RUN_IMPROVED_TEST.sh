#!/bin/bash
# Run HumanEval test with IMPROVED patterns and composition logic
# Expected: 73.2% → 80-85% pass rate

cd "$(dirname "$0")"

export LD_LIBRARY_PATH="/mnt/nvme/aion-omega/NebulOS-Scaffolding/src/storage/lattice:$LD_LIBRARY_PATH"
export PYTHONPATH="/mnt/nvme/aion-omega/NebulOS-Scaffolding/python-sdk:$PYTHONPATH"
export HUMANEVAL_USE_IMPROVED=true

echo "======================================================================"
echo "HumanEval Test - IMPROVED Patterns + Better Composition Logic"
echo "======================================================================"
echo ""
echo "Using improved patterns (~55 high-quality, specific patterns)"
echo "Composition logic actually USES the patterns it finds"
echo ""
echo "Expected: 80-85% pass rate (up from 73.2%)"
echo ""
echo "Starting test..."
echo ""

# Remove old lattice
rm -f humaneval_poc.lattice

# Run test
python3 -u test_humaneval_poc.py 2>&1 | tee /tmp/humaneval_improved.log

echo ""
echo "======================================================================"
echo "Test complete. Results saved to /tmp/humaneval_improved.log"
echo "======================================================================"

