<h1 align="center">
<img width="1000" height="280" alt="ChatGPT Image Jun 30, 2026, 09_08_47 PM" src="https://github.com/user-attachments/assets/59e55e3f-7f1b-4793-8d46-442c84002517" />
 </h1>
<h3 align="center">VeneeR &middot; Skin Changer &middot; Counter-Strike 2</h3>
<p align="center">
  External overlay &middot; D3D11 &middot; Dear ImGui &middot; No injection
</p>
<p align="center">
  <code>mrch4n725/cs2-skin-changer-external</code> &nbsp;&middot;&nbsp; <strong>v1.8.2</strong>
</p>

<div align="center">

[![Release](https://img.shields.io/badge/release-v1.8.2-F59E0B?style=flat-square)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)
[![Windows](https://img.shields.io/badge/Windows-10%2F11-2D7EE7?style=flat-square&logo=windows11&logoColor=white)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)

</div>

An external skin changer for Counter-Strike 2 using process memory access. Creates its own transparent overlay window with Dear ImGui UI — no DLL injection required.

> **Development Status:** Pre-v2. All v1.x releases are developmental builds. Core systems are being assembled and hardened. Once proven reliable across CS2 updates, the project graduates to v2.0.0.

## Features

- **Skins** — Apply any paint kit to any weapon
- **Knives** — All 18 cosmetic knife types with finishes (Doppler phases, Fade, Marble Fade, etc.)
- **Gloves** — All 8 tradeable glove types with paint kits
- **Music Kits** — ~70 music kits
- **Float & Pattern** — Adjust wear (0.00–1.00) and pattern seed per weapon
- **Config Persistence** — Auto-saves to `loadout.txt` alongside the EXE
- **Offsets Auto-Update** — Fetches latest offsets from cs2-dumper at every launch
- **Skin Texture Previews** — Downloads skin images from Steam CDN with 3 fallback strategies
- **Single EXE** — No DLL, no injector, no extra files

## How to Use

1. **Add folder exclusion** in Windows Security (see below) — required before downloading
2. **Download `CS2SkinChanger.exe`** from [Releases](https://github.com/mrch4n725/cs2-skin-changer-external/releases/tag/v1.8.2)
3. **Launch Counter-Strike 2** and join a game
4. **Run `CS2SkinChanger.exe` as Administrator**
5. **Press `Insert`** to toggle the overlay

### Troubleshooting

- **"Failed to open CS2 process"** — Run the EXE as **Administrator**. Ensure CS2 is running.
- **"CS2 not found"** — CS2 must be running before (or shortly after) the EXE starts.
- **Windows blocks the download** — see exclusion setup below.

## Windows Blocks the App

The binary is unsigned and uses memory APIs (`OpenProcess`, `ReadProcessMemory`, `WriteProcessMemory`) which Windows Defender flags heuristically. Every memory editor and mod tool has the same issue.

### Add an exclusion before downloading

1. **Windows Security** → **Virus & threat protection** → **Manage settings**
2. **Exclusions** → **Add or remove exclusions** → **Add an exclusion** → **Folder**
3. Select the folder where you'll save the file
4. Download and run — it works without interference

### If Defender already deleted the file

1. **Windows Security** → **Protection history**
2. Find the binary → **Actions** → **Restore**
3. Then add the folder exclusion above before running again

## Build from Source

Cross-compiled on Linux with MinGW. See [BUILD.md](BUILD.md) for details.

## Architecture

```
CS2SkinChanger.exe (standalone external overlay)
├── Creates transparent D3D11 overlay window
├── Opens CS2 via OpenProcess (RPM/WPM)
├── XOR-obfuscated Win32 API resolution
├── Dear ImGui UI rendered in own window
├── Config stored alongside EXE (loadout.txt, offsets.dat)
└── Auto-closes when CS2 exits
```

Memory access pattern:
```
OpenProcess(CS2) → ReadProcessMemory / WriteProcessMemory
  → Resolve local player → Get weapons → Write skins
  → All via XOR-obfuscated Win32 API pointers
```

## VAC Evasion

- **Symbols stripped** (`-s` linker flag) — no mangled C++ names in binary
- **All logging compiled out in Release** — no format strings, no fprintf
- **XOR encryption** — 8-byte multi-key derived from `__TIME__`
- **No injection** — no CreateRemoteThread, no LoadLibraryA, no MinHook
- **External only** — never runs inside CS2's process

## Disclaimer

For educational purposes only. Using this against CS2's ToS may result in a VAC ban. Use at your own risk.
