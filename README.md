# CS2 Skin Changer — External Overlay

An external skin changer for Counter-Strike 2 using process memory access. Standalone overlay with Dear ImGui UI — no injection required.

## Features

- **Skins** — Apply any paint kit to any weapon (top 5 desirable skins pre-loaded per weapon)
- **Knives** — Change knife model + finish (all 18 cosmetic knife types, Doppler phases, Fade, Marble Fade, etc.)
- **Gloves** — Apply glove paint kits across all 8 tradeable glove types
- **Music Kits** — Select from ~70 music kits

## How It Works

Runs as a standalone transparent overlay (Discord window hijack + D3D11 + ImGui). Reads/writes CS2's process memory externally using `ReadProcessMemory`/`WriteProcessMemory`:

```
GetLocalController → GetLocalPlayer → GetWeapons
  → for each weapon: write paint kit, set item ID high,
    create econ attributes, set mesh mask
  → call RegenerateWeaponSkins
  → apply glove skin via m_EconGloves
```

## Build

Requires CMake, C++20 compiler, and ImGui. Built for Windows x64.

```
cmake -B build -DS2RF_BUILD_STANDALONE=ON
cmake --build build
```

## Disclaimer

This project is for educational purposes only. Using it against CS2's Terms of Service may result in a VAC ban. Use at your own risk.
