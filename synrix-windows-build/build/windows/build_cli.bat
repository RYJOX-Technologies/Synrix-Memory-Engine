@echo off
REM Build SYNRIX CLI executable for Windows
REM This creates synrix.exe that can be called directly from command line

echo ========================================
echo Building SYNRIX CLI (synrix.exe)
echo ========================================
echo.

cd /d "%~dp0"

REM Use MSYS2 build directory
set BUILD_DIR=build_msys2
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo Configuring CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    echo Make sure MSYS2 MinGW-w64 is in PATH
    exit /b 1
)

echo.
echo Building...
cmake --build . --target synrix_cli
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build complete!
echo ========================================
echo.
echo CLI executable: %BUILD_DIR%\bin\synrix.exe
echo.
echo Usage:
echo   synrix.exe add memory.lattice "MEMORY:test" "data"
echo   synrix.exe get memory.lattice 12345
echo   synrix.exe query memory.lattice "MEMORY:" 10
echo.

cd ..
