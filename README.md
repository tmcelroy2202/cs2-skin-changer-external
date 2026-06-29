# CS2 Skin Changer — External Overlay

An external skin changer for Counter-Strike 2 using process memory access. Standalone overlay with Dear ImGui UI — no injection required.

## Features

- **Skins** — Apply any paint kit to any weapon (top 5 desirable skins pre-loaded per weapon)
- **Knives** — Change knife model + finish (all 18 cosmetic knife types, Doppler phases, Fade, Marble Fade, etc.)
- **Gloves** — Apply glove paint kits across all 8 tradeable glove types
- **Music Kits** — Select from ~70 music kits
- **Float & Pattern** — Adjust wear value (0.00–1.00) and pattern seed per weapon
- **Cloud-Synced Configs** — Loadout auto-saves to `%APPDATA%/cs2-skin-changer/loadout.txt` on every change; persists across restarts
- **Full-Update Bypass** — Fire-and-forget remote thread; no blocking, no lag stutter
- **Anti-Detection** — Timing jitter (random 1–15ms), randomized write-order (6 permutations per weapon), idle rate-limiting (~20 Hz), cached sig addresses (no repeated pattern scans), unique random window class/title per launch, minimum-access process handle

## How It Works

Runs as a standalone transparent overlay (self-created D3D11 window + ImGui). Reads/writes CS2's process memory externally using `ReadProcessMemory`/`WriteProcessMemory`:

```
GetLocalController → GetLocalPlayer → GetWeapons
  → for each weapon: write paint kit, set item ID high,
    create econ attributes, set mesh mask
  → call RegenerateWeaponSkins
  → apply glove skin via m_EconGloves
```

---

## Choose Your Path

### Option 1: Download Pre-Built Binary (easiest)

Grab the latest `CS2SkinChanger.exe` from [Releases](https://github.com/mrch4n725/cs2-skin-changer-external/releases).

1. Download `CS2SkinChanger.exe` (~2 MB)
2. Launch Counter-Strike 2 and join a game
3. Run `CS2SkinChanger.exe` **as Administrator**
4. Press **Insert** to toggle the overlay

All offsets and defaults are embedded with automatic updates from [cs2-dumper](https://github.com/a2x/cs2-dumper) at every launch — no manual setup needed. If CS2 updates break skin application, just restart the tool and it will fetch the latest offsets automatically.

> ⚠️ **Windows blocks the download?** See the [troubleshooting guide](#windows-blocks-the-app) below.

### Option 2: Build from Source (for tinkers)

You get full control: edit the code, change the UI, fix game updates yourself by editing offsets, or add new features.

**What you'll need:**

| Dependency | Size | Purpose |
|---|---|---|
| Windows 10/11 | — | Target OS |
| [CMake](https://cmake.org/download/) 3.20+ | ~30 MB | Build system |
| [VS2022 Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) | ~300 MB | C++ compiler, linker, MSVC, Windows SDK |
| [Git](https://git-scm.com/downloads) | ~50 MB | Clone dependencies |
| [Dear ImGui](https://github.com/ocornut/imgui) 1.91+ | ~10 MB | UI framework (auto-cloned) |

> **Note:** Windows-only. Uses Win32/D3D11 APIs not available on Linux.

**Quick Setup (recommended):**

Double-click `setup.bat`. It checks for missing tools, installs them if you want, clones ImGui, configures CMake, and builds the Release binary — all in one go.

```
setup.bat
```

**Manual Build:**

```batch
:: Clone ImGui if not present
git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party\imgui

:: Configure (standalone overlay EXE)
cmake --preset windows-release

:: Build
cmake --build --preset windows-release
```

The binary is written to `build\Release\CS2SkinChanger.exe`.

For release checklist details and the DLL preset, see [BUILD.md](BUILD.md).

---

## Usage

1. **Launch Counter-Strike 2** and join a game (official or community).
2. **Run the overlay as Administrator** — required for `OpenProcess` access to `cs2.exe`.
   ```
   CS2SkinChanger.exe
   ```
3. **Press `Insert`** to toggle the overlay UI on/off (overlay is visible by default).
4. **Use the overlay window** — three sidebar tabs:

   | Tab | What you can do |
   |---|---|
   | **Skins** | Select a weapon, pick a paint kit, adjust float (0.00–1.00) and pattern seed |
   | **Gloves** | Choose glove type and paint kit |
   | **Music Kit** | Select from ~70 music kits (applied per-weapon in CS2) |

5. **Changes apply automatically** — every user action saves and syncs. No save button needed.

### Knife Skins

18 cosmetic knife types are listed alongside the weapon combos. To equip a knife:
1. Select a knife type from the weapon dropdown (e.g. "Talon Knife").
2. Pick a finish (e.g. "Doppler Phase 1").
3. The overlay handles def-index swapping and paint kit write automatically.

### Config Persistence

Your loadout is saved to `%APPDATA%\cs2-skin-changer\loadout.txt` on every change and restored on next launch. You can share or back up this file.

### Offsets

Game offsets are stored in `%APPDATA%\cs2-skin-changer\offsets.txt`. Defaults are written automatically on first launch. If a game update breaks skin application, update the offset values here — no recompile needed.

---

## Build Options

| CMake Flag | Default | Description |
|---|---|---|
| `-DCSC_BUILD_STANDALONE=ON` | OFF | Build as standalone overlay EXE (OFF = injected DLL) |

---

## Windows Blocks the App

This is normal — the binary is unsigned and uses memory-editing APIs (`OpenProcess`, `WriteProcessMemory`) which Windows Defender flags heuristically. Every memory editor, mod tool, and game trainer has the same issue.

There are two possible symptoms depending on your Windows version:

- **"Windows protected your PC"** (blue dialog) — SmartScreen reputation. Click *More info → Run anyway* and it works.
- **"SmartScreen can't be reached right now"** / file vanishes — **Defender already removed the file** in the background. See below.

### Quick fix: add an exclusion before downloading

1. Open **Windows Security** → **Virus & threat protection** → **Manage settings**
2. Under **Exclusions**, click **Add or remove exclusions** → **Add an exclusion** → **Folder**
3. Select the folder where you'll save `CS2SkinChanger.exe` (e.g. Downloads)
4. Download and run — it will work without interference

### If Defender already deleted the file

1. Open **Windows Security** → **Virus & threat protection** → **Protection history**
2. Find `CS2SkinChanger.exe` → click **Actions** → **Restore**
3. Then add the folder exclusion above before running again

### Option 2: Build from Source (for tinkers)

To build as an injectable DLL instead:

```batch
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCSC_BUILD_STANDALONE=OFF
cmake --build build --config Release
```

You will need your own injector to load `CS2SkinChanger.dll` into `cs2.exe`.

---

## Uninstall

Run `uninstall.bat` to:

1. Delete the `build/` directory and `third_party/imgui`
2. Delete saved config and offsets (`%APPDATA%\cs2-skin-changer\`)
3. Optionally uninstall CMake, Git, and VS2022 Build Tools (prompts for each)

To fully remove the project, just delete the repository folder.

---

## Disclaimer

This project is for educational purposes only. Using it against CS2's Terms of Service may result in a VAC ban. Use at your own risk.
