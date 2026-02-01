@echo off
REM Quick build script for free tier - runs from project root

setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0synrix-windows-build
set BUILD_DIR=%PROJECT_ROOT%\build\windows\build_free_tier

echo ========================================
echo SYNRIX Free Tier Build (50k Node Limit)
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo Install with: pacman -S mingw-w64-x86_64-cmake
    exit /b 1
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure with free tier defines
echo Configuring CMake with free tier settings...
cmake "%PROJECT_ROOT%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED" ^
    -G "MinGW Makefiles"

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build
echo.
echo Building free tier DLL...
cmake --build . --config Release

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM Check if DLL was created
if exist "bin\libsynrix_free_tier.dll" (
    echo.
    echo ========================================
    echo Free Tier Build Successful!
    echo ========================================
    echo.
    echo Output: %BUILD_DIR%\bin\libsynrix_free_tier.dll
    echo.
    echo Features:
    echo   - Hard-coded 50,000 node limit
    echo   - Evaluation mode always enabled
    echo   - Cannot disable evaluation mode
    echo.
) else (
    echo.
    echo ERROR: DLL not found at expected location
    exit /b 1
)
