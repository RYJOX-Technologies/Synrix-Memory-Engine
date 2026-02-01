# Quick Build Guide - Free Tier 50k Package

## From Project Root

You're currently in: `C:\Users\Livew\Desktop\synrix-windows-build`

The build directory is nested: `synrix-windows-build\build\windows`

## Option 1: Use Quick Build Script (Easiest)

From the current directory, run:
```cmd
build_free_tier_quick.bat
```

This script handles the path navigation automatically.

## Option 2: Navigate Manually

```cmd
cd synrix-windows-build\build\windows
build_free_tier_50k.bat
```

## Option 3: Full Manual Build

```cmd
cd synrix-windows-build\build\windows
mkdir build_free_tier
cd build_free_tier

cmake ..\..\.. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED" ^
    -G "MinGW Makefiles"

cmake --build . --config Release
```

## Output Location

The DLL will be at:
```
synrix-windows-build\build\windows\build_free_tier\bin\libsynrix_free_tier.dll
```
