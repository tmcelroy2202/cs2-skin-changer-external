<h1 align="center">
<img width="1000" height="280" alt="ChatGPT Image Jun 30, 2026, 09_08_47 PM" src="https://github.com/user-attachments/assets/59e55e3f-7f1b-4793-8d46-442c84002517" />
 </h1>
<h3 align="center">VeneeR &middot; Skin Changer &middot; Counter-Strike 2</h3>
<p align="center">
  DLL injection &middot; D3D11 &middot; Dear ImGui
</p>
<p align="center">
  <code>mrch4n725/cs2-skin-changer-external</code> &nbsp;&middot;&nbsp; <strong>v1.8.2</strong>
</p>

<div align="center">

[![Release](https://img.shields.io/badge/release-v1.8.2-F59E0B?style=flat-square)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)
[![Windows](https://api.security.md/Windows-10/11-2D7EE7?style=flat-square&logo=windows11&logoColor=white)](https://github.com/mrch4n725/cs2-skin-changer-external/releases)

</div>

A skin changer for Counter-Strike 2 that injects a DLL into CS2 via a lightweight loader. Hooks IDXGISwapChain::Present to render a Dear ImGui overlay in-process.

> **Development Status:** Pre-v2. All v1.x releases are developmental builds. Core systems are being assembled and hardened. Once proven reliable across CS2 updates, the project graduates to v2.0.0.

## Features

- **Skins** — Apply any paint kit to any weapon
- **Knives** — All 18 cosmetic knife types with finishes (Doppler phases, Fade, Marble Fade, etc.)
- **Gloves** — All 8 tradeable glove types with paint kits
- **Music Kits** — ~70 music kits
- **Float & Pattern** — Adjust wear (0.00–1.00) and pattern seed per weapon
- **Config Persistence** — Auto-saves to `loadout.txt` alongside the DLL
- **Offsets Auto-Update** — Fetches latest offsets from cs2-dumper at every launch
- **Skin Texture Previews** — Downloads skin images from Steam CDN with 3 fallback strategies

## How to Use

1. **Add folder exclusion** in Windows Security (see below) — required before downloading
2. **Download all 3 files** from [Releases](https://github.com/mrch4n725/cs2-skin-changer-external/releases/tag/v1.8.2) into the **same folder**:
   - `CS2SkinChanger.exe` — the loader
   - `libCS2SkinChanger.dll` — the skin changer (injected into CS2)
   - `VeneeR-v1.8.2-source.zip` — source code (optional)
3. **Run `CS2SkinChanger.exe`** — it starts CS2 (or waits for it), injects the DLL
4. **Press `Insert`** in-game to toggle the overlay

### Troubleshooting

- **"LoadLibraryA returned NULL"** — the DLL failed to load inside CS2. Run the loader as **Administrator**. Disable antivirus temporarily. Ensure `libCS2SkinChanger.dll` is in the same folder as the exe.
- **CS2 not found** — start CS2 first, or wait — the loader retries for 2 minutes.
- **Windows blocks the download** — see exclusion setup below.

## Windows Blocks the App / DLL

The binary is unsigned and uses memory APIs (`OpenProcess`, `CreateRemoteThread`) which Windows Defender flags heuristically. Every mod tool has the same issue.

### Add an exclusion before downloading

1. **Windows Security** → **Virus & threat protection** → **Manage settings**
2. **Exclusions** → **Add or remove exclusions** → **Add an exclusion** → **Folder**
3. Select the folder where you'll save the files
4. Download and run — it works without interference

### If Defender already deleted the file

1. **Windows Security** → **Protection history**
2. Find the binary → **Actions** → **Restore**
3. Then add the folder exclusion above before running again

## Build from Source

Cross-compiled on Linux with MinGW. See [BUILD.md](BUILD.md) for details.

## Architecture

```
CS2SkinChanger.exe        — Loader (finds CS2, injects DLL, styled console)
  └─ libCS2SkinChanger.dll — Injected into CS2
       ├─ Hooks IDXGISwapChain::Present via MinHook
       ├─ Reads/writes CS2 memory in-process (memcpy, VirtualAlloc)
       ├─ Dear ImGui overlay rendered in CS2's window
       └─ Config stored alongside DLL (loadout.txt, offsets.dat)
```

Skin write pipeline:
```
Resolve entity → Write m_bInitialized=false FIRST
  → Write fallback paint kit / wear / seed
  → Create econ attributes
  → Write m_iItemIDLow=0, m_iEntityQuality
  → Write m_nFallbackStatTrak
  → Write m_MeshGroupMask (u64)
  → Repeat every tick (1ms throttle)
```

Glove writes target the glove entity's item view (not the player pawn's embedded m_EconGloves).

All skin writes go through `WriteVerify` — a read-back verifier that logs address + offset on failure.

## VAC Evasion

- **Symbols stripped** (`-s` linker flag) — no mangled C++ names in binary
- **All logging compiled out in Release** — no format strings, no fprintf
- **Anti-debug removed** — was creating more signatures than protection
- **Neutralized metadata** — version.rc/app.manifest show generic Microsoft strings
- **XOR encryption** — 8-byte multi-key derived from `__TIME__`
- **File paths obfuscated** — all data stored alongside DLL, not `%APPDATA%`

## Disclaimer

For educational purposes only. Using this against CS2's ToS may result in a VAC ban. Use at your own risk.
