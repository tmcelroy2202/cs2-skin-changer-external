param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# -- Elevate to Admin if not already running as Admin ---------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]"Administrator")
if (-not $isAdmin) {
    Write-Host "Setup requires Administrator privileges." -ForegroundColor Yellow
    Write-Host "Relaunching with UAC elevation..." -ForegroundColor Yellow
    Start-Process powershell -ArgumentList "-NoExit -NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" $(if ($NoBuild) { '-NoBuild' })" -Verb RunAs
    exit 0
}

# -- Progress helpers -----------------------------------------------------------
$stepNum = 0
$totalSteps = 5
function Step($title) {
    $script:stepNum++
    Write-Host "`n[$($script:stepNum)/$totalSteps] $title" -ForegroundColor White
}
function Ok($msg) {
    Write-Host "  [OK]  $msg" -ForegroundColor Green
}
function Fail($msg) {
    Write-Host "  [FAIL]  $msg" -ForegroundColor Red
}
function Skip($msg) {
    Write-Host "  [SKIP] $msg" -ForegroundColor Gray
}
function Info($msg) {
    Write-Host "  [INFO] $msg" -ForegroundColor Cyan
}

# -- Banner --------------------------------------------------------------------
Clear-Host
Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "  CS2 Skin Changer - Setup" -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "  Root: $root" -ForegroundColor Gray
Write-Host "===========================================" -ForegroundColor Cyan

# -- Step 1: Check prerequisites -----------------------------------------------
Step "Checking prerequisites"

$hasWinget = (Get-Command winget -ErrorAction SilentlyContinue) -ne $null
$hasCMake = (Get-Command cmake -ErrorAction SilentlyContinue) -ne $null
$hasGit   = (Get-Command git -ErrorAction SilentlyContinue) -ne $null
$hasMsBuild = (Get-Command msbuild -ErrorAction SilentlyContinue) -ne $null
$hasCl      = (Get-Command cl -ErrorAction SilentlyContinue) -ne $null

if ($hasCMake)   { Ok "CMake 3.20+" } else { Fail "CMake 3.20+" }
if ($hasGit)     { Ok "Git" } else { Fail "Git" }
if ($hasMsBuild -or $hasCl) { Ok "Visual Studio 2022 (C++ tools)" } else { Fail "Visual Studio 2022 (C++ tools)" }
if ($hasWinget)  { Ok "winget package manager" } else { Info "winget not found - manual installs will be needed" }
Write-Host ""

# -- Step 2: Install missing dependencies via winget ---------------------------
Step "Installing missing dependencies"

$needed = @()
if (-not $hasCMake) { $needed += @{Name='CMake'; WingetId='Kitware.CMake'; Url='https://cmake.org/download/'} }
if (-not $hasGit)   { $needed += @{Name='Git'; WingetId='Git.Git'; Url='https://git-scm.com/downloads'} }

if ($needed.Count -gt 0) {
    if (-not $hasWinget) {
        Fail "winget not available - install these manually:"
        foreach ($dep in $needed) { Write-Host "         $($dep.Url)" -ForegroundColor Blue }
        Write-Host "`nThen re-run this setup." -ForegroundColor Yellow
        exit 1
    }
    foreach ($dep in $needed) {
        $resp = Read-Host "  $($dep.Name) is missing. Install it now? [Y/n]"
        if ($resp -eq "" -or $resp -eq "y" -or $resp -eq "Y") {
            Write-Progress -Activity "Installing $($dep.Name)" -Status "winget install..." -PercentComplete 50
            & winget install --id $dep.WingetId --accept-package-agreements 2>&1
            Write-Progress -Activity "Installing $($dep.Name)" -Completed
            if ($LASTEXITCODE -eq 0) {
                Ok "$($dep.Name) installed"
            } else {
                Fail "$($dep.Name) install failed (exit $LASTEXITCODE)"
                Write-Host "         Install manually from: $($dep.Url)" -ForegroundColor Blue
                exit 1
            }
        } else {
            Skip "$($dep.Name) - install manually from $($dep.Url)"
        }
    }
} else {
    Skip "All dependencies already satisfied"
}

