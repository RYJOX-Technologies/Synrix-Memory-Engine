#!/bin/bash
# Create transfer package for Windows build
# Run this on Jetson, then transfer to Windows
# Includes: Engine source, Python SDK, build scripts, documentation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="/tmp/synrix-windows-build"

echo "Creating Windows build transfer package..."
echo "Output: $OUTPUT_DIR"
echo "Project root: $PROJECT_ROOT"

# Create output directory structure
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"/{build/windows,python-sdk}

# 1. Copy build files (CMakeLists.txt, build scripts, docs)
echo "Copying build files..."
cp -r "$SCRIPT_DIR"/* "$OUTPUT_DIR/build/windows/"

# 2. Copy Python SDK (essential files only)
echo "Copying Python SDK..."
PYTHON_SDK="$PROJECT_ROOT/python-sdk"

# Copy entire synrix package
if [ -d "$PYTHON_SDK/synrix" ]; then
    cp -r "$PYTHON_SDK/synrix" "$OUTPUT_DIR/python-sdk/"
fi

# Copy essential SDK files
for file in setup.py pyproject.toml README.md; do
    if [ -f "$PYTHON_SDK/$file" ]; then
        cp "$PYTHON_SDK/$file" "$OUTPUT_DIR/python-sdk/"
    fi
done

# Copy examples directory (useful for testing)
if [ -d "$PYTHON_SDK/examples" ]; then
    cp -r "$PYTHON_SDK/examples" "$OUTPUT_DIR/python-sdk/"
fi

# 3. Copy lattice file (the actual memory/knowledge graph)
LATTICE_FILE="$HOME/.cursor_ai_memory.lattice"
if [ -f "$LATTICE_FILE" ]; then
    echo "Copying lattice file (your memories)..."
    mkdir -p "$OUTPUT_DIR/lattice"
    cp "$LATTICE_FILE" "$OUTPUT_DIR/lattice/cursor_ai_memory.lattice"
    LATTICE_SIZE=$(du -h "$LATTICE_FILE" | cut -f1)
    echo "  Lattice file: $LATTICE_SIZE"
else
    echo "⚠️  Lattice file not found at $LATTICE_FILE"
    echo "   You can transfer it separately later if needed"
fi

# 4. Copy context export and import script
if [ -f "/tmp/synrix_context_export.json" ]; then
    cp /tmp/synrix_context_export.json "$OUTPUT_DIR/"
fi
if [ -f "$SCRIPT_DIR/import_context.py" ]; then
    cp "$SCRIPT_DIR/import_context.py" "$OUTPUT_DIR/python-sdk/"
fi

# 5. Create README for Windows
cat > "$OUTPUT_DIR/README.md" << 'EOF'
# SYNRIX Windows Build Package

Complete package for building and using SYNRIX on Windows.

## Contents

- `build/windows/` - Engine source files, CMakeLists.txt, build scripts
- `python-sdk/` - Python SDK with Windows DLL support
- `lattice/cursor_ai_memory.lattice` - **Your actual memory/knowledge graph** (use this!)
- `synrix_context_export.json` - Exported context from Jetson (backup/fallback)
- `python-sdk/import_context.py` - Script to import context into Windows SYNRIX

## Quick Start

1. **Build the engine:**
   ```powershell
   cd build\windows
   .\build.ps1
   ```

2. **Copy DLL to Python SDK:**
   ```powershell
   # After build, DLL will be in build\windows\build\Release\libsynrix.dll
   copy build\windows\build\Release\libsynrix.dll python-sdk\
   ```

3. **Test Python SDK:**
   ```powershell
   cd python-sdk
   python -c "from synrix.raw_backend import RawSynrixBackend; print('OK')"
   ```

4. **Use your lattice file (your memories):**
   ```powershell
   # Copy lattice file to your home directory
   copy lattice\cursor_ai_memory.lattice $env:USERPROFILE\.cursor_ai_memory.lattice
   
   # Test it works
   cd python-sdk
   python -c "from synrix.raw_backend import RawSynrixBackend; m = RawSynrixBackend('~/.cursor_ai_memory.lattice'); print('✅ Memories loaded!')"
   ```

5. **Import context (optional, if lattice file missing):**
   ```powershell
   cd python-sdk
   python import_context.py
   ```

## Structure

```
synrix-windows-build/
├── build/
│   └── windows/          # Engine build files
│       ├── src/          # C source files
│       ├── include/      # Header files
│       ├── CMakeLists.txt
│       ├── build.ps1
│       └── *.md          # Documentation
├── python-sdk/           # Python SDK
│   ├── synrix/           # Python package
│   ├── examples/         # Example scripts
│   └── import_context.py
├── lattice/              # Your memory/knowledge graph
│   └── cursor_ai_memory.lattice
└── README.md
```

## Requirements

- CMake 3.15+
- Visual Studio 2019/2022 OR MinGW-w64
- Python 3.7+

See `build/windows/BUILD.md` for detailed instructions.
EOF

# Create archive
echo "Creating archive..."
cd /tmp
tar -czf synrix-windows-build.tar.gz synrix-windows-build/

# Calculate size
SIZE=$(du -h synrix-windows-build.tar.gz | cut -f1)

echo ""
echo "✅ Transfer package created: /tmp/synrix-windows-build.tar.gz"
echo "   Size: $SIZE"
echo ""
echo "Transfer to Windows:"
echo "  scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz \$env:USERPROFILE\\Downloads\\"
echo ""
echo "On Windows, extract and run:"
echo "  cd C:\\synrix-windows-build"
echo "  cd build\\windows"
echo "  .\\build.ps1"
echo ""
