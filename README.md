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

Runs as a standalone transparent overlay (Discord window hijack + D3D11 + ImGui). Reads/writes CS2's process memory externally using `ReadProcessMemory`/`WriteProcessMemory`:

```
GetLocalController → GetLocalPlayer → GetWeapons
  → for each weapon: write paint kit, set item ID high,
    create econ attributes, set mesh mask
  → call RegenerateWeaponSkins
  → apply glove skin via m_EconGloves
```

## Requirements

| Dependency | Version | Purpose |
|---|---|---|
| Windows 10/11 | x64 | Target OS |
| Visual Studio 2022 | with "Desktop development with C++" workload | MSVC compiler & Windows SDK |
| [CMake](https://cmake.org/download/) | 3.20+ | Build system |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91+ (auto-fetched) | UI framework |

> **Note:** This project is Windows-only. It builds as either a standalone overlay EXE or an injected DLL — both use Win32/D3D11 APIs not available on Linux.

## Quick Start

Double-click `setup.bat` or run from a terminal:

```
setup.bat
```

The script checks prerequisites, clones Dear ImGui into `third_party/imgui/`, configures CMake, and builds the Release binary.

## Manual Build

```batch
:: Clone ImGui if not present
git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party\imgui

:: Configure (standalone overlay EXE)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DS2RF_BUILD_STANDALONE=ON

:: Build
cmake --build build --config Release
```

The binary is written to `build\Release\S2ResearchFramework.exe`.

## Usage

1. **Launch Counter-Strike 2** and join a game (official or community).
2. **Run the overlay as Administrator** — required for `OpenProcess` access to `cs2.exe`.
   ```
   build\Release\S2ResearchFramework.exe
   ```
3. **Use the overlay window** — three sidebar tabs:

   | Tab | What you can do |
   |---|---|
   | **Skins** | Select a weapon, pick a paint kit, adjust float (0.00–1.00) and pattern seed |
   | **Gloves** | Choose glove type and paint kit |
   | **Music Kit** | Select from ~70 music kits (applied per-weapon in CS2) |

4. **Changes apply automatically** — every user action saves and syncs. No save button needed.

### Knife Skins

18 cosmetic knife types are listed alongside the weapon combos. To equip a knife:
1. Select a knife type from the weapon dropdown (e.g. "Talon Knife").
2. Pick a finish (e.g. "Doppler Phase 1").
3. The overlay handles def-index swapping and paint kit write automatically.

### Config Persistence

Your loadout is saved to `%APPDATA%\cs2-skin-changer\loadout.txt` on every change and restored on next launch. You can share or back up this file.

## Build Options

| CMake Flag | Default | Description |
|---|---|---|
| `-DS2RF_BUILD_STANDALONE=ON` | OFF | Build as standalone overlay EXE (OFF = injected DLL) |

### DLL Mode

To build as an injectable DLL instead:

```batch
cmake -B build -G "Visual Studio 17 2022" -A x64 -DS2RF_BUILD_STANDALONE=OFF
cmake --build build --config Release
```

You will need your own injector to load `S2ResearchFramework.dll` into `cs2.exe`.

## Disclaimer

This project is for educational purposes only. Using it against CS2's Terms of Service may result in a VAC ban. Use at your own risk.
