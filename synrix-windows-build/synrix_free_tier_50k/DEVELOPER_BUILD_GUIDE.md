# Developer Build Guide - For Joseph

This guide explains how to build the SYNRIX engine from source for Windows and Linux.

## What You Need

1. **Python SDK** (already included in `synrix/` directory)
2. **Engine Source Code** (in `engine/` directory)
3. **Build Tools** (CMake + compiler)

## Quick Start

### Windows (MSYS2/MinGW - Recommended)

```bash
# 1. Install MSYS2 from https://www.msys2.org/
# 2. Open MSYS2 MinGW64 terminal
# 3. Install build tools:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make

# 4. Build:
cd engine/build/windows
bash build_msys2.sh

# 5. Copy DLL to Python SDK:
cp build_msys2/bin/libsynrix_free_tier.dll ../../../synrix/
```

### Windows (Visual Studio)

```powershell
# 1. Install Visual Studio 2022 (Community Edition is free)
#    - Select "Desktop development with C++" workload
# 2. Install CMake 3.15+ from https://cmake.org/download/

# 3. Build:
cd engine\build\windows
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DSYNRIX_FREE_TIER_50K=ON
cmake --build . --config Release

# 4. Copy DLL to Python SDK:
Copy-Item build\Release\libsynrix_free_tier.dll ..\..\..\synrix\
```

### Linux

```bash
# 1. Install build tools:
sudo apt-get update
sudo apt-get install -y build-essential cmake

# 2. Build:
cd engine/build/linux
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSYNRIX_FREE_TIER_50K=ON
make -j$(nproc)

# 3. Copy .so to Python SDK:
cp libsynrix_free_tier.so ../../../synrix/
```

## Directory Structure

```
synrix_free_tier_50k/
├── synrix/              # Python SDK (ready to use)
│   ├── __init__.py
│   ├── _native.py       # Windows DLL loader
│   ├── raw_backend.py   # Core backend
│   ├── ai_memory.py     # AI memory interface
│   └── *.dll or *.so    # Built binaries go here
├── engine/              # Engine source code
│   ├── src/             # C source files
│   ├── include/         # Header files
│   └── build/
│       ├── windows/     # Windows build files
│       │   ├── CMakeLists.txt
│       │   ├── build.ps1
│       │   └── build_msys2.sh
│       └── linux/       # Linux build files
│           ├── CMakeLists.txt
│           └── build.sh
└── README.md
```

## Build Configuration

### Free Tier Build (50k Node Limit)

The build is configured for free tier by default:

- **Windows**: `-DSYNRIX_FREE_TIER_50K=ON` in CMake
- **Linux**: `-DSYNRIX_FREE_TIER_50K=ON` in CMake
- **Output**: `libsynrix_free_tier.dll` (Windows) or `libsynrix_free_tier.so` (Linux)

### Full Build (No Limits)

To build without limits, remove the `-DSYNRIX_FREE_TIER_50K=ON` flag:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
# (no -DSYNRIX_FREE_TIER_50K=ON)
```

## Prerequisites

### Windows

**Option 1: MSYS2 (Recommended)**
- MSYS2 from https://www.msys2.org/
- MinGW-w64 toolchain: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake`

**Option 2: Visual Studio**
- Visual Studio 2022 (Community Edition)
- CMake 3.15+

### Linux

```bash
sudo apt-get install build-essential cmake
# or
sudo yum install gcc cmake make
```

## Source Files

### Core Source Files (Required)

- `src/persistent_lattice.c` - Main lattice implementation
- `src/wal.c` - Write-ahead log
- `src/dynamic_prefix_index.c` - Prefix indexing
- `src/semantic_index.c` - Semantic search
- `src/export.c` - Export functions
- `src/checksum.c` - Checksum validation
- `src/license_utils.c` - License validation
- `src/seqlock.c` - Sequential locks (for WAL)
- `src/windows_compat.c` - Windows compatibility (Windows only)

### Optional Source Files

- `src/cluster_persistence.c`
- `src/namespace_isolation.c`
- `src/lattice_constraints.c`
- `src/isolation.c`
- `src/integrity_validator.c`
- `src/intelligent_lattice_loader.c`
- `src/lattice_dump.c`

## Build Output

After building, you'll get:

- **Windows**: `libsynrix_free_tier.dll` (or `libsynrix.dll` for full build)
- **Linux**: `libsynrix_free_tier.so` (or `libsynrix.so` for full build)

Copy this file to the `synrix/` directory alongside the Python SDK files.

## Testing the Build

```python
import sys
sys.path.insert(0, 'synrix_free_tier_50k')

from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:build", "Engine built successfully!")
print(f"✅ Build test passed! Found {len(memory.query('TEST:'))} nodes")
```

## Troubleshooting

### Windows: "CMake not found"
- Install CMake and restart terminal
- Or add CMake to PATH: `$env:PATH += ";C:\Program Files\CMake\bin"`

### Windows: "GCC not found" (MSYS2)
- Make sure you're in MSYS2 MinGW64 terminal
- Install: `pacman -S mingw-w64-x86_64-gcc`

### Linux: "Permission denied"
- Make sure build scripts are executable: `chmod +x build.sh`

### Build fails with "undefined reference"
- Check that all source files are listed in CMakeLists.txt
- Verify include paths are correct

### DLL/.so not found after build
- Check `build/` subdirectory
- Look in `Release/` or `Debug/` folders (Visual Studio)
- Search: `find . -name "libsynrix*"`

## Next Steps

1. Build the engine (choose Windows or Linux method above)
2. Copy the binary to `synrix/` directory
3. Test with Python (see Testing section)
4. Use the Python SDK (see `QUICK_START.md`)

## Full Documentation

- `README.md` - Package overview
- `QUICK_START.md` - 5-minute getting started
- `AI_AGENT_GUIDE.md` - Comprehensive usage examples
- `INSTALL.md` - Installation options
