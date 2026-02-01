@echo off
REM SYNRIX Free Tier Installer (Batch File)
REM Double-click this file to install SYNRIX

echo.
echo ========================================
echo   SYNRIX Free Tier Installer
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

REM Get script directory
set "SCRIPT_DIR=%~dp0"
set "PACKAGE_DIR=%SCRIPT_DIR%"

echo Package directory: %PACKAGE_DIR%
echo.

REM Install package
echo Installing SYNRIX package...
python -m pip install -e "%PACKAGE_DIR%"
if errorlevel 1 (
    echo.
    echo ERROR: Installation failed!
    pause
    exit /b 1
)

echo.
echo Testing installation...
python -c "from synrix.ai_memory import get_ai_memory; m = get_ai_memory(); m.add('TEST:install', 'Works!'); print('Installation test passed!')"
if errorlevel 1 (
    echo.
    echo WARNING: Installation test failed, but package may still work.
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
