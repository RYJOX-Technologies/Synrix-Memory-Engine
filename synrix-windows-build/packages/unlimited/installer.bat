@echo off
REM SYNRIX One-Click Installer (Default)
REM This is the main installer - uses installer_v2.py for full functionality

REM Check if installer_v2.py exists (improved installer)
if exist "installer_v2.py" (
    echo.
    echo ========================================
    echo   SYNRIX One-Click Installer
    echo ========================================
    echo.
    echo Using improved installer (auto-installs all dependencies)...
    echo.
    python installer_v2.py
    exit /b %errorlevel%
)

REM Fallback to basic installation if installer_v2.py not available
echo.
echo ========================================
echo   SYNRIX Installer (Basic Mode)
echo ========================================
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    echo   Please install Python 3.8+ from https://www.python.org/
    pause
    exit /b 1
)

echo Checking Python version...
python --version
echo.

REM Get script directory (remove trailing backslash if present)
set "SCRIPT_DIR=%~dp0"
set "PACKAGE_DIR=%SCRIPT_DIR%"
if "%PACKAGE_DIR:~-1%"=="\" set "PACKAGE_DIR=%PACKAGE_DIR:~0,-1%"

echo Package directory: %PACKAGE_DIR%
echo.

REM Install package
echo Installing SYNRIX package...
python -m pip install -e "%PACKAGE_DIR%"
if errorlevel 1 (
    echo.
    echo ERROR: Installation failed!
    echo.
    echo For better error handling and automatic dependency installation,
    echo make sure installer_v2.py is in this directory.
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Installation Complete!
echo ========================================
echo.
echo SYNRIX has been successfully installed.
echo.
echo You can now use SYNRIX in your Python scripts:
echo   from synrix.ai_memory import get_ai_memory
echo   memory = get_ai_memory()
echo.
pause
