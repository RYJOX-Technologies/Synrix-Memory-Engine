#!/bin/bash
# Query Latency Diagnostic - Per-query nanosecond measurement
# ===========================================================
#
# Answers: Are 96/192 ns real per-query latencies?
#
# Usage:
#   ./run_query_latency_diagnostic.sh [lattice_path] [iterations]
#
# If no lattice exists, run extended benchmark first:
#   ./run_extended_p99_benchmark.sh 1000 100000
# (creates /tmp/extended_p99_benchmark_c.lattice)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

LATTICE_PATH="${1:-/tmp/query_latency_diagnostic.lattice}"
ITERATIONS="${2:-1000}"

echo "Query Latency Diagnostic"
echo "========================"
echo "Lattice: $LATTICE_PATH"
echo "Iterations: $ITERATIONS"
echo "(Diagnostic will create minimal lattice if not found)"
echo ""

# Build
echo "Building diagnostic..."
LATTICE_SRC="src/storage/lattice/persistent_lattice.c \
             src/storage/lattice/wal.c \
             src/storage/lattice/isolation.c \
             src/storage/lattice/seqlock.c \
             src/storage/lattice/dynamic_prefix_index.c \
             src/storage/lattice/exact_name_index.c \
             src/storage/lattice/lattice_constraints.c \
             src/storage/lattice/license_utils.c"

gcc -O3 -std=c11 -Wall -Wextra \
    -I. -I./src/storage/lattice \
    tools/query_latency_diagnostic.c \
    $LATTICE_SRC \
    -lm -lpthread -o tools/query_latency_diagnostic 2>&1 || true

if [ ! -f tools/query_latency_diagnostic ]; then
    echo "Build failed"
    exit 1
fi

echo "Running..."
echo ""
tools/query_latency_diagnostic "$LATTICE_PATH" "$ITERATIONS"
