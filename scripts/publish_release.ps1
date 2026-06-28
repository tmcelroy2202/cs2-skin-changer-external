param(
    [string]$Version = "1.2.3",
    [string]$Repo = "mrch4n725/cs2-skin-changer-external"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$tag = "v$Version"
$exe = Join-Path $root "build\Release\CS2SkinChanger.exe"

Write-Host "Publishing $tag to $Repo" -ForegroundColor Cyan

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw "GitHub CLI (gh) is required. Install with: winget install GitHub.cli"
}

gh auth status | Out-Null

if (-not (Test-Path $exe)) {
    Write-Host "Building Release binary..." -ForegroundColor Yellow
    Push-Location $root
    if (-not (Test-Path "third_party\imgui\imgui.h")) {
        git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party\imgui
    }
    cmake --preset windows-release
    cmake --build --preset windows-release
    Pop-Location
}

if (-not (Test-Path $exe)) {
    throw "Build output not found: $exe"
}

$notes = @"
## CS2 Skin Changer $tag — Skin Application Fix

### Fixed
- Updated all CS2 memory offsets to current cs2-dumper values (June 2026)
- Auto-sync offsets from GitHub on startup when online
- Fixed skin application flow: item ID high/low, DisallowSOC, mesh mask, RegenerateWeaponSkins patch
- Fixed weapon enumeration and active weapon detection
- Added Apply to Equipped Weapon button in overlay

### Usage
1. Launch CS2 and join a match
2. Run CS2SkinChanger.exe as Administrator
3. Press Insert, select weapon + skin — changes apply automatically
"@

$bodyFile = Join-Path $env:TEMP "cs2-skin-changer-release-notes.md"
Set-Content -Path $bodyFile -Value $notes -Encoding UTF8

Write-Host "Creating GitHub release $tag..." -ForegroundColor Green
gh release create $tag $exe `
    --repo $Repo `
    --title "v$Version — Skin Application Fix" `
    --notes-file $bodyFile

Write-Host "Done: https://github.com/$Repo/releases/tag/$tag" -ForegroundColor Green