# Handle Visual Studio separately (complex install)
if (-not ($hasMsBuild -or $hasCl)) {
    $resp = Read-Host "  Visual Studio 2022 (C++ tools) is missing. Install now? [Y/n]"
    if ($resp -eq "" -or $resp -eq "y" -or $resp -eq "Y") {
        if (-not $hasWinget) {
            Fail "winget not available. Install VS2022 manually: https://visualstudio.microsoft.com/"
            exit 1
        }
        Write-Progress -Activity "Installing Visual Studio 2022 Community" -Status "winget install (large download, 10+ min)..." -PercentComplete 30
        & winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements 2>&1
        Write-Progress -Activity "Installing Visual Studio 2022 Community" -PercentComplete 80
        if ($LASTEXITCODE -eq 0) {
            Ok "VS2022 Community installed"
            Info 'Adding "Desktop development with C++" workload...'
            $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
            if (Test-Path $vsInstaller) {
                & $vsInstaller modify --installPath "$env:ProgramFiles\Microsoft Visual Studio\2022\Community" --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive --norestart --wait
                if ($LASTEXITCODE -eq 0 -or $LASTEXITCODE -eq 3010) {
                    Ok "C++ workload added"
                } else {
                    Info "C++ workload may need manual setup (exit $LASTEXITCODE)"
                }
            } else {
                Info "VS installer not found at expected path - add C++ workload manually"
                Write-Host "         Run: vs_installer.exe modify --add Microsoft.VisualStudio.Workload.NativeDesktop" -ForegroundColor Blue
            }
        } else {
            Fail "VS2022 install failed (exit $LASTEXITCODE)"
            Write-Host "         Install manually from: https://visualstudio.microsoft.com/" -ForegroundColor Blue
            exit 1
        }
    } else {
        Skip "Visual Studio - install manually and re-run setup"
        exit 1
    }
    Write-Progress -Activity "Installing Visual Studio 2022 Community" -Completed
} else {
    Skip "Visual Studio C++ tools already present"
}
Write-Host ""

# Refresh PATH for newly installed tools
$env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User')
$hasMsBuild = (Get-Command msbuild -ErrorAction SilentlyContinue) -ne $null
$hasCl      = (Get-Command cl -ErrorAction SilentlyContinue) -ne $null

# -- Step 3: Fetch Dear ImGui --------------------------------------------------
Step "Fetching Dear ImGui"

$imguiDir = "$root\third_party\imgui"
if (-not (Test-Path "$imguiDir\imgui.h")) {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        Fail "Git is required to clone Dear ImGui"
        exit 1
    }
    Write-Progress -Activity "Fetching Dear ImGui" -Status "git clone..." -PercentComplete 50
    git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git "$imguiDir" 2>$null
    Write-Progress -Activity "Fetching Dear ImGui" -Completed
    if ($LASTEXITCODE -eq 0) {
        Ok "Dear ImGui v1.91.8"
    } else {
        Fail "Failed to clone ImGui"
        exit 1
    }
} else {
    Ok "Dear ImGui (found in third_party/imgui)"
}
Write-Host ""

# -- Step 4: CMake configure ---------------------------------------------------
Step "Configuring CMake"

$buildDir = "$root\build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

$gen = if ($hasMsBuild -or $hasCl) { "Visual Studio 17 2022" } else { "Ninja" }
$arch = "x64"

if (-not $NoBuild) {
    Write-Progress -Activity "CMake configure" -Status "Running cmake..." -PercentComplete 40
    cmake -S $root -B $buildDir -G "$gen" -A $arch -DS2RF_BUILD_STANDALONE=ON
    if ($LASTEXITCODE -eq 0) {
        Write-Progress -Activity "CMake configure" -Completed
        Ok "CMake configuration complete"
    } else {
        Write-Progress -Activity "CMake configure" -Completed
        Fail "CMake configuration failed"
        exit 1
    }
} else {
    Skip "Skipped (-NoBuild)"
}
Write-Host ""

# -- Step 5: Build -------------------------------------------------------------
Step "Building"

if (-not $NoBuild) {
    Write-Progress -Activity "Building" -Status "cmake --build Release..." -PercentComplete 30
    cmake --build $buildDir --config Release
    if ($LASTEXITCODE -eq 0) {
        Write-Progress -Activity "Building" -Completed
        Ok "Build succeeded (Release)"
    } else {
        Write-Progress -Activity "Building" -Completed
        Fail "Build failed"
        exit 1
    }
} else {
    Skip "Skipped (-NoBuild)"
}

# -- Done ----------------------------------------------------------------------
Write-Host ""
Write-Host "===========================================" -ForegroundColor Green
Write-Host "  Setup Complete" -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Binary:       $buildDir\Release\S2ResearchFramework.exe" -ForegroundColor White
Write-Host "  Mode:         Standalone overlay (no injection)" -ForegroundColor White
Write-Host "  CS2 process:  cs2.exe" -ForegroundColor White
Write-Host ""
Write-Host "  Usage:" -ForegroundColor Cyan
Write-Host "    1. Launch Counter-Strike 2" -ForegroundColor Gray
Write-Host "    2. Run S2ResearchFramework.exe as Administrator" -ForegroundColor Gray
Write-Host "    3. Select skins in the overlay window" -ForegroundColor Gray
Write-Host "    4. Changes apply automatically" -ForegroundColor Gray
Write-Host ""
Write-Host "  To rebuild: cmake --build $buildDir --config Release" -ForegroundColor Gray
Write-Host ""
Write-Host "===========================================" -ForegroundColor Green
