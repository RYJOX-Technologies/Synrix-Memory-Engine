# Create password-protected zip with the license signing key for Joseph.
# Part 1: send key_for_joseph.zip. Part 2: send the password (in PASSWORD_FOR_JOSEPH.txt) via a different channel.
# Run from: build/windows

$ErrorActionPreference = "Stop"
$outDir = Join-Path $PSScriptRoot "key_for_joseph_output"
$keyFile = Join-Path $outDir "LICENSE_SIGNING_PRIVATE_KEY.txt"
$zipPath = Join-Path $PSScriptRoot "key_for_joseph.zip"
$passwordFile = Join-Path $PSScriptRoot "PASSWORD_FOR_JOSEPH.txt"

# The one line Joseph needs for: supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=...
$keyLine = "MC4CAQAwBQYDK2VwBCIEIFXfeC1UKs8yb2pwqhnciptuP5l3GLL8yHeUVNudwUKf"

# Strong random password (24 chars: letters + digits + 2 symbols)
$chars = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789!@"
$password = -join ((1..24) | ForEach-Object { $chars[(Get-Random -Maximum $chars.Length)] })

# Create temp dir and key file
if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }
New-Item -ItemType Directory -Path $outDir | Out-Null
Set-Content -Path $keyFile -Value $keyLine -NoNewline

# Find 7-Zip
$7z = $null
foreach ($p in @("C:\Program Files\7-Zip\7z.exe", "C:\Program Files (x86)\7-Zip\7z.exe")) {
    if (Test-Path $p) { $7z = $p; break }
}

if (-not $7z) {
    Set-Content -Path $passwordFile -Value $password
    Write-Host "7-Zip not found. Two options:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  A) Install 7-Zip (https://www.7-zip.org/), then run this script again. It will create the zip + a new password in PASSWORD_FOR_JOSEPH.txt." -ForegroundColor Gray
    Write-Host ""
    Write-Host "  B) Zip the key file yourself with any tool that supports password:" -ForegroundColor Gray
    Write-Host "     Key file: $keyFile" -ForegroundColor Gray
    Write-Host "     Password (Part 2) saved to: $passwordFile" -ForegroundColor Gray
    Write-Host "     Content: $password" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

# Create password-protected zip
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
& $7z a -tzip -p$password $zipPath $keyFile | Out-Null
if (-not (Test-Path $zipPath)) {
    Write-Host "Failed to create zip." -ForegroundColor Red
    exit 1
}

# Remove temp key file and dir
Remove-Item $keyFile -Force -ErrorAction SilentlyContinue
Remove-Item $outDir -Recurse -Force -ErrorAction SilentlyContinue

# Save password for Part 2
Set-Content -Path $passwordFile -Value $password

Write-Host "Done." -ForegroundColor Green
Write-Host ""
Write-Host "Part 1 (send to Joseph): $zipPath" -ForegroundColor Cyan
Write-Host "Part 2 (send via different channel): open $passwordFile and send him the password." -ForegroundColor Cyan
Write-Host ""
Write-Host "Joseph: unzip with the password, then run:" -ForegroundColor Gray
Write-Host "  supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=<the one line inside the zip>" -ForegroundColor Gray
Write-Host ""
Write-Host "Password also saved to: $passwordFile" -ForegroundColor Gray
