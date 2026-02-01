# Download zlib1.dll for Robotics SDK
# This is required by libsynrix.dll

param(
    [string]$TargetDir = $PSScriptRoot
)

Write-Host "=== Downloading zlib1.dll ===" -ForegroundColor Cyan
Write-Host ""

$synrixDir = Join-Path $TargetDir "synrix"
$zlibPath = Join-Path $synrixDir "zlib1.dll"

# Create synrix directory if it doesn't exist
if (-not (Test-Path $synrixDir)) {
    New-Item -ItemType Directory -Path $synrixDir -Force | Out-Null
}

# Check if already exists
if (Test-Path $zlibPath) {
    Write-Host "[OK] zlib1.dll already exists at:" -ForegroundColor Green
    Write-Host "  $zlibPath" -ForegroundColor Gray
    exit 0
}

$tempDir = Join-Path $env:TEMP "zlib_download"
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

Write-Host "Downloading zlib1.dll from MinGW-w64..." -ForegroundColor Yellow

try {
    $zipUrl = "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0-16.0.6-11.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.0-r1.zip"
    $zipPath = Join-Path $tempDir "zlib.zip"
    
    Write-Host "  Downloading from GitHub..." -ForegroundColor Gray
    Invoke-WebRequest -Uri $zipUrl -OutFile $zipPath -ErrorAction Stop
    
    Write-Host "  Extracting zlib1.dll..." -ForegroundColor Gray
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    
    $entry = $zip.Entries | Where-Object { $_.FullName -like "*bin/zlib1.dll" } | Select-Object -First 1
    
    if ($entry) {
        $extractPath = Join-Path $tempDir "zlib1.dll"
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $extractPath, $true)
        $zip.Dispose()
        
        # Copy to synrix directory
        Copy-Item $extractPath $zlibPath -Force
        Write-Host "  [SUCCESS] zlib1.dll downloaded!" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] zlib1.dll not found in zip" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "  [ERROR] Download failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual download:" -ForegroundColor Yellow
    Write-Host "1. Go to: https://github.com/brechtsanders/winlibs_mingw/releases" -ForegroundColor Cyan
    Write-Host "2. Download the latest winlibs-x86_64 zip file" -ForegroundColor Cyan
    Write-Host "3. Extract bin/zlib1.dll" -ForegroundColor Cyan
    Write-Host "4. Copy it to: $zlibPath" -ForegroundColor Cyan
    exit 1
} finally {
    # Cleanup
    Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
Write-Host "zlib1.dll is now at: $zlibPath" -ForegroundColor Cyan
