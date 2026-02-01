@echo off
REM SYNRIX One-Click Installer v2 (Batch Wrapper)
REM Automatically installs all dependencies and SYNRIX

python installer_v2.py
if errorlevel 1 (
    echo.
    echo Installation failed. See error messages above.
    pause
    exit /b 1
)
