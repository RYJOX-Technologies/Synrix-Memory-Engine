@echo off
REM Simple batch file build script for Windows
REM Usage: build.bat [Release|Debug]

setlocal

set BUILD_TYPE=Release
if not "%1"=="" set BUILD_TYPE=%1

echo ========================================
echo SYNRIX Windows Build
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo Install from: https://cmake.org/download/
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

REM Find DLL
echo.
echo Searching for libsynrix.dll...
for /r %%f in (libsynrix.dll) do (
    echo Found: %%f
    goto :found
)

:found
echo.
echo Build complete!
echo.

endlocal
