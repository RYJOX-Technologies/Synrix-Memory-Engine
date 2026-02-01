#!/usr/bin/env bash
# Build SYNRIX shared library for Linux
# Run this from the build/linux directory

set -e

echo "========================================"
echo "  SYNRIX Linux Build"
echo "========================================"
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found!"
    echo "  Install with: sudo apt-get install cmake"
    echo "  Or: sudo yum install cmake"
    exit 1
fi

# Check for GCC
if ! command -v gcc &> /dev/null; then
    echo "ERROR: GCC not found!"
    echo "  Install with: sudo apt-get install build-essential"
    echo "  Or: sudo yum install gcc"
    exit 1
fi

echo "OK: CMake found: $(which cmake)"
echo "OK: GCC found: $(which gcc)"
echo ""

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DSYNRIX_FREE_TIER_50K=ON

# Build
echo ""
echo "Building..."
cmake --build . -j$(nproc)

# Find output
echo ""
echo "Locating output..."
SO_PATH=$(find . -name "libsynrix*.so" | head -1)

if [ -n "$SO_PATH" ]; then
    echo "OK: Build successful!"
    echo "  Library: $(realpath "$SO_PATH")"
    echo "  Size: $(du -h "$SO_PATH" | cut -f1)"
else
    echo "WARNING: Build completed but .so not found"
    echo "  Check build directory: $BUILD_DIR"
fi

echo ""
echo "Next steps:"
echo "  1. Copy libsynrix_free_tier.so to ../../../synrix/"
echo "  2. Test with: python -c 'from synrix.raw_backend import RawSynrixBackend'"
echo ""
