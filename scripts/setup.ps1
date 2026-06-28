param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Write-Host "=== CS2 Skin Changer - Setup ===" -ForegroundColor Cyan
Write-Host "Root: $root`n" -ForegroundColor Gray

# ── Prerequisites ──────────────────────────────────────────────────────────────
function Check-Command($name, $url, $hint) {
    if (!(Get-Command $name -ErrorAction SilentlyContinue)) {
        Write-Host "[MISSING] $name" -ForegroundColor Red
        Write-Host "         $hint" -ForegroundColor Gray
        Write-Host "         $url" -ForegroundColor Blue
        return $false
    }
    Write-Host "[OK]      $name" -ForegroundColor Green
    return $true
}

$allOk = $true
$allOk = (Check-Command cmake "https://cmake.org/download/" `
    "Download & install CMake 3.20+ (add to PATH)") -and $allOk

$vs = Check-Command msbuild "https://visualstudio.microsoft.com/vs/" `
    "Install Visual Studio 2022 with 'Desktop development with C++' workload"
$cl = Check-Command cl "https://visualstudio.microsoft.com/vs/" `
    "Install Visual Studio 2022 with 'Desktop development with C++' workload"
if (!$vs -and !$cl) { $allOk = $false }

$git = Check-Command git "https://git-scm.com/downloads" `
    "Install Git for Windows"

if (!$allOk) {
    Write-Host "`nSome prerequisites are missing." -ForegroundColor Yellow
    exit 1
}

# ── Dear ImGui ─────────────────────────────────────────────────────────────────
$imguiDir = "$root\third_party\imgui"
if (!(Test-Path "$imguiDir\imgui.h")) {
    Write-Host "`n[..]     Fetching Dear ImGui..." -ForegroundColor Yellow
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        Write-Host "Git is required to clone Dear ImGui." -ForegroundColor Red
        exit 1
    }
    git clone --depth 1 --branch v1.91.8 `
        https://github.com/ocornut/imgui.git "$imguiDir" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to clone ImGui." -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK]      Dear ImGui v1.91.8" -ForegroundColor Green
} else {
    Write-Host "[OK]      Dear ImGui (found in third_party/imgui)" -ForegroundColor Green
}

# ── Build directory ────────────────────────────────────────────────────────────
$buildDir = "$root\build"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

Write-Host "`n=== Configuration Complete ===" -ForegroundColor Cyan

# ── CMake configure (opt-out via -NoBuild) ─────────────────────────────────────
if (!$NoBuild) {
    Write-Host "`n[..]     Running CMake configure..." -ForegroundColor Yellow
    $gen = if ($cl) { "Visual Studio 17 2022" } else { "Ninja" }
    $arch = "x64"

    cmake -S $root -B $buildDir `
        -G "$gen" `
        -A $arch `
        -DS2RF_BUILD_STANDALONE=ON

    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed." -ForegroundColor Red
        exit 1
    }

    Write-Host "`n[..]     Building..." -ForegroundColor Yellow
    cmake --build $buildDir --config Release

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed." -ForegroundColor Red
        exit 1
    }

    Write-Host "[OK]      Build succeeded (Release)" -ForegroundColor Green
}

# ── Done ───────────────────────────────────────────────────────────────────────
Write-Host @"

===========================
 Setup Complete
===========================

Binary:        $buildDir\Release\S2ResearchFramework.exe
Standalone:    YES (external overlay, no injection)
CS2 process:   cs2.exe

Usage:
  1. Launch Counter-Strike 2
  2. Run S2ResearchFramework.exe as Administrator
  3. Select skins in the overlay window
  4. Changes apply automatically

To rebuild:
  cmake --build $buildDir --config Release

To switch to injected DLL mode:
  cmake -B $buildDir -DS2RF_BUILD_STANDALONE=OFF
  cmake --build $buildDir --config Release

"@ -ForegroundColor Cyan
