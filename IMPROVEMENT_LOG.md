# Improvement Log

## Session: Autonomous Code Quality (2026-07-01)

### Cycle 1 — Fix combined-read pointer-chain bug (4 instances)

**Area scanned:** `EconSystem.h`, `EntitySystem.h`, `SkinChanger.cpp`

**Bug found:** 4 locations compute `weapon + Offsets::m_AttributeManager + Offsets::m_Item` as a single
address offset instead of reading the pointer chain. Since `m_AttributeManager` is a pointer offset
within the weapon entity, reading `weapon + m_AttributeManager + m_Item` reads from a wrong address
(it adds the offsets together before dereferencing).

Fix: split each into two reads — first `mem.Read<u64>(weapon + m_AttributeManager)` to get the
pointer, then `mem.Read<u64>(attr_mgr + m_Item)` to get the item.

**Instances fixed:**
1. `EconSystem.h:147` — `ResolveVtable()` scanning weapons
2. `EntitySystem.h:164` — `FindGloveEntity()` entity list scan
3. `EntitySystem.h:185` — `GetActiveWeaponDef()` active weapon lookup
4. `SkinChanger.cpp:246` — `ResolveAccountId()` scanning weapons

**Downstream effect check:** grep confirmed zero remaining instances of the combined-read pattern.
All callers use the fixed signature (the functions' return types/semantics unchanged).

**Additional optimization:** `ResolveVtable()` now accepts a `hint_item` parameter from the caller
(pre-verified item address) so it doesn't redundantly scan all weapons when the caller already has
one. Skips the hint in the fallback loop to avoid re-reading.

**Deferred:** None.

### Cycle 2 — Fix IsKnifeDef magic range, add SetSkin guard, remove dead GetSkinIndex

**Area scanned:** `SkinDatabase.h`, `WeaponTypes.h`

**Bug 1 — Magic knife range:** `IsKnifeDef(i32 def)` checked `def >= 500 && def <= 530` instead of
iterating the authoritative `kAllKnifeDefs` array. If CS2 adds a knife with def > 530 in a future
update, the check silently returns false — knives would fail to load skins.

Fix: replace the magic range with a loop over `kAllKnifeDefs`.

**Bug 2 — Missing guard in SetSkin:** `SetSkin` didn't guard against `weaponType == WeaponsEnum::None`.
If a caller accidentally passed a skin with None weaponType, it would overwrite the first entry or
push a bogus entry. Added early-return guard.

**Dead code removal:** `GetSkinIndex()` was defined but never called anywhere in the codebase.
Removed it.

**Downstream effect check:**
- `IsKnifeDef` callers (SkinChanger.cpp:96, SkinChangerPanel.cpp:314) use result as boolean — safe.
- `SetSkin` callers (SkinChangerPanel.cpp:430) always pass non-None weaponTypes — safe.
- `GetSkinIndex` had zero call sites — safe removal.

**Deferred:** None.

### Cycle 3 — Scan UI panels for UX issues

**Area scanned:** `SkinChangerPanel.cpp`, `SkinChangerPanel.h`

**Findings:**
- **Hardcoded version string:** `v1.5.7` at line 261 didn't match the v1.6.0 release. Updated.
- Weapon/skin filter indices correctly clamped; no OOB access.
- `selected_music_idx_` re-synced from `changer_.GetMusicKit()` each frame — correct.
- No other UX bugs.

### Cycle 4 — Scan ConfigManager for data-loss bugs

**Area scanned:** `ConfigManager.h`

**Findings:**
- Comma escaping in skin names (`\,`) works correctly in save and load.
- Variable part count parsing (name, old_model, stat_trak) correct.
- Knife/glove/music_kit save checks `> 0` correctly.
- No data-loss bugs.

### Cycle 5 — Scan Offsets.h download/parse for failure modes

**Area scanned:** `Offsets.h` (load, save, parse, download, auto-update)

**Findings:**
- `ParseCs2Line` handles both namespaced and flat identifier formats.
- `LoadFromFile` safely loads into inline variables; `string_view` keys are ephemeral.
- `UpdateFromRemote` handles download failures gracefully.
- `DownloadUrl` uses correct XOR-obfuscated anti-detection pattern.
- Noted DRY violation: offset→name tables duplicated in 3 places.

### Cycle 6 — Remove CallRemoteThread dead code

**Area scanned:** `ProcessMemory.h`

**Bug:** `CallRemoteThread()` (using CreateRemoteThread) was dead code after the v1.6.0 freeze fix
removed it from Tick(), but the method + supporting API resolution remained — a safety risk.

**Changes applied:**
- Removed `CallRemoteThread` method entirely
- Removed `CreateRemoteThread` and `WaitForSingleObject` from Win32Api struct (typedefs, members,
  XOR-obfuscated arrays, resolution calls)
- Removed `PROCESS_CREATE_THREAD` flag from both `OpenProcess` calls

**Downstream effect check:** grep confirmed zero callers for removed symbols. Flag removal
doesn't affect memory operations. No functional impact.

---

## Summary

| Category | Count |
|----------|-------|
| Correctness bugs fixed | 6 (combined reads ×4, magic range, SetSkin guard) |
| Dead code removed | 2 (GetSkinIndex, CallRemoteThread + CreateRemoteThread/WaitForSingleObject) |
| UX issues fixed | 1 (version string) |
| Files modified | 8 |
| Deferred items | 0 |

**Deferred/flagged for human review:** None.

**Recurring patterns:**
- The `ent/weapon + m_AttributeManager + m_Item` combined-read bug appeared in 4 separate
  files — indicates a copy-paste pattern that was spread without review.
- Dead code (`GetSkinIndex`, `CallRemoteThread`, `LastOpOk`) accumulates when features are
  changed/removed without cleaning up.
