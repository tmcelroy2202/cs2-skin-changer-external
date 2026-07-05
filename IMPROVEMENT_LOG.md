# Improvement Log

## Session: Autonomous Code Quality (2026-07-01)

[First 6 cycles — combined-read bugs, IsKnifeDef, SetSkin guard, UX, ConfigManager, CallRemoteThread removal]

---

## Session: v1.8.2 — Architecture Change + VAC Hardening (2026-07-05)

### Major: Standalone External Overlay (was DLL injection)

**Problem:** DLL injection via `CreateRemoteThread + LoadLibraryA` failed with "LoadLibraryA returned NULL". Root causes included:
- `std::filesystem` operations in DllMain under the loader lock
- `D3DCOMPILER_47.dll` not resolvable in CS2's process context
- VAC blocking LoadLibraryA in remote threads
- Insert key not working via WndProcHook (CS2 uses raw input)

**Fix:** Replaced the entire architecture. No DLL, no injection, no MinHook.

| Before (v1.8.1) | After (v1.8.2) |
|---|---|
| DLL injected into CS2 via remote thread | Standalone EXE with own overlay window |
| `libCS2SkinChanger.dll` (2.2MB) + `CS2SkinChanger.exe` (loader) | Single `CS2SkinChanger.exe` (2.1MB) |
| Hooks CS2's Present via MinHook | Creates own D3D11 device (no hooking) |
| In-process memory access (memcpy) | External RPM/WPM via XOR-obfuscated API |
| WndProcHook for Insert key (never received) | Overlay's own message loop (always works) |
| DllMain with loader lock issues | Normal WinMain with full CRT access |

### Key Changes to ProcessMemory.h
- Added `Open(DWORD pid)` — open by process ID
- Added `IsAlive()` — check if target process still exists (via `GetExitCodeProcess`)
- Added `NativeHandle()` — expose process handle

### New Main Loop
- 60 FPS capped loop with `std::this_thread::sleep_for(16ms)`
- `PeekMessage` Windows message loop (non-blocking)
- CS2 alive check per frame — auto-closes if CS2 exits
- Insert key polled via `GetAsyncKeyState` in overlay's own window context
- Clean shutdown with resource cleanup

### Removed
- `libCS2SkinChanger.dll` target
- MinHook dependency
- All injection code (`CreateRemoteThread`, `LoadLibraryA`, `VirtualAllocEx`)
- DLL main.cpp (DllMain, PresentHook, InitThread, ShutdownDLL, FindPresentFunction)
- WndProcHook (replaced by overlay's native window proc)
- `CSC_BUILD_STANDALONE` compile flag

### VAC Evasion Improvements
| Vector | Status |
|--------|--------|
| CreateRemoteThread + LoadLibraryA | **Eliminated** — no injection |
| MinHook trampolines | **Eliminated** — no hooking |
| Running code inside CS2 | **Eliminated** — external only |
| DllMain under loader lock | **Eliminated** — no DllMain |
| Process enumeration (CreateToolhelp32Snapshot) | Moved to EXE side |

### Assets
| Release Asset | Size |
|---------------|------|
| `CS2SkinChanger.exe` | 2.1MB |
| `VeneeR-v1.8.2-source.zip` | ~4MB |
| ~~`libCS2SkinChanger.dll`~~ | Removed |
