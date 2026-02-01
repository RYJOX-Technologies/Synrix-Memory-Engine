# Building Free Tier Package with MSYS2

## Prerequisites

1. **MSYS2 installed** - https://www.msys2.org/
2. **MinGW-w64 toolchain** installed in MSYS2:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-make
   ```

## Build Steps

1. **Open MSYS2 MinGW64 terminal** (not PowerShell!)

2. **Navigate to build directory:**
   ```bash
   cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows
   ```

3. **Run the build script:**
   ```bash
   bash build_free_tier_50k.sh
   ```

## Output

The DLL will be created at:
```
build/windows/build_free_tier/bin/libsynrix_free_tier.dll
```

## What This Build Does

- Creates `libsynrix_free_tier.dll` with hard-coded 50,000 node limit
- Evaluation mode is always enabled (cannot be disabled)
- `lattice_disable_evaluation_mode()` returns an error

## Important Notes

- **Must run from MSYS2 MinGW64 terminal**, not PowerShell
- Uses the same toolchain as the regular MSYS2 build
- Output DLL is compatible with the Python SDK
