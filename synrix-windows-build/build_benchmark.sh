#!/bin/bash
# Build script for pure C benchmark

set -e

echo "========================================"
echo "  Building SYNRIX Pure C Benchmark"
echo "========================================"

# Detect platform
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    PLATFORM="Windows"
    CC="gcc"
    LIB_EXT=".dll"
    LDFLAGS="-L./build_msys2/bin -lsynrix -lz"
    CFLAGS="-O3 -I./include -I./build/windows/include"
    OUTPUT="benchmark_c_raw.exe"
else
    PLATFORM="Linux"
    CC="gcc"
    LIB_EXT=".so"
    LDFLAGS="-L./build -lsynrix -lz -lm"
    CFLAGS="-O3 -I./include"
    OUTPUT="benchmark_c_raw"
fi

echo "Platform: $PLATFORM"
echo "Compiler: $CC"
echo ""

# Check if library exists
if [ ! -f "build_msys2/bin/libsynrix.dll" ] && [ ! -f "build/libsynrix.so" ]; then
    echo "ERROR: Synrix library not found!"
    echo "Please build the library first:"
    echo "  Windows: cd build/windows && bash build_msys2.sh"
    echo "  Linux: mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# Compile benchmark
echo "Compiling benchmark..."
$CC $CFLAGS -o $OUTPUT benchmark_c_raw.c $LDFLAGS

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "Run with: ./$OUTPUT [node_count] [lookup_count] [prefix_iterations]"
    echo "Example: ./$OUTPUT 10000 1000 100"
else
    echo "❌ Build failed"
    exit 1
fi
