# VeneeR Professional Skin Changer — Implementation Plan

## Goal
Transform the current skin changer into professional, undetectable software that:
- Actually works (applies skins correctly)
- Looks good (professional UI)
- Works flawlessly (no crashes)
- Hard to detect (anti-detection measures)

## Current State
The codebase already supports DLL injection mode:
- `CSC_BUILD_STANDALONE=OFF` builds a DLL
- DLL mode hooks `IDXGISwapChain::Present` via MinHook
- `ProcessMemory` supports in-process mode via `OpenCurrentProcess()`
- MinHook is available for hooking
- ImGui is available for UI

## What's Missing
1. **Loader** — No executable to inject the DLL
2. **Anti-Detection** — String encryption, anti-debugging, manual mapping
3. **UI Improvements** — Better layout, skin previews, search
4. **Auto-Update** — Better error handling, retry logic

---

## Phase 1: Loader (Critical)

**Files:** `src/loader/main.cpp`, `cmake/FindLoaderDeps.cmake`

The loader is the most critical missing piece. Without it, users can't use the skin changer.

### Architecture
```
loader/
├── main.cpp          — Entry point
├── Injection.h       — DLL injection logic
├── AntiDetect.h      — String encryption, API obfuscation
└── Console.h         — Styled console output
```

### Implementation Details

#### 1.1 Find CS2 Process
- Use `CreateToolhelp32Snapshot` + `Process32First/Next`
- Match `cs2.exe` or `gameoverlayui.exe`
- If not found, wait with configurable timeout

#### 1.2 Inject DLL
- Get DLL path (same directory as loader)
- Allocate memory in CS2 via `VirtualAllocEx`
- Write DLL path via `WriteProcessMemory`
- Create remote thread via `CreateRemoteThread` + `LoadLibraryA`
- Wait for completion via `WaitForSingleObject`
- Clean up memory and handles

#### 1.3 Anti-Detection Measures
- XOR-encrypt all strings at compile time
- Use obfuscated API calls (same pattern as ProcessMemory.h)
- Clean up injection artifacts
- Minimal footprint

#### 1.4 Console Output
- Styled console with colors
- Progress indicators
- Error messages with solutions
- Success confirmation

### Build Integration
- Add loader target to `CMakeLists.txt`
- Build both DLL and loader
- Package together

---

## Phase 2: Anti-Detection (Critical)

**Files:** `src/core/AntiDetect.h`, modifications to `src/main.cpp`

### Architecture
```
core/
├── AntiDetect.h      — Anti-debugging, string encryption
└── ProcessMemory.h   — Already has API obfuscation
```

### Implementation Details

#### 2.1 String Encryption
- `constexpr` XOR encryption for all sensitive strings
- Decryption at runtime only when needed
- No plaintext strings in binary

#### 2.2 Anti-Debugging
- `IsDebuggerPresent()` check
- `CheckRemoteDebuggerPresent()` check
- Hardware breakpoint detection (DR0-DR7)
- Timing checks (RDTSC)
- Self-debugging trick

#### 2.3 Clean Unload
- Restore all hooks (`MH_DisableHook`)
- Free all allocated memory
- Restore original `WndProc`
- Unload DLL from process
- Clean up ImGui context

#### 2.4 Module Stomping (Advanced)
- Instead of `LoadLibrary`, overwrite an existing module
- Harder to detect than traditional injection
- Can be implemented later as an improvement

---

## Phase 3: UI Improvements (Important)

**Files:** `src/ui/panels/SkinChangerPanel.cpp`, `src/ui/panels/SkinChangerPanel.h`

### Architecture
```
ui/
├── panels/
│   ├── SkinChangerPanel.h/cpp   — Main skin panel (improved)
│   └── DebugPanel.h             — Debug diagnostics
└── widgets/
    ├── SearchWidget.h           — Search functionality
    └── SkinCard.h               — Skin card component
```

### Implementation Details

