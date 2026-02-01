# Building SYNRIX with MSYS2

MSYS2 provides a full POSIX environment on Windows with all necessary headers (`sys/mman.h`, etc.).

## Step 1: Install MSYS2

1. Download from: https://www.msys2.org/
2. Run the installer
3. Open **MSYS2 MinGW 64-bit** terminal (not MSYS2 UCRT64 or MSYS)

## Step 2: Install Required Packages

In the MSYS2 MinGW 64-bit terminal:

```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
```

## Step 3: Build SYNRIX

```bash
# Navigate to build directory
cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows

# Run build script
bash build_msys2.sh
```

Or manually:

```bash
cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows
mkdir -p build_msys2
cd build_msys2
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Step 4: Find the DLL

The DLL will be at:
```
build/windows/build_msys2/libsynrix.dll
```

## Step 5: Copy to Python SDK

```bash
# From MSYS2 terminal
cp build_msys2/libsynrix.dll ../../../python-sdk/
```

Or from PowerShell:
```powershell
Copy-Item build\windows\build_msys2\libsynrix.dll python-sdk\
```

## Troubleshooting

### "CMake not found"
```bash
pacman -S mingw-w64-x86_64-cmake
```

### "GCC not found"
```bash
pacman -S mingw-w64-x86_64-gcc
```

### "sys/mman.h: No such file or directory"
Make sure you're using **MSYS2 MinGW 64-bit** terminal, not MSYS2 UCRT64 or plain MSYS.

### Path issues
MSYS2 uses Unix-style paths. Use `/c/Users/...` instead of `C:\Users\...`
