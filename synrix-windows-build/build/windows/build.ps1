# Build SYNRIX DLL for Windows
# Run this from the build/windows directory

param(
    [string]$BuildType = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  SYNRIX Windows Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check for CMake
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCmd) {
    Write-Host "ERROR: CMake not found!" -ForegroundColor Red
    Write-Host "  Install from: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "  Or install via: winget install Kitware.CMake" -ForegroundColor Yellow
    exit 1
}
Write-Host "OK: CMake found: $($cmakeCmd.Source)" -ForegroundColor Green

# Create build directory
$buildDir = "build"
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

# Configure
Write-Host ""
Write-Host "Configuring CMake..." -ForegroundColor Yellow
Write-Host "  Generator: $Generator" -ForegroundColor Gray
Write-Host "  Build Type: $BuildType" -ForegroundColor Gray

Set-Location $buildDir

# Configure based on generator
if ($Generator -like "*Visual Studio*") {
    cmake .. -G "$Generator" -A x64
} else {
    cmake .. -G "$Generator" -DCMAKE_BUILD_TYPE=$BuildType
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

# Build
Write-Host ""
Write-Host "Building..." -ForegroundColor Yellow
if ($Generator -like "*Visual Studio*") {
    cmake --build . --config $BuildType
} else {
    cmake --build .
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

# Find output
Write-Host ""
Write-Host "Locating output..." -ForegroundColor Yellow
$dllPath = $null

if ($Generator -like "*Visual Studio*") {
    $dllPath = Get-ChildItem -Recurse -Filter "libsynrix.dll" | Where-Object { $_.DirectoryName -like "*$BuildType*" } | Select-Object -First 1
} else {
    $dllPath = Get-ChildItem -Recurse -Filter "libsynrix.dll" | Select-Object -First 1
}

Set-Location ..

if ($dllPath) {
    Write-Host "OK: Build successful!" -ForegroundColor Green
    Write-Host "  DLL: $($dllPath.FullName)" -ForegroundColor White
    Write-Host "  Size: $([math]::Round($dllPath.Length / 1KB, 2)) KB" -ForegroundColor Gray
} else {
    Write-Host "WARNING: Build completed but DLL not found" -ForegroundColor Yellow
    Write-Host "  Check build directory: $buildDir" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Copy libsynrix.dll to your Python SDK" -ForegroundColor White
Write-Host "  2. Update Python SDK to find Windows DLL" -ForegroundColor White
Write-Host "  3. Test with: python -c 'from synrix.raw_backend import RawSynrixBackend'" -ForegroundColor White
Write-Host ""
