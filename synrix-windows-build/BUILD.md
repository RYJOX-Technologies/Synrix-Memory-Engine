# Building SYNRIX DLL for Windows

## Quick Start

### Method 1: MSYS2/MinGW-w64 (Recommended)

**Prerequisites:**
- Install MSYS2 from https://www.msys2.org/
- Install build tools: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make`

**Build:**
```bash
cd build/windows
bash build_msys2.sh
```

**Output:** `build_msys2/bin/libsynrix.dll`

### Method 2: Free Tier Build (50k Node Limit)

```bash
cd build/windows
bash build_free_tier_50k.sh
```

**Output:** `build_free_tier/bin/libsynrix_free_tier.dll`

### Method 3: PowerShell Script

```powershell
cd C:\synrix-windows-build\synrix-windows-build
.\build.ps1
```

### Method 4: Batch File

```cmd
cd C:\synrix-windows-build\synrix-windows-build
build.bat Release
```

## Prerequisites

### Required
- **CMake 3.15+** - https://cmake.org/download/
- **MSYS2 with MinGW-w64** (Recommended) - https://www.msys2.org/
  - OR **Visual Studio 2019/2022** (Community Edition is free)
  - OR **MinGW-w64** standalone

### MSYS2 Installation

1. Download and install MSYS2 from https://www.msys2.org/
2. Open MSYS2 MinGW64 terminal
3. Install build tools:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-make
   pacman -S mingw-w64-x86_64-zlib
   ```

### Verify Installation

```bash
cmake --version
gcc --version
```

Should show CMake 3.15+ and GCC.

## Build Output

After building, the DLL will be in:
- **MSYS2/MinGW**: `build/windows/build_msys2/bin/libsynrix.dll`
- **Free Tier**: `build/windows/build_free_tier/bin/libsynrix_free_tier.dll`
- **Visual Studio**: `build/Release/libsynrix.dll` or `build/Debug/libsynrix.dll`

## Copy DLL to Python SDK

After building:
```bash
cp build_msys2/bin/libsynrix.dll ../../python-sdk/
```

Or on Windows:
```powershell
Copy-Item build\windows\build_msys2\bin\libsynrix.dll python-sdk\
```

## Testing

Test the build:
```python
import sys
sys.path.insert(0, 'python-sdk')
from synrix.raw_backend import RawSynrixBackend

b = RawSynrixBackend('test.lattice', max_nodes=100000, evaluation_mode=False)
node_id = b.add_node('TEST:node', 'data', 5)
result = b.get_node(node_id)
print(f"OK: Node {node_id} = {result['name']}")
b.save()
b.close()
```

## Known Issues & Fixes

### Windows File Replacement
- **Issue**: `rename()` fails if target exists on Windows
- **Fix**: Uses `MoveFileEx` with `MOVEFILE_REPLACE_EXISTING` for atomic replacement
- **Status**: ✅ Fixed in current build

### Memory-Mapped File Deletion
- **Issue**: Cannot delete memory-mapped files on Windows
- **Fix**: Unmaps and closes file before atomic replacement
- **Status**: ✅ Fixed in current build

### DLL Dependencies
- **Required DLLs**: `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`, `zlib1.dll`
- **Location**: Copied to `python-sdk/` directory
- **Status**: ✅ Included in package

## Troubleshooting

### "CMake not found"
- Install CMake and restart terminal
- Or add CMake to PATH manually
- For MSYS2: `pacman -S mingw-w64-x86_64-cmake`

### "GCC not found"
- Install MinGW-w64 via MSYS2: `pacman -S mingw-w64-x86_64-gcc`
- Or install Visual Studio with C++ workload

### "DLL not found after build"
- Check `build_msys2/bin/` directory
- Search: `find . -name "libsynrix*.dll"`
- Verify CMake build completed successfully

### "Failed to load DLL"
- Ensure DLL is in `python-sdk/` directory
- Check DLL dependencies are present
- Set `SYNRIX_LIB_PATH` environment variable if needed

### "Permission denied" on save
- **Fixed**: Current build uses atomic file replacement
- If still occurs, ensure file isn't locked by another process

## Build Options

### Standard Build (Unlimited)
```bash
cd build/windows
bash build_msys2.sh
```

### Free Tier Build (50k Node Limit)
```bash
cd build/windows
bash build_free_tier_50k.sh
```

### Custom Build
```bash
cd build/windows
mkdir build_custom
cd build_custom
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Next Steps

1. ✅ Build `libsynrix.dll`
2. ✅ Copy to Python SDK directory
3. ✅ Test with Python
4. ✅ Run robustness tests: `python test_windows_robust.py`
5. ✅ Package for distribution (see `create_free_tier_package.ps1`)
