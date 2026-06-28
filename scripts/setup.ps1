param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Write-Host "=== CS2 Skin Changer - Setup ===" -ForegroundColor Cyan
Write-Host "Root: $root`n" -ForegroundColor Gray

# -- Helper: prompt yes/no ------------------------------------------------------
function Confirm-Install($name) {
    $response = Read-Host "`n$name is missing. Install it now? [Y/n]"
    return ($response -eq "" -or $response -eq "y" -or $response -eq "Y")
}

# -- winget wrapper -------------------------------------------------------------
function Install-WithWinget($wingetId, $displayName) {
    Write-Host "Installing $displayName via winget..." -ForegroundColor Yellow
    & winget install --id $wingetId --accept-package-agreements --silent 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK]      $displayName installed" -ForegroundColor Green
        return $true
    }
    Write-Host "[FAIL]    $displayName install failed (exit $LASTEXITCODE)" -ForegroundColor Red
    return $false
}

# -- Check for winget (Windows 10 1809+) ----------------------------------------
$hasWinget = (Get-Command winget -ErrorAction SilentlyContinue) -ne $null
if (-not $hasWinget) {
    Write-Host "[INFO]    winget not found - installs will fall back to manual guidance" -ForegroundColor Yellow
}

# -- Prerequisites --------------------------------------------------------------
function Check-And-Install($name, $wingetId, $url, $hint) {
    $found = (Get-Command $name -ErrorAction SilentlyContinue) -ne $null
    if ($found) {
        Write-Host "[OK]      $name" -ForegroundColor Green
        return $true
    }
    Write-Host "[MISSING] $name" -ForegroundColor Red
    if ($hasWinget -and $wingetId -and (Confirm-Install $name)) {
        $ok = Install-WithWinget $wingetId $name
        if (-not $ok) { return $false }
        # Re-check (winget may not update PATH for current session)
        return (Get-Command $name -ErrorAction SilentlyContinue) -ne $null
    }
    Write-Host "         $hint" -ForegroundColor Gray
    Write-Host "         $url" -ForegroundColor Blue
    return $false
}

$allOk = $true

$allOk = (Check-And-Install cmake "Kitware.CMake" `
    "https://cmake.org/download/" `
    'Download and install CMake 3.20+ (ensure "Add to PATH" during install)') -and $allOk

$hasMsBuild = (Get-Command msbuild -ErrorAction SilentlyContinue) -ne $null
$hasCl      = (Get-Command cl -ErrorAction SilentlyContinue) -ne $null
if ($hasMsBuild -or $hasCl) {
    Write-Host '[OK]      Visual Studio 2022 (C++ tools)' -ForegroundColor Green
} else {
    Write-Host '[MISSING] Visual Studio 2022 with "Desktop development with C++"' -ForegroundColor Red
    if ($hasWinget -and (Confirm-Install "Visual Studio 2022 Community")) {
        Write-Host "Installing Visual Studio 2022 Community via winget..." -ForegroundColor Yellow
        & winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --silent 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK]      VS2022 Community installed" -ForegroundColor Green
            Write-Host '[..]     Adding "Desktop development with C++" workload...' -ForegroundColor Yellow
            $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
            if (Test-Path $vsInstaller) {
                & $vsInstaller modify --installPath "$env:ProgramFiles\Microsoft Visual Studio\2022\Community" --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive --norestart --wait
                if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq 3010) {
                    Write-Host '[OK]      C++ workload added' -ForegroundColor Green
                } else {
                    Write-Host '[WARN]    C++ workload may need manual setup (exit' $LASTEXITCODE ')' -ForegroundColor Yellow
                }
            } else {
                Write-Host '[WARN]    VS installer not found at expected path - add C++ workload manually' -ForegroundColor Yellow
                Write-Host '         Run: "vs_installer.exe modify --add Microsoft.VisualStudio.Workload.NativeDesktop"' -ForegroundColor Blue
            }
        } else {
            Write-Host "[FAIL]    VS2022 install failed (exit $LASTEXITCODE)" -ForegroundColor Red
            $allOk = $false
        }
    } else {
        Write-Host "         https://visualstudio.microsoft.com/vs/" -ForegroundColor Blue
        $allOk = $false
    }
}

$allOk = (Check-And-Install git "Git.Git" `
    "https://git-scm.com/downloads" `
    "Install Git for Windows") -and $allOk

if (-not $allOk) {
    Write-Host "`nSome prerequisites are still missing. Resolve them and re-run setup." -ForegroundColor Yellow
    exit 1
}

# Refresh PATH so newly installed tools are visible in this session
$env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User')
$hasMsBuild = (Get-Command msbuild -ErrorAction SilentlyContinue) -ne $null
$hasCl      = (Get-Command cl -ErrorAction SilentlyContinue) -ne $null

# -- Dear ImGui -----------------------------------------------------------------
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

# -- Build directory ------------------------------------------------------------
$buildDir = "$root\build"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

Write-Host "`n=== Configuration Complete ===" -ForegroundColor Cyan

# -- CMake configure (opt-out via -NoBuild) -------------------------------------
if (!$NoBuild) {
    Write-Host "`n[..]     Running CMake configure..." -ForegroundColor Yellow
    $gen = if ($hasMsBuild -or $hasCl) { "Visual Studio 17 2022" } else { "Ninja" }
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

# -- Done -----------------------------------------------------------------------
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