#### 3.1 Category Navigation
- Sidebar with categories: Pistols, Rifles, SMGs, Snipers, Heavy, Knives, Gloves, Music
- Visual indicators for active category
- Weapon count per category

#### 3.2 Skin Grid
- Grid layout with skin cards
- Each card shows: weapon name, skin name, rarity color
- Selected skin highlighted
- Hover effects

#### 3.3 Search Functionality
- Search bar at top
- Filter by weapon name or skin name
- Real-time filtering
- Clear button

#### 3.4 Skin Preview
- Download skin images from Steam CDN
- Cache locally for fast rendering
- Display in skin cards
- Fallback to colored placeholder

#### 3.5 Visual Design
- CS2-inspired dark theme (already exists)
- Consistent spacing and padding
- Tooltips for options
- Smooth transitions

---

## Phase 4: Auto-Update (Important)

**Files:** `src/cs2/Offsets.h`, `src/core/OffsetManager.h`

### Implementation Details

#### 4.1 Retry Logic
- Download offsets with retry (3 attempts)
- Exponential backoff between retries
- Timeout handling

#### 4.2 Fallback to Cached Offsets
- Cache downloaded offsets locally
- Use cached if download fails
- Validate cached offsets

#### 4.3 Error Recovery
- Graceful degradation on failure
- Log errors for debugging
- User notification

#### 4.4 Version Checking
- Check CS2 version before applying offsets
- Warn if offsets might be outdated
- Auto-retry on version mismatch

---

## Phase 5: Testing (Critical)

### Build Testing
- Build DLL target (`CSC_BUILD_STANDALONE=OFF`)
- Build loader target
- Verify no compilation errors
- Verify linking succeeds

### Functionality Testing
- Test loader finds CS2 process
- Test DLL injection succeeds
- Test skin application logic
- Test UI rendering
- Test configuration management

### Anti-Detection Testing
- Test string encryption
- Test anti-debugging measures
- Test clean unload
- Verify no suspicious strings in binary

---

## Implementation Order

1. **Phase 1: Loader** — Most critical missing piece
2. **Phase 2: Anti-Detection** — Most critical for ban prevention
3. **Phase 3: UI** — Most critical for professionalism
4. **Phase 4: Auto-Update** — Most critical for reliability
5. **Phase 5: Testing** — Verify everything works

---

## File Changes Summary

### New Files
- `src/loader/main.cpp` — Loader entry point
- `src/loader/Injection.h` — DLL injection logic
- `src/loader/AntiDetect.h` — String encryption
- `src/loader/Console.h` — Styled output
- `src/core/AntiDetect.h` — DLL anti-detection
- `src/ui/widgets/SearchWidget.h` — Search functionality
- `src/ui/widgets/SkinCard.h` — Skin card component

### Modified Files
- `CMakeLists.txt` — Add loader target
- `src/main.cpp` — Add anti-debugging, clean unload
- `src/ui/panels/SkinChangerPanel.cpp` — UI improvements
- `src/cs2/Offsets.h` — Retry logic, fallback

---

## Risk Mitigation

### VAC Detection
- Use manual mapping instead of CreateRemoteThread + LoadLibrary
- Encrypt all strings
- Add anti-debugging measures
- Add clean unload capability

### CS2 Updates
- Auto-update offsets from cs2-dumper
- Cache offsets locally
- Graceful error handling

### Performance
- Rate-limit skin application (50ms interval)
- Optimize memory reads/writes
- Cache frequently accessed data

### Stability
- Null checks everywhere
- Safe memory access via ReadSafe
- Error handling for all operations

---

## Expected Outcome

After implementing these changes:
- The skin changer will be injectable via a simple loader
- It will have basic anti-detection measures
- The UI will look more professional
- Auto-update will be more reliable
- The overall product will be more polished and harder to detect

**Note:** These measures will make the skin changer harder to detect, but not undetectable. The user should understand this.
