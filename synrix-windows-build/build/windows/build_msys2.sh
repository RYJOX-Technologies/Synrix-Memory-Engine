#!/usr/bin/env bash
# Build SYNRIX DLL using MSYS2 MinGW-w64
# Run this from MSYS2 MinGW64 terminal

set -e

echo "========================================"
echo "  SYNRIX Windows Build (MSYS2/MinGW-w64)"
echo "========================================"
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found!"
    echo "  Install with: pacman -S mingw-w64-x86_64-cmake"
    exit 1
fi

# Check for GCC
if ! command -v gcc &> /dev/null; then
    echo "ERROR: GCC not found!"
    echo "  Install with: pacman -S mingw-w64-x86_64-gcc"
    exit 1
fi

echo "OK: CMake found: $(which cmake)"
echo "OK: GCC found: $(which gcc)"
echo "  (For signed license keys, install OpenSSL: pacman -S mingw-w64-x86_64-openssl)"
echo ""

# Create build directory
BUILD_DIR="build_msys2"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring CMake..."
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Building..."
cmake --build . --config Release

# Find output
echo ""
echo "Locating output..."
DLL_PATH=$(find . -name "libsynrix.dll" | head -1)

if [ -n "$DLL_PATH" ]; then
    echo "OK: Build successful!"
    echo "  DLL: $(realpath "$DLL_PATH")"
    echo "  Size: $(du -h "$DLL_PATH" | cut -f1)"
else
    echo "WARNING: Build completed but DLL not found"
    echo "  Check build directory: $BUILD_DIR"
fi

echo ""
echo "Next steps:"
echo "  1. Copy libsynrix.dll to your Python SDK"
echo "  2. Test with: python -c 'from synrix.raw_backend import RawSynrixBackend'"
echo ""
