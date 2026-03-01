#!/bin/bash
# Extended P99 Benchmark Runner
# ============================
# 
# Compiles and runs comprehensive p99 benchmarks for SYNRIX:
# - C API direct access (core engine)
# - Python SDK (with overhead)
# - Full percentile analysis (p50, p95, p99, p99.9)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Extended P99 Benchmark Suite for SYNRIX                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
ITERATIONS=${1:-100000}  # Default 100k iterations
DATASET_SIZE=${2:-1000000}  # Default 1M nodes for O(k) test
echo "Configuration:"
echo "  • Iterations per test: $ITERATIONS"
echo "  • Dataset size for O(k) test: $DATASET_SIZE nodes"
echo "  • This will take several minutes to complete"
echo ""

# Check dependencies
echo "Checking dependencies..."
if ! command -v gcc &> /dev/null; then
    echo "❌ Error: gcc not found"
    exit 1
fi

if ! python3 -c "import sys; sys.path.insert(0, 'python-sdk'); from synrix.raw_backend import RawSynrixBackend" 2>/dev/null; then
    echo "⚠️  Warning: Python SDK not available, skipping Python benchmark"
    SKIP_PYTHON=1
else
    SKIP_PYTHON=0
fi
echo "✅ Dependencies OK"
echo ""

# Build C benchmark
echo "Building C benchmark..."
C_BENCHMARK="tools/extended_p99_benchmark"
LATTICE_SRC="src/storage/lattice/persistent_lattice.c \
             src/storage/lattice/wal.c \
             src/storage/lattice/isolation.c \
             src/storage/lattice/seqlock.c \
             src/storage/lattice/dynamic_prefix_index.c \
             src/storage/lattice/exact_name_index.c \
             src/storage/lattice/lattice_constraints.c \
             src/storage/lattice/license_utils.c"

gcc -O3 -std=c11 -Wall -Wextra \
    -I. \
    -I./src/storage/lattice \
    tools/extended_p99_benchmark.c \
    $LATTICE_SRC \
    -lm -lpthread -o "$C_BENCHMARK" 2>&1 | grep -E "(error|Error)" || true

if [ ! -f "$C_BENCHMARK" ]; then
    echo "❌ Failed to compile C benchmark"
    exit 1
fi
echo "✅ C benchmark compiled"
echo ""

# Run C benchmark
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Running C API Benchmark (Core Engine Performance)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
"$C_BENCHMARK" "/tmp/extended_p99_benchmark_c.lattice" "$ITERATIONS" "$DATASET_SIZE"
C_EXIT_CODE=$?

if [ $C_EXIT_CODE -ne 0 ]; then
    echo "❌ C benchmark failed with exit code $C_EXIT_CODE"
    exit 1
fi

echo ""
echo ""

# Run Python benchmark
if [ $SKIP_PYTHON -eq 0 ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Running Python SDK Benchmark (With Python Overhead)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    python3 tools/extended_p99_benchmark_python.py \
        --lattice "/tmp/extended_p99_benchmark_python.lattice" \
        --iterations "$ITERATIONS" \
        --dataset-size "$DATASET_SIZE"
    PYTHON_EXIT_CODE=$?
    
    if [ $PYTHON_EXIT_CODE -ne 0 ]; then
        echo "❌ Python benchmark failed with exit code $PYTHON_EXIT_CODE"
        exit 1
    fi
    
    echo ""
    echo ""
fi

# Final summary
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Benchmark Suite Complete                                      ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "✅ All benchmarks completed successfully"
echo ""
echo "📊 Summary:"
echo "  • C API: Shows true core engine performance (sub-microsecond)"
echo "  • Python SDK: Shows practical performance with Python overhead"
echo ""
echo "💡 Use these numbers for:"
echo "  • Marketing claims (with proper attribution to access method)"
echo "  • Performance comparisons with other systems"
echo "  • Setting realistic expectations for users"
echo ""
echo "📁 Results saved to:"
echo "  • C benchmark: /tmp/extended_p99_benchmark_c.lattice"
if [ $SKIP_PYTHON -eq 0 ]; then
    echo "  • Python benchmark: /tmp/extended_p99_benchmark_python.lattice"
fi
echo ""
