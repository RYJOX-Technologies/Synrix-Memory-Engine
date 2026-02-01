@echo off
REM Install Visual C++ 2013 Redistributable (x64) required for libsynrix.dll
REM This fixes the "msvcr120.dll not found" error

echo ========================================
echo  Installing Visual C++ 2013 Runtime
echo ========================================
echo.

REM Check if already installed
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\12.0\VC\Runtimes\x64" >nul 2>&1
if %errorlevel% == 0 (
    echo [OK] Visual C++ 2013 Runtime is already installed.
    echo.
    pause
    exit /b 0
)

echo [INFO] Visual C++ 2013 Runtime not found. Downloading installer...
echo.

REM Create temp directory
set TEMP_DIR=%TEMP%\synrix_vc2013
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

REM Download VC++ 2013 Redistributable
echo Downloading from Microsoft...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe' -OutFile '%TEMP_DIR%\vcredist_x64.exe'}"

if not exist "%TEMP_DIR%\vcredist_x64.exe" (
    echo [ERROR] Download failed!
    echo.
    echo Please download manually from:
    echo https://www.microsoft.com/en-us/download/details.aspx?id=40784
    echo.
    pause
    exit /b 1
)

echo [OK] Download complete. Installing...
echo.

REM Install silently
"%TEMP_DIR%\vcredist_x64.exe" /install /quiet /norestart

if %errorlevel% == 0 (
    echo [OK] Visual C++ 2013 Runtime installed successfully!
    echo.
    echo You may need to restart your terminal/Python for changes to take effect.
) else (
    echo [ERROR] Installation failed with error code %errorlevel%
    echo.
    echo Please install manually from:
    echo https://www.microsoft.com/en-us/download/details.aspx?id=40784
)

REM Cleanup
del "%TEMP_DIR%\vcredist_x64.exe" >nul 2>&1
rmdir "%TEMP_DIR%" >nul 2>&1

echo.
pause
