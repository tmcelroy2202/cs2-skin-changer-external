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

### Cycle 3 — Scan UI panels for UX issues

**Area scanned:** `SkinChangerPanel.cpp`, `SkinChangerPanel.h`

**Findings:**
- **Hardcoded version string:** `v1.5.7` at line 261 didn't match the v1.6.0 release. Updated to `v1.6.0`.
- Weapon filter and skin filter indices are correctly clamped to filtered-list bounds (no OOB).
- `selected_music_idx_` is re-synced every frame from `changer_.GetMusicKit()` — correct.
- Card click no longer auto-applies (already fixed in v1.6.0).
- No other UX bugs found.

### Cycle 4 — Scan ConfigManager for data-loss bugs

**Area scanned:** `ConfigManager.h`

**Findings:**
- Save format correctly escapes commas in skin names as `\,`.
- Load parser correctly handles escaped commas, variable part count, and comments.
- Knife/glove/music_kit save checks `> 0` correctly (default/unset has paint 0).
- No data-loss bugs found.



**Area scanned:** `SkinDatabase.h`, `WeaponTypes.h`

**Bug 1 — Magic knife range:** `IsKnifeDef(i32 def)` checked `def >= 500 && def <= 530` instead of iterating
the authoritative `kAllKnifeDefs` array. If CS2 adds a knife with def > 530 in a future update, the
check silently returns false — knives would fail to load skins and fall back to their vanilla appearance.

Fix: replace the magic range with a loop over `kAllKnifeDefs`. No perf concern (18 entries, called per-weapon
per frame in Tick and per card-select in the UI).

**Bug 2 — Missing guard in SetSkin:** `SetSkin` didn't guard against `weaponType == WeaponsEnum::None`.
If a caller accidentally passed a skin with None weaponType, it would match the first entry in
`active_skins` or push a bogus entry. Added early-return guard.

**Dead code removal:** `GetSkinIndex()` was defined but never called anywhere in the codebase.
Removed it (60 lines).

**Downstream effect check:**
- `IsKnifeDef` callers (SkinChanger.cpp:96, SkinChangerPanel.cpp:314) use the result as a boolean.
  No type/semantic change.
- `SetSkin` callers (SkinChangerPanel.cpp:430) always passes non-None weaponTypes. Safe.
- `GetSkinIndex` had zero call sites. Safe removal.

**Deferred:** None.

