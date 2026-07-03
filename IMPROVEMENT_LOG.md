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

