#!/bin/bash
# Create transfer package for Windows build
# Run this on Jetson, then transfer to Windows

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="/tmp/synrix-windows-build"

echo "Creating Windows build transfer package..."
echo "Output: $OUTPUT_DIR"

# Create output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Copy all files from build/windows
echo "Copying build files..."
cp -r "$SCRIPT_DIR"/* "$OUTPUT_DIR/"

# Create archive
echo "Creating archive..."
cd /tmp
tar -czf synrix-windows-build.tar.gz synrix-windows-build/

echo ""
echo "✅ Transfer package created: /tmp/synrix-windows-build.tar.gz"
echo ""
echo "Transfer to Windows:"
echo "  scp astro@jetson-ip:/tmp/synrix-windows-build.tar.gz C:\\synrix-windows-build.tar.gz"
echo ""
echo "On Windows, extract and run:"
echo "  cd C:\\synrix-windows-build"
echo "  .\\build.ps1"
echo ""
