@echo off
REM Build script for pure C benchmark on Windows (MSYS2)

echo ========================================
echo   Building SYNRIX Pure C Benchmark
echo ========================================
echo.

set BUILD_DIR=build\windows\build_msys2
set BIN_DIR=%BUILD_DIR%\bin
set INCLUDE_DIR=include

if not exist "%BIN_DIR%\libsynrix.dll" (
    echo ERROR: Synrix library not found at %BIN_DIR%\libsynrix.dll
    echo Please build the library first: cd build\windows ^&^& bash build_msys2.sh
    exit /b 1
)

echo Compiling benchmark...
gcc -O3 -I%INCLUDE_DIR% -o benchmark_c_raw.exe benchmark_c_raw.c -L%BIN_DIR% -lsynrix -lz

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo.
    echo Run with: benchmark_c_raw.exe [node_count] [lookup_count] [name_lookup_count]
    echo Example: benchmark_c_raw.exe 10000 1000 100
) else (
    echo.
    echo Build failed
    exit /b 1
)
