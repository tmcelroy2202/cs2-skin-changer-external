<h1 align="center">
<img width="1000" height="280" alt="ChatGPT Image Jun 30, 2026, 09_08_47 PM" src="https://github.com/user-attachments/assets/59e55e3f-7f1b-4793-8d46-442c84002517" />
 </h1>
<h3 align="center">VeneeR &middot; External Skin Changer &middot; Counter-Strike 2</h3>
<p align="center">
  Standalone overlay &middot; D3D11 &middot; Dear ImGui &middot; No injection
</p>
<p align="center">
  <code>mrch4n725/cs2-skin-changer-external</code> &nbsp;&middot;&nbsp; <strong>v1.5.8</strong>
</p>

<div align="center">

[![Build](https://img.shields.io/badge/build-passing-2D7EE7?style=flat-square)](https://github.com/mrch4n725/cs2-skin-changer-external/actions)
[![Release](https://img.shields.io/badge/release-v1.5.8-F59E0B?style=flat-square)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)
[![License](https://img.shields.io/badge/license-MIT-888999?style=flat-square)](LICENSE)
[![Windows](https://img.shields.io/badge/Windows-10%2F11-2D7EE7?style=flat-square&logo=windows11&logoColor=white)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)

</div>

An external skin changer for Counter-Strike 2 using process memory access. Standalone transparent overlay with Dear ImGui UI — no injection required.

> ⚠️ **Development Status: Pre-v2 Testing Phase**
>
> All releases under the **v1.\*.\*** umbrella are **developmental builds** — the project is actively being constructed and has not yet reached a stable, fully functional state. These versions serve as the foundational building phase: core systems are being assembled, the codebase is being audited and hardened, and the UI is being refined.
>
> Once the skin changer is proven to work reliably across CS2 updates with a polished user experience, the project will graduate to **v2.0.0** and be considered a production-ready release.
>
> **Until then, expect bugs, incomplete features, and breaking changes between releases.** Contributions, issue reports, and testing feedback are all welcome and help move the project toward v2.

## Features

- **Skins** — Apply any paint kit to any weapon (top 5 desirable skins pre-loaded per weapon)
- **Knives** — Change knife model + finish (all 18 cosmetic knife types, Doppler phases, Fade, Marble Fade, etc.)
- **Gloves** — Apply glove paint kits across all 8 tradeable glove types
- **Music Kits** — Select from ~70 music kits
- **Float & Pattern** — Adjust wear value (0.00–1.00) and pattern seed per weapon
- **Cloud-Synced Configs** — Loadout auto-saves to `%APPDATA%/veneer/loadout.txt` on every change; persists across restarts
- **Full-Update Bypass** — Fire-and-forget remote thread; no blocking, no lag stutter
- **Anti-Detection** — Timing jitter (random 1–15ms), randomized write-order (6 permutations per weapon), idle rate-limiting (~20 Hz), cached sig addresses (no repeated pattern scans), unique random window class/title per launch, minimum-access process handle

## Screenshots

![VeneeR UI — Rifles category with AK-47 Fire Serpent selected]<img width="820" height="560" alt="veneer-ui" src="https://github.com/user-attachments/assets/4928d9f9-caf0-4496-92ff-486a71e72563" />

*VeneeR main UI — sidebar categories, weapon list, skin cards, and wear/StatTrak controls*

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

Grab the latest **VeneeR** release from [Releases](https://github.com/mrch4n725/cs2-skin-changer-external/releases).

1. Download the binary (~2 MB)
2. Launch Counter-Strike 2 and join a game
3. Run it **as Administrator**
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
3. **Press `Insert`** to toggle the overlay UI on/off (overlay is visible by default).
4. **Use the overlay window** — a vertical sidebar organises 8 categories on the left with the active category highlighted in teal. The main panel shows a searchable weapon list and a grid of skin cards. Adjust wear, pattern seed, and StatTrak, then click **Apply Skin**.

5. **Click "Apply Skin"** to save weapon/knife changes, or select a glove/music kit for immediate apply.

### Knife Skins

Knives live under the **Knives** category tab alongside weapon combos. Select a knife type chip (e.g. "Talon Knife"), then pick a finish card (e.g. "Doppler Phase 1") and click **Apply Skin**.

### Config Persistence

Your loadout is saved to `%APPDATA%\veneer\loadout.txt` on every change and restored on next launch. You can share or back up this file.

### Offsets

Game offsets are stored in `%APPDATA%\veneer\offsets.txt`. Defaults are written automatically on first launch. If a game update breaks skin application, update the offset values here — no recompile needed.

---

## Build Options

| CMake Flag | Default | Description |
|---|---|---|
| `-DCSC_BUILD_STANDALONE=ON` | ON | Build as standalone overlay EXE (OFF = injected DLL) |

---

## Windows Blocks the App

This is normal — the binary is unsigned and uses memory-editing APIs (`OpenProcess`, `WriteProcessMemory`) which Windows Defender flags heuristically. Every memory editor, mod tool, and game trainer has the same issue.

There are two possible symptoms depending on your Windows version:

- **"Windows protected your PC"** (blue dialog) — SmartScreen reputation. Click *More info → Run anyway* and it works.
- **"SmartScreen can't be reached right now"** / file vanishes — **Defender already removed the file** in the background. See below.

### Quick fix: add an exclusion before downloading

1. Open **Windows Security** → **Virus & threat protection** → **Manage settings**
2. Under **Exclusions**, click **Add or remove exclusions** → **Add an exclusion** → **Folder**
3. Select the folder where you'll save the binary (e.g. Downloads)
4. Download and run — it will work without interference

### If Defender already deleted the file

1. Open **Windows Security** → **Virus & threat protection** → **Protection history**
2. Find the binary → click **Actions** → **Restore**
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
2. Delete saved config and offsets (`%APPDATA%\veneer\`)
3. Optionally uninstall CMake, Git, and VS2022 Build Tools (prompts for each)

To fully remove the project, just delete the repository folder.

---

## Disclaimer

This project is for educational purposes only. Using it against CS2's Terms of Service may result in a VAC ban. Use at your own risk.
