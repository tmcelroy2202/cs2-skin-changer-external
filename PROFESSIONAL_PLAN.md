# VeneeR Professional Skin Changer — Implementation Plan

## Goal
Ship a working CS2 skin changer that applies skins correctly, doesn't crash, and is professional-grade.

## Architecture (Current)

```
CS2SkinChanger.exe (standalone external overlay)
├── Creates transparent D3D11 overlay window
├── Opens CS2 via OpenProcess (RPM/WPM external access)
├── XOR-obfuscated Win32 API resolution for all memory operations
├── Dear ImGui UI rendered in its own window
├── Config stored alongside EXE via BasePath()
└── Auto-closes when CS2 exits
```

## Completed Phases

### Phase 1: Architecture — External Overlay (Complete)
- Standalone EXE creates its own transparent overlay window
- D3D11 device created in-process (no hooking needed)
- Own Windows message loop with 60 FPS cap
- Insert key polling via overlay's message loop
- Auto-detects CS2 process and opens for memory access

### Phase 2: Anti-Detection (Complete)
- **Symbols stripped** via `-s` linker flag — removes all mangled C++ names
- **All logging compiled out in Release** — full Logger class under `#ifndef NDEBUG`; stubs with no-ops in Release
- **XOR encryption** — 8-byte multi-key derived from `__TIME__`
- **External-only access** — never runs inside CS2's process (no injection, no MinHook, no DllMain)
- **No CreateRemoteThread** — eliminates VAC's primary detection vector
- **No LoadLibraryA** in remote process — no DLL loading failures
- **Version.rc neutralized** — "Microsoft Visual C++ Runtime Library"

### Phase 3: UI Improvements (Complete)
- Sidebar category navigation (Pistols, Rifles, SMGs, Snipers, Heavy, Knives, Gloves, Music)
- Skin grid with cards showing weapon name, skin name, rarity color
- Skin texture previews from Steam CDN with 3 fallback strategies
- Search functionality (real-time filtering)
- Debug panel with offset viewer, entity state, hex view
- Console panel for in-game log viewer
- CS2-inspired dark theme

### Phase 4: Auto-Update & Reliability (Complete)
- Offsets auto-update from cs2-dumper at every launch
- Retry logic with exponential backoff for items_game.txt download
- Local CS2 installation path checked first before downloading
- Stale offset detection via ValidateLoadedOffsets()
- Graceful fallback to cached offsets on download failure

### Phase 5: Testing (Complete)
- 30+ original bugs fixed
- 15+ diagnostic bugs fixed
- Full ripple-effect audit — zero compilation/runtime bugs

## Skin Rendering Root Causes Fixed

| # | Issue | Fix |
|---|-------|-----|
| 1 | m_bInitialized=false written AFTER fallbacks | Written FIRST before all fallback values |
| 2 | m_bInitialized conditional — missed CS2 reset | Unconditional every tick |
| 3 | Glove writes to wrong entity | Write to glove entity's item view |
| 4 | Glove attributes created on player pawn | Create on glove entity |
| 5 | 50ms tick throttle — values overwritten | Reduced to 1ms |
| 6 | m_iItemIDLow = -1 treated as invalid | Changed to 0 |

## Skin Texture Pipeline Root Causes Fixed

| # | Issue | Fix |
|---|-------|-----|
| 1 | No retry on items_game.txt download | Local CS2 path first + 3 retries |
| 2 | 1 texture/frame bottleneck | Now 4 textures/frame |
| 3 | No CS2 economy URL fallback | Added cloudflare CDN URLs |
| 4 | Only CDN fallback | Added Steam market search |

## VAC Evasion Vectors

| Vector | Risk | Status |
|--------|------|--------|
| Mangled C++ symbols | CRITICAL | Fixed (`-s`) |
| CreateRemoteThread + LoadLibraryA | CRITICAL | Eliminated (no injection) |
| MinHook trampolines | HIGH | Eliminated (no hooking) |
| version.rc / manifest | CRITICAL | Fixed |
| Running inside CS2 process | CRITICAL | Eliminated (external only) |
| Anti-debug API calls | HIGH | Eliminated |
| Logging output | MEDIUM | Fixed (NDEBUG guard) |
| Single-byte XOR | MEDIUM | Fixed (multi-byte key) |
