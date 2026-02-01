@echo off
REM Install Dependencies for Robotics SDK
REM This installs Visual C++ 2013 Runtime and zlib1.dll

echo ========================================
echo Robotics SDK - Dependency Installer
echo ========================================
echo.

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SYNRIX_DIR=%SCRIPT_DIR%synrix"

echo Installing dependencies...
echo.

REM Step 1: Install VC++ 2013 Runtime
echo [1/2] Installing Visual C++ 2013 Runtime...
echo.

REM Check if running as admin
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo WARNING: Not running as Administrator!
    echo VC++ 2013 installation requires admin rights.
    echo.
    echo Please run this script as Administrator:
    echo   1. Right-click install_dependencies.bat
    echo   2. Select "Run as Administrator"
    echo.
    echo Or install manually from:
    echo   https://www.microsoft.com/en-us/download/details.aspx?id=40784
    echo.
    pause
    goto :install_zlib
)

REM Download VC++ 2013 Redistributable
echo Downloading VC++ 2013 Redistributable...
powershell -Command "Invoke-WebRequest -Uri 'https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe' -OutFile '%TEMP%\vcredist_x64.exe'"

if not exist "%TEMP%\vcredist_x64.exe" (
    echo ERROR: Download failed!
    echo Please install manually from:
    echo   https://www.microsoft.com/en-us/download/details.aspx?id=40784
    goto :install_zlib
)

echo Installing VC++ 2013 Runtime...
"%TEMP%\vcredist_x64.exe" /quiet /norestart
echo [OK] VC++ 2013 Runtime installed (or already installed)
echo.

:install_zlib
REM Step 2: Download zlib1.dll
echo [2/2] Downloading zlib1.dll...
echo.

if exist "%SYNRIX_DIR%\zlib1.dll" (
    echo [OK] zlib1.dll already exists
    goto :done
)

powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%download_zlib.ps1"

if exist "%SYNRIX_DIR%\zlib1.dll" (
    echo [OK] zlib1.dll downloaded successfully
) else (
    echo [WARN] zlib1.dll download failed
    echo You may need to download it manually
)

:done
echo.
echo ========================================
echo Dependency Installation Complete
echo ========================================
echo.
echo Next steps:
echo   1. Restart your computer (if VC++ was installed)
echo   2. Run: python setup.py install
echo   3. Test: python examples/robotics_quickstart.py
echo.
pause
