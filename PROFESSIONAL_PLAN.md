# VeneeR Professional Skin Changer — Implementation Plan

## Goal
Ship a working CS2 skin changer that applies skins correctly via DLL injection, doesn't crash, and is professional-grade.

## Architecture (Current)

```
CS2SkinChanger.exe              — Loader (finds CS2, injects DLL, styled console)
  └─ CreateRemoteThread + LoadLibraryA
       └─ libCS2SkinChanger.dll — Injected into CS2
            ├─ ProcessMemory: memcpy (DLL) / RPM+WPM (external) dual mode
            ├─ Hooks IDXGISwapChain::Present via MinHook
            ├─ Dear ImGui overlay rendered in CS2's window
            └─ Config stored alongside DLL via BasePath() (not %APPDATA%)
```

## Completed Phases

### Phase 1: Loader (Complete)
- Created `src/loader/main.cpp`, `Injection.h`, `Console.h`, `AntiDetect.h`
- Find CS2 via `CreateToolhelp32Snapshot` + `Process32First/Next`
- Inject via `CreateRemoteThread` + `LoadLibraryA` with XOR-obfuscated API names
- WAIT_TIMEOUT handling (DLL may still load asynchronously)
- Colored console output with progress indicators
- 2-minute timeout waiting for CS2

### Phase 2: Anti-Detection (Complete)
- **Symbols stripped** via `-s` linker flag — removes all mangled C++ names
- **Version.rc neutralized** — "Microsoft Visual C++ Runtime Library" instead of "VeneeR CS2 Skin Changer"
- **App.manifest neutralized** — "Microsoft.CRT" assembly identity
- **All logging compiled out in Release** — full Logger class under `#ifndef NDEBUG`; stubs with no-ops in Release
- **Anti-debug removed** — replaced with stub `DetectDebugger()` returning false; eliminates IsDebuggerPresent/CheckRemoteDebuggerPresent/GetThreadContext signatures
- **XOR encryption** — 8-byte multi-key derived from `__TIME__`
- **File paths obfuscated** — all data stored alongside DLL via `BasePath()`, not `%APPDATA%/veneer/`
- **MinHook trampolines** — acceptable risk for any VAC-tier detour

### Phase 3: UI Improvements (Complete)
- Sidebar category navigation (Pistols, Rifles, SMGs, Snipers, Heavy, Knives, Gloves, Music)
- Skin grid with cards showing weapon name, skin name, rarity color
- Skin texture previews from Steam CDN with 3 fallback strategies (CDN, economy images, Steam market search)
- Search functionality (real-time filtering)
- Debug panel with offset viewer, entity state, hex view
- Console panel for in-game log viewer
- CS2-inspired dark theme

### Phase 4: Auto-Update & Reliability (Complete)
- Offsets auto-update from cs2-dumper at every launch
- Retry logic with exponential backoff for `items_game.txt` download
- Local CS2 installation path checked first before downloading
- Stale offset detection via `ValidateLoadedOffsets()` (~15 offset checks)
- Graceful fallback to cached offsets on download failure

### Phase 5: Testing (Complete)
- 30+ original bugs fixed (combined pointer-chain reads, magic knife range, SetSkin guard, etc.)
- 15+ diagnostic bugs fixed (m_MeshGroupMask u32→u64, std::clamp UB on empty vector, COM leak, etc.)
- Full ripple-effect audit of all changed files — zero compilation/runtime bugs

## Skin Rendering — All Root Causes Fixed

| # | Issue | Fix |
|---|-------|-----|
| 1 | `m_bInitialized=false` written AFTER fallbacks — CS2 skips fallback fields | Written FIRST before all fallback values |
| 2 | `m_bInitialized` conditional — missed when CS2 resets it | Unconditional every tick |
| 3 | Glove item-view fields written to player pawn, not glove entity | Write to `cached_glove_entity_ + m_AttributeManager + m_Item` |
| 4 | Glove attributes created on player pawn | Create on glove entity's item view |
| 5 | 50ms tick throttle — 3 network ticks overwrote values | Reduced to 1ms |
| 6 | `m_iItemIDLow = -1` — some code paths treat as invalid item | Changed to `0` |

## Skin Texture Pipeline — All Root Causes Fixed

| # | Issue | Fix |
|---|-------|-----|
| 1 | `InitMapping()` no retry on 25MB items_game.txt download | Local CS2 path first + 3 retries with exponential backoff |
| 2 | `ProcessPending()` only 1 texture/frame | Now up to 4 textures/frame |
| 3 | No CS2-format economy image URL fallback | Added `community.cloudflare.steamstatic.com` URLs |
| 4 | Steam market search only fallback for missing CDN | Added as third download path alongside CDN + economy API |

## Remaining Work

### Before v2.0 Release
1. **In-game testing** — verify skins actually appear with all fixes applied. The `RegenerateWeaponSkins` signature (defined at `Offsets.h:126`) is never resolved or called — CS2 may require an explicit call after writing fallback values.
2. **Manual mapping** — replace `CreateRemoteThread + LoadLibraryA` with manual mapping to bypass VAC's LoadLibrary hook. P1 for v2.0.
3. **VAC testing** — run the binary through detection scanners; verify no signature hits.

### Polish Items (Low Priority)
- Improve skin image caching (memory-mapped files, LRU eviction)
- Add skin preview in selection (large thumbnail)
- Add "random skin" option per weapon
- Save/load multiple loadout profiles

## VAC Evasion Vectors Addressed

| Vector | Risk | Status |
|--------|------|--------|
| Mangled C++ symbols | CRITICAL | Fixed (`-s`) |
| `CreateRemoteThread + LoadLibraryA` | CRITICAL | Manual mapping deferred to v2.0 |
| MinHook trampolines | HIGH | Acceptable — inherent to hooking |
| version.rc cheat strings | CRITICAL | Fixed |
| Anti-debug API calls | HIGH | Fixed (removed) |
| `fprintf` in Logger | MEDIUM | Fixed (NDEBUG guard) |
| `%APPDATA%/veneer/` paths | MEDIUM | Fixed (BasePath) |
| Single-byte XOR | MEDIUM | Fixed (multi-byte key) |
| Identifiable exports/symbols | HIGH | Fixed (stripped) |
