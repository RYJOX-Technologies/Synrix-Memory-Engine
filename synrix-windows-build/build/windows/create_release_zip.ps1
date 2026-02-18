# Create synrix-windows.zip for GitHub Release.
# Run from: build/windows (after you've run build_msys2.sh).
# Requires: DLL at build_msys2/bin/libsynrix.dll; MSYS2 at C:\msys64\mingw64\bin for runtime DLLs.

$ErrorActionPreference = "Stop"
$buildBin = Join-Path $PSScriptRoot "build_msys2\bin"
# Use env SYNRIX_MSYS2_BIN to override (e.g. D:\msys64\mingw64\bin)
$mingw = if ($env:SYNRIX_MSYS2_BIN) { $env:SYNRIX_MSYS2_BIN.TrimEnd('\') } else { "C:\msys64\mingw64\bin" }
$outDir = Join-Path $PSScriptRoot "release_stage"
$zipPath = Join-Path $PSScriptRoot "synrix-windows.zip"

if (-not (Test-Path (Join-Path $buildBin "libsynrix.dll"))) {
    Write-Host "ERROR: libsynrix.dll not found. Run: bash build_msys2.sh" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $mingw)) {
    Write-Host "WARNING: MSYS2 bin not at $mingw - zip will contain only libsynrix.dll (may need runtimes on target)." -ForegroundColor Yellow
    Write-Host "  Set SYNRIX_MSYS2_BIN to your path, e.g. D:\msys64\mingw64\bin" -ForegroundColor Gray
    $mingw = $null
} else {
    Write-Host "Using MSYS2 bin: $mingw" -ForegroundColor Gray
}

# Clean and create stage dir
if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }
New-Item -ItemType Directory -Path $outDir | Out-Null

# Copy our build
Copy-Item (Join-Path $buildBin "libsynrix.dll") $outDir
if (Test-Path (Join-Path $buildBin "synrix.exe")) {
    Copy-Item (Join-Path $buildBin "synrix.exe") $outDir
}

# Copy runtime DLLs from MSYS2 (needed for license verification + zlib)
$runtimeDlls = @(
    "libgcc_s_seh-1.dll",
    "libwinpthread-1.dll",
    "zlib1.dll"
)
if ($mingw) {
    foreach ($dll in $runtimeDlls) {
        $src = Join-Path $mingw $dll
        if (Test-Path $src) { Copy-Item $src $outDir }
        else { Write-Host "  (skip $dll - not found)" -ForegroundColor Gray }
    }
    # OpenSSL: copy by glob so we get whatever names MSYS2 uses (e.g. libssl-3-x64.dll)
    $ssl = Get-ChildItem -Path $mingw -Filter "libssl*.dll" -ErrorAction SilentlyContinue
    $crypto = Get-ChildItem -Path $mingw -Filter "libcrypto*.dll" -ErrorAction SilentlyContinue
    foreach ($f in $ssl) { Copy-Item $f.FullName $outDir; Write-Host "  + $($f.Name)" -ForegroundColor Green }
    foreach ($f in $crypto) { Copy-Item $f.FullName $outDir; Write-Host "  + $($f.Name)" -ForegroundColor Green }
    if (-not $ssl -and -not $crypto) {
        Write-Host "  OpenSSL DLLs not found in $mingw" -ForegroundColor Yellow
    }
}

# Create zip
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $outDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "OK: $zipPath" -ForegroundColor Green
Write-Host "  Upload this file to your GitHub Release." -ForegroundColor Cyan
# Optional: remove stage dir
Remove-Item $outDir -Recurse -Force -ErrorAction SilentlyContinue
