@echo off
REM Simple build script for SYNRIX CLI
REM Builds synrix.exe that can be called directly from command line

echo Building SYNRIX CLI (synrix.exe)...
echo.

cd /d "%~dp0\build\windows"

REM Rebuild with CLI target
cd build_msys2
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --target synrix_cli

if exist bin\synrix.exe (
    echo.
    echo [OK] Build successful!
    echo CLI: build\windows\build_msys2\bin\synrix.exe
    echo.
    echo Usage:
    echo   synrix.exe add memory.lattice "MEMORY:test" "data"
    echo   synrix.exe get memory.lattice 12345
    echo   synrix.exe query memory.lattice "MEMORY:" 10
) else (
    echo.
    echo [FAIL] Build failed - synrix.exe not found
)

cd ..\..
