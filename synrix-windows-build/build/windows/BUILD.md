# Building SYNRIX DLL for Windows

## Quick Start

### Method 1: PowerShell Script (Easiest)

```powershell
cd C:\synrix-windows-build
.\build.ps1
```

### Method 2: Batch File

```cmd
cd C:\synrix-windows-build
build.bat Release
```

### Method 3: CMake Directly

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Prerequisites

### Required
- **CMake 3.15+** - https://cmake.org/download/
- **Visual Studio 2019/2022** (Community Edition is free)
  - Install "Desktop development with C++" workload
  - OR **MinGW-w64** via MSYS2

### Verify Installation

```powershell
cmake --version
```

Should show CMake 3.15 or higher.

## Directory Structure

```
build/windows/
├── src/              # All .c source files
├── include/          # All .h header files
├── CMakeLists.txt   # CMake build configuration
├── build.ps1        # PowerShell build script
├── build.bat        # Batch file build script
└── README.md        # This file
```

## Build Output

After building, the DLL will be in:
- **Visual Studio**: `build/Release/libsynrix.dll` or `build/Debug/libsynrix.dll`
- **MinGW**: `build/libsynrix.dll`

## Transfer from Jetson

**On Jetson:**
```bash
cd /mnt/nvme/aion-omega/NebulOS-Scaffolding/build/windows
bash create-transfer-package.sh
```

**On Windows:**
```powershell
# Transfer
scp astro@jetson-ip:/tmp/synrix-windows-build.tar.gz C:\synrix-windows-build.tar.gz

# Extract (use 7-Zip or tar if available)
# Then navigate and build
cd C:\synrix-windows-build
.\build.ps1
```

## Troubleshooting

### "CMake not found"
- Install CMake and restart PowerShell
- Or add CMake to PATH manually

### "Visual Studio not found"
- Install Visual Studio with C++ workload
- Or use MinGW: `cmake .. -G "MinGW Makefiles"`

### "Build failed"
- Check that all source files are in `src/` directory
- Check that all headers are in `include/` directory
- Verify CMakeLists.txt paths are correct

### "DLL not found after build"
- Check `build/` subdirectory
- Look in `Release/` or `Debug/` folders
- Search: `Get-ChildItem -Recurse -Filter "libsynrix.dll"`

## Next Steps

1. Build `libsynrix.dll`
2. Copy to Python SDK directory
3. Update Python SDK `raw_backend.py` to find Windows DLL
4. Test with Python
