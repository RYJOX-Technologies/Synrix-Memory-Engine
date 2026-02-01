# Download zlib1.dll required for libsynrix.dll
# This fixes the "zlib1.dll not found" error

Write-Host "========================================"
Write-Host "  Downloading zlib1.dll"
Write-Host "========================================"
Write-Host ""

# Find synrix directory
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$synrixDir = Join-Path $scriptPath "synrix"
$zlibPath = Join-Path $synrixDir "zlib1.dll"

# Check if already exists
if (Test-Path $zlibPath) {
    Write-Host "[OK] zlib1.dll already exists at: $zlibPath"
    Write-Host ""
    exit 0
}

Write-Host "[INFO] zlib1.dll not found. Downloading..."
Write-Host ""

# Create temp directory
$tempDir = Join-Path $env:TEMP "synrix_zlib"
if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir | Out-Null
}

# Download zlib1.dll from MinGW-w64 binaries
$zlibUrl = "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0-16.0.6-11.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.0-r1.zip"
$zipPath = Join-Path $tempDir "mingw.zip"

Write-Host "Downloading from GitHub..."
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $zlibUrl -OutFile $zipPath -UseBasicParsing
} catch {
    Write-Host "[ERROR] Download failed: $_"
    Write-Host ""
    Write-Host "Please download manually from:"
    Write-Host "https://github.com/brechtsanders/winlibs_mingw/releases"
    Write-Host ""
    Write-Host "Extract zlib1.dll from the bin/ directory and copy it to:"
    Write-Host $synrixDir
    exit 1
}

Write-Host "[OK] Download complete. Extracting..."
Write-Host ""

# Extract zlib1.dll from zip
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
    
    $zlibEntry = $zip.Entries | Where-Object { $_.Name -eq "zlib1.dll" -and $_.FullName -like "*bin*" } | Select-Object -First 1
    
    if ($zlibEntry) {
        $extractPath = Join-Path $tempDir "zlib1.dll"
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($zlibEntry, $extractPath, $true)
        
        # Copy to synrix directory
        Copy-Item $extractPath $zlibPath -Force
        Write-Host "[OK] zlib1.dll installed to: $zlibPath"
    } else {
        Write-Host "[ERROR] zlib1.dll not found in downloaded archive"
        Write-Host ""
        Write-Host "Please download manually from:"
        Write-Host "https://github.com/brechtsanders/winlibs_mingw/releases"
        exit 1
    }
    
    $zip.Dispose()
} catch {
    Write-Host "[ERROR] Extraction failed: $_"
    Write-Host ""
    Write-Host "Please download manually from:"
    Write-Host "https://github.com/brechtsanders/winlibs_mingw/releases"
    exit 1
}

# Cleanup
Remove-Item $zipPath -ErrorAction SilentlyContinue
Remove-Item $tempDir -Recurse -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "[OK] zlib1.dll installation complete!"
Write-Host ""
