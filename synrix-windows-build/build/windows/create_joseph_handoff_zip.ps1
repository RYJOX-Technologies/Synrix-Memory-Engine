# Create Joseph_handoff.zip with everything Joseph needs (website, billing, Supabase).
# No private keys - send the signing key separately via create_key_zip_for_joseph.ps1.
# Run from: build/windows (or from repo root; script uses $PSScriptRoot).

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$zipName = "Joseph_handoff.zip"
$zipPath = Join-Path $root $zipName
$tempDir = Join-Path $env:TEMP "joseph_handoff_$(Get-Random)"

# Parent of build/windows (repo root) - use the one that contains FOR_JOSEPH_PACK.md
$repoRoot = $null
foreach ($candidate in @((Join-Path $root ".."), (Join-Path $root "..\.."))) {
    $candidate = (Resolve-Path $candidate -ErrorAction SilentlyContinue).Path
    if ($candidate -and (Test-Path (Join-Path $candidate "FOR_JOSEPH_PACK.md"))) { $repoRoot = $candidate; break }
}
if (-not $repoRoot) { $repoRoot = (Resolve-Path (Join-Path $root "..")).Path }

New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

# Copy docs and tools (flat in zip so he has one folder to open)
$items = @(
    @{ From = Join-Path $root "KEY_FORMAT_FOR_JOSEPH.md"; Name = "KEY_FORMAT_FOR_JOSEPH.md" },
    @{ From = Join-Path $root "LICENSE_SIGNED_KEY.md"; Name = "LICENSE_SIGNED_KEY.md" },
    @{ From = Join-Path $root "tools\synrix_license_keygen.py"; Name = "synrix_license_keygen.py" },
    @{ From = Join-Path $repoRoot "FOR_JOSEPH_PACK.md"; Name = "FOR_JOSEPH_PACK.md" },
    @{ From = Join-Path $repoRoot "SUMMARY_FOR_JOSEPH.md"; Name = "SUMMARY_FOR_JOSEPH.md" },
    @{ From = Join-Path $repoRoot "RESPONSE_TO_JOSEPH.md"; Name = "RESPONSE_TO_JOSEPH.md" },
    @{ From = Join-Path $repoRoot "USER_JOURNEY_AND_FLOW.md"; Name = "USER_JOURNEY_AND_FLOW.md" },
    @{ From = Join-Path $repoRoot "QUICK_REFERENCE_JOSEPH.md"; Name = "QUICK_REFERENCE_JOSEPH.md" },
    @{ From = Join-Path $repoRoot "ALIGNMENT_JOSEPH_BACKEND.md"; Name = "ALIGNMENT_JOSEPH_BACKEND.md" },
    @{ From = Join-Path $repoRoot "USAGE_REPORT_FOR_JOSEPH.md"; Name = "USAGE_REPORT_FOR_JOSEPH.md" }
)

foreach ($item in $items) {
    if (Test-Path $item.From) {
        Copy-Item -Path $item.From -Destination (Join-Path $tempDir $item.Name) -Force
    }
}

# README: start here
$readme = @"
JOSEPH – START HERE
===================

Engine and SDKs are done. Your part: website, billing, Supabase.

1. KEY FORMAT & SIGNING
   - KEY_FORMAT_FOR_JOSEPH.md  = exact key format the engine expects
   - LICENSE_SIGNED_KEY.md     = how signing works
   - You need the private key in Supabase (Ryan sends it separately via create_key_zip_for_joseph.ps1 + password in another channel).

2. ISSUING KEYS (after payment)
   - Run: python synrix_license_keygen.py --tier <25k|1m|10m|50m|unlimited> --private <path or use env from Supabase>
   - Send the one-line output to the customer. They set SYNRIX_LICENSE_KEY to that value.

3. FULL INDEX
   - FOR_JOSEPH_PACK.md = index of all docs (user journey, DB format, Q&A).
   - SUMMARY_FOR_JOSEPH.md = one-pager for exec/partners.
   - USER_JOURNEY_AND_FLOW.md = free vs paid flow, upgrade, expiry.
   - RESPONSE_TO_JOSEPH.md = answers on free tier, key sharing, UX, expiry.

4. GLOBAL CAP (new)
   - One node cap per license per machine across all lattices/processes. No backend change needed; just issue keys in the format above.

No private keys in this zip. Get the signing key from Ryan separately (password-protected zip + password in another channel).
"@
Set-Content -Path (Join-Path $tempDir "README_START_HERE.txt") -Value $readme -Encoding UTF8

# Create zip (PowerShell 5+)
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $tempDir "*") -DestinationPath $zipPath -Force
Remove-Item $tempDir -Recurse -Force

Write-Host "Done." -ForegroundColor Green
Write-Host ""
Write-Host "Send this to Joseph: $zipPath" -ForegroundColor Cyan
Write-Host "No private key inside. Send the signing key separately (e.g. create_key_zip_for_joseph.ps1)." -ForegroundColor Gray
