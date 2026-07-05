# Build and Release Guide

Cross-compiled on Linux for Windows x64 using MinGW-w64. No Windows host required.

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| Linux | Any modern distro | Build host |
| [CMake](https://cmake.org/download/) | 3.20+ | Build system |
| [x86_64-w64-mingw32-g++](https://www.mingw-w64.org/) | 13+ | Cross-compiler |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91+ | UI framework (included in `third_party/`) |

Install cross-compiler on Debian/Ubuntu:

```bash
sudo apt install g++-mingw-w64-x86-64
```

## Build

```bash
# Configure (Release, MinGW cross-compile)
cmake -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres

# Build
cmake --build build-release -j$(nproc)
```

## Output

| File | Description |
|------|-------------|
| `build-release/CS2SkinChanger.exe` | Standalone external overlay (single file) |

Statically linked (libgcc, libstdc++), stripped of symbols (`-s`), with logging compiled out (`-DNDEBUG`). No DLL, no dependencies beyond standard Windows system DLLs.

## Release Checklist

1. Clean build: `rm -rf build-release && cmake -B build-release ... && cmake --build build-release -j$(nproc)`
2. Verify no errors, no warnings.
3. Copy exe to `release/` folder.
4. Bump version in `CMakeLists.txt` (project version), `src/loader/main.cpp` (UI string).
5. Commit, tag, push.
6. Create GitHub release, upload the EXE.

## Project Structure

```
CMakeLists.txt                   — Top-level build (single EXE target)
src/
├── loader/
│   └── main.cpp                 — Entry point (overlay window, main loop)
├── core/
│   ├── Core.h                   — Type aliases, BasePath()
│   ├── Logging.h / .cpp         — Logger (compiled out in Release via NDEBUG)
│   ├── ProcessMemory.h          — External RPM/WPM with XOR-obfuscated API
│   └── DllAntiDetect.h          — XOR encryption
├── cs2/
│   ├── Offsets.h                — All CS2 offsets, auto-update from cs2-dumper
│   ├── SkinChanger.h / .cpp     — Core skin write logic (WriteVerify, attribute creation)
│   ├── SkinDatabase.h           — Weapon/skin definitions
│   ├── SkinTextureCache.h       — Skin image download + caching (3 fallback URLs)
│   ├── ConfigManager.h          — Loadout save/load
│   ├── EntitySystem.h           — Entity resolution (weapons, gloves)
│   └── EconSystem.h             — Econ attribute creation
├── ui/
│   ├── Framework.h / .cpp       — ImGui init, frame lifecycle
│   ├── Panels.h / .cpp          — Panel manager
│   ├── overlay/OverlayWindow.h/cpp — Transparent overlay window
│   ├── panels/
│   │   ├── SkinChangerPanel.cpp — Main skin UI
│   │   ├── DebugPanel.cpp       — Diagnostics panel
│   │   └── ConsolePanel.cpp     — In-game log viewer
│   └── widgets/
│       └── HexViewWidget.cpp    — Memory hex viewer
└── resources/
    └── version.rc               — Neutralized ("Microsoft Visual C++ Runtime Library")
```

## Notes

- `build-release/`, `release/`, and `third_party/` are gitignored.
- MinGW's `-static-libgcc -static-libstdc++` ensures no runtime DLL dependency.
- No MinHook, no injection — the EXE is a standalone overlay using external memory access.
