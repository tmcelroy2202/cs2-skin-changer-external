$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# ── Helpers ────────────────────────────────────────────────────────────────────
function Write-Prompt($msg) {
    Write-Host "  >>> $msg" -ForegroundColor Yellow
}
function Write-Ok($msg) {
    Write-Host "  [OK]  $msg" -ForegroundColor Green
}
function Write-Fail($msg) {
    Write-Host "  [FAIL] $msg" -ForegroundColor Red
}

# ── Banner ─────────────────────────────────────────────────────────────────────
Clear-Host
Write-Host "===========================================" -ForegroundColor Red
Write-Host "  CS2 Skin Changer - Uninstall" -ForegroundColor Red
Write-Host "===========================================" -ForegroundColor Red
Write-Host "  Root: $root" -ForegroundColor Gray
Write-Host "===========================================" -ForegroundColor Red

# ── 1. Project files ──────────────────────────────────────────────────────────
Write-Host "`n[1/3] Removing project build artifacts..." -ForegroundColor White

$buildDir = "$root\build"
if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue
    Write-Ok "Deleted build directory"
} else {
    Write-Ok "Build directory not found"
}

$imguiDir = "$root\third_party\imgui"
if (Test-Path $imguiDir) {
    Remove-Item -Recurse -Force $imguiDir -ErrorAction SilentlyContinue
    Write-Ok "Deleted third_party/imgui"
} else {
    Write-Ok "third_party/imgui not found"
}

# ── 2. AppData config ─────────────────────────────────────────────────────────
Write-Host "`n[2/3] Removing saved config and offsets..." -ForegroundColor White

$configDir = "$env:APPDATA\cs2-skin-changer"
if (Test-Path $configDir) {
    Remove-Item -Recurse -Force $configDir -ErrorAction SilentlyContinue
    Write-Ok "Deleted %APPDATA%\cs2-skin-changer"
} else {
    Write-Ok "Config directory not found"
}

# ── 3. Toolchain (optional) ───────────────────────────────────────────────────
Write-Host "`n[3/3] Toolchain removal (optional)..." -ForegroundColor White
Write-Host "  These tools may be used by other projects." -ForegroundColor Gray
Write-Host "  Only remove them if you're sure they aren't needed elsewhere." -ForegroundColor Gray

$hasWinget = (Get-Command winget -ErrorAction SilentlyContinue) -ne $null
if (-not $hasWinget) {
    Write-Fail "winget not available - uninstall tools manually"
    Write-Host "         CMake:  https://cmake.org/download/" -ForegroundColor Blue
    Write-Host "         Git:    https://git-scm.com/downloads" -ForegroundColor Blue
    Write-Host "         VS2022: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022" -ForegroundColor Blue
} else {
    # ── CMake ──
    $hasCMake = (Get-Command cmake -ErrorAction SilentlyContinue) -ne $null
    if ($hasCMake) {
        $resp = Read-Host "`n  Remove CMake? [y/N]"
        if ($resp -eq "y" -or $resp -eq "Y") {
            & winget uninstall Kitware.CMake --silent 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Ok "CMake uninstalled"
            } else {
                Write-Fail "CMake uninstall failed (exit $LASTEXITCODE)"
            }
        } else {
            Write-Ok "CMake kept"
        }
    } else {
        Write-Ok "CMake not found"
    }

    # ── Git ──
    $hasGit = (Get-Command git -ErrorAction SilentlyContinue) -ne $null
    if ($hasGit) {
        $resp = Read-Host "`n  Remove Git? [y/N]"
        if ($resp -eq "y" -or $resp -eq "Y") {
            & winget uninstall Git.Git --silent 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Ok "Git uninstalled"
            } else {
                Write-Fail "Git uninstall failed (exit $LASTEXITCODE)"
            }
        } else {
            Write-Ok "Git kept"
        }
    } else {
        Write-Ok "Git not found"
    }

    # ── VS Build Tools ──
    $vsUninstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
    if (Test-Path $vsUninstaller) {
        $resp = Read-Host "`n  Remove VS2022 Build Tools (C++ compiler)? [y/N]"
        if ($resp -eq "y" -or $resp -eq "Y") {
            Write-Host "  >>> This will uninstall only the Build Tools (not full VS if installed)." -ForegroundColor Yellow
            & winget uninstall Microsoft.VisualStudio.2022.BuildTools --silent 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Ok "VS2022 Build Tools uninstalled"
            } else {
                Write-Fail "VS2022 Build Tools uninstall failed (exit $LASTEXITCODE)"
                Write-Host "         Try: 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe uninstall --installPath \"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\" --passive --wait'" -ForegroundColor Blue
            }
        } else {
            Write-Ok "VS2022 Build Tools kept"
        }
    } else {
        Write-Ok "VS2022 Build Tools not found"
    }
}

# ── Done ───────────────────────────────────────────────────────────────────────
Write-Host "`n===========================================" -ForegroundColor Green
Write-Host "  Uninstall complete" -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Project files removed." -ForegroundColor White
Write-Host "  If you want to remove the source code too," -ForegroundColor Gray
Write-Host "  simply delete the repository folder." -ForegroundColor Gray
Write-Host ""
