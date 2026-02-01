#!/bin/bash
# Run the pure C benchmark in MSYS2 environment

cd "$(dirname "$0")"

echo "========================================"
echo "  SYNRIX Pure C Performance Benchmark"
echo "========================================"
echo ""

# Set library path
export PATH="$PATH:/c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows/build_msys2/bin"

# Run benchmark
./benchmark_c_raw.exe 10000 1000 100
