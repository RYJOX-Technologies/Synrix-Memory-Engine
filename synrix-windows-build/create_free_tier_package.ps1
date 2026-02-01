# Create Free Tier Package for Distribution
# This creates a zip file with DLL, dependencies, and Python SDK

$ErrorActionPreference = "Stop"

$PackageName = "synrix-free-tier-50k-windows"
$Version = "1.0.0"
$OutputDir = "dist\free_tier_package"
$ZipFile = "dist\${PackageName}-${Version}.zip"

Write-Host "========================================"
Write-Host "Creating SYNRIX Free Tier Package"
Write-Host "========================================"
Write-Host ""

# Create output directory
if (Test-Path $OutputDir) {
    Remove-Item -Recurse -Force $OutputDir
}
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
New-Item -ItemType Directory -Path "$OutputDir\bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$OutputDir\python-sdk" -Force | Out-Null

# Copy DLL
$DllSource = "build\windows\build_free_tier\bin\libsynrix_free_tier.dll"
if (Test-Path $DllSource) {
    Copy-Item $DllSource "$OutputDir\bin\libsynrix_free_tier.dll"
    Write-Host "OK: Copied DLL"
} else {
    Write-Host "ERROR: DLL not found at $DllSource"
    Write-Host "Build the free tier DLL first: bash build/windows/build_free_tier_50k.sh"
    exit 1
}

# Copy dependencies (from python-sdk or build directory)
$Dependencies = @(
    "python-sdk\libgcc_s_seh-1.dll",
    "python-sdk\libwinpthread-1.dll",
    "python-sdk\zlib1.dll"
)

foreach ($dep in $Dependencies) {
    if (Test-Path $dep) {
        Copy-Item $dep "$OutputDir\bin\"
        Write-Host "OK: Copied dependency: $(Split-Path $dep -Leaf)"
    } else {
        Write-Host "WARNING: Dependency not found: $dep"
    }
}

# Copy Python SDK (essential files only)
$SdkFiles = @(
    "python-sdk\synrix",
    "python-sdk\setup.py",
    "python-sdk\README.md"
)

foreach ($file in $SdkFiles) {
    if (Test-Path $file) {
        if (Test-Path $file -PathType Container) {
            Copy-Item -Recurse $file "$OutputDir\python-sdk\"
        } else {
            Copy-Item $file "$OutputDir\python-sdk\"
        }
        Write-Host "OK: Copied SDK: $(Split-Path $file -Leaf)"
    }
}

# Create README
$Readme = @"
SYNRIX Free Tier Package (50k Node Limit)
==========================================

This package contains:
- libsynrix_free_tier.dll (Windows DLL with 50k node limit)
- Required dependencies (MinGW runtime DLLs)
- Python SDK

Installation:
-------------

1. Extract this zip file to a directory (e.g., C:\synrix-free-tier)

2. Install Python SDK:
   cd python-sdk
   pip install -e .

3. Test installation:
   python -c "from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('test.lattice'); print('OK: SYNRIX loaded')"

Features:
---------
- Hard-coded 50,000 node limit
- Evaluation mode always enabled
- Cannot disable evaluation mode
- All SYNRIX features available within limit

Usage:
------
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("memory.lattice", max_nodes=100000)
node_id = backend.add_node("PATTERN:test", "data", node_type=5)
results = backend.find_by_prefix("PATTERN:", limit=10)

# Limit: Can add up to 50,000 nodes
# After 50,000 nodes, add_node() will return 0 and show limit message

backend.close()

Files:
------
bin/
  - libsynrix_free_tier.dll (main library)
  - libgcc_s_seh-1.dll (MinGW runtime)
  - libwinpthread-1.dll (MinGW runtime)
  - zlib1.dll (compression library)

python-sdk/
  - synrix/ (Python SDK)
  - setup.py (install script)

License:
--------
See LICENSE file or visit synrix.io for license information.
"@

Set-Content -Path "$OutputDir\README.txt" -Value $Readme
Write-Host "OK: Created README.txt"

# Create install script
$InstallScript = @"
@echo off
REM SYNRIX Free Tier Installer
echo ========================================
echo SYNRIX Free Tier Installation
echo ========================================
echo.

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    echo Please install Python 3.8+ from python.org
    pause
    exit /b 1
)

echo Installing Python SDK...
cd python-sdk
pip install -e . >nul 2>&1
if errorlevel 1 (
    echo ERROR: Installation failed. Trying with output...
    pip install -e .
    pause
    exit /b 1
)
cd ..

echo.
echo ========================================
echo Installation Complete!
echo ========================================
echo.
echo Test: python -c "from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('test.lattice'); print('SUCCESS')"
echo.
pause
"@

Set-Content -Path "$OutputDir\install.bat" -Value $InstallScript
Write-Host "OK: Created install.bat"

# Create INSTALL.txt (simpler quick guide)
$InstallTxt = @"
SYNRIX Free Tier - Quick Install Guide
======================================

INSTALLATION (2 steps):
-----------------------

1. Extract this zip file to any folder

2. Double-click install.bat

That's it! SYNRIX is now installed.

TEST IT:
--------
python -c "from synrix.raw_backend import RawSynrixBackend; b = RawSynrixBackend('test.lattice'); print('SUCCESS: SYNRIX loaded!')"

WHAT'S INCLUDED:
----------------
- libsynrix_free_tier.dll (50k node limit, hard-coded)
- Required dependencies (MinGW runtime DLLs)
- Python SDK (full-featured)
- This installer

FEATURES:
---------
- 50,000 node limit (hard-coded, cannot be changed)
- Evaluation mode always enabled
- All SYNRIX features available within limit
- Persistent memory across sessions
- Sub-millisecond performance

USAGE:
------
from synrix.raw_backend import RawSynrixBackend

# Create/open lattice
backend = RawSynrixBackend("memory.lattice", max_nodes=100000)

# Add nodes (up to 50,000)
node_id = backend.add_node("PATTERN:my_pattern", "pattern data", node_type=5)

# Query by prefix
results = backend.find_by_prefix("PATTERN:", limit=10)

# Close when done
backend.close()

LIMIT:
------
After 50,000 nodes, add_node() will return 0 and display a limit message.
Delete nodes or upgrade to Pro tier for unlimited nodes.

SUPPORT:
--------
Visit synrix.io for documentation and support.
"@

Set-Content -Path "$OutputDir\INSTALL.txt" -Value $InstallTxt
Write-Host "OK: Created INSTALL.txt"

# Create zip file
Write-Host ""
Write-Host "Creating zip package..."
if (Test-Path $ZipFile) {
    Remove-Item $ZipFile
}

# Use PowerShell compression
Compress-Archive -Path "$OutputDir\*" -DestinationPath $ZipFile -Force
Write-Host "OK: Created $ZipFile"

Write-Host ""
Write-Host "========================================"
Write-Host "Package Created Successfully!"
Write-Host "========================================"
Write-Host ""
Write-Host "Package: $ZipFile"
Write-Host "Size: $((Get-Item $ZipFile).Length / 1MB) MB"
Write-Host ""
Write-Host "Contents:"
Write-Host "  - libsynrix_free_tier.dll"
Write-Host "  - Dependencies (MinGW runtime)"
Write-Host "  - Python SDK"
Write-Host "  - install.bat (installer script)"
Write-Host "  - README.txt (instructions)"
Write-Host ""
Write-Host "Send this zip file to your cofounder!"
Write-Host ""
