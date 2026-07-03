#pragma once

#include "core/ProcessMemory.h"
#include "core/Logging.h"
#include "Offsets.h"
#include "WeaponTypes.h"

/* ── CS2 Entity System ──────────────────────────────────────────────────────
 *
 * Port of the reference's entity list handling for CS2.
 * Uses the chunked entity list structure (3 chunks × 512 slots).
 * ──────────────────────────────────────────────────────────────────────────── */

// ── Entity list constants ─────────────────────────────────────────────────────
constexpr u32 kChunkSize       = 512;
constexpr u32 kChunkArraySize  = 3;
constexpr u32 kMaxEntities     = kChunkArraySize * kChunkSize;
constexpr u32 kEntityIdentitySize = 0x70;

/// Resolve an entity handle (CHandle) to its C_BaseEntity pointer.
/// The chunked entity list stores the entity pointer as the first 8 bytes
/// of each CEntityIdentity slot (stride = kEntityIdentitySize).
/// Returns 0 for invalid handles (0xFFFFFFFF, out-of-range, etc.).
///
/// Handle bit layout in CS2:
///   bits 0-8:   entry index (0-511, within a chunk)
///   bits 9-14:  chunk index (0-63, into the entity list array)
///   bits 15-30: serial number (for lifetime validation)
///   bit 31:     invalid flag
inline u64 GetEntityByHandle(ProcessMemory& mem, u32 handle) {
    if (!handle || handle == 0xFFFFFFFF) return 0;

    u32 entry_idx = handle & (kChunkSize - 1);   // 0x1FF (9 bits)
    u32 chunk_idx = (handle >> 9) & 0x3F;          // 0x3F (6 bits, 0-63)

    auto entity_list = mem.ReadSafe<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list || !*entity_list) return 0;

    auto list_entry = mem.ReadSafe<u64>(*entity_list + sizeof(u64) * chunk_idx + 0x10);
    if (!list_entry || !*list_entry) return 0;

    u64 slot_addr = *list_entry + kEntityIdentitySize * entry_idx;
    return mem.ReadSafe<u64>(slot_addr).value_or(0);
}

/// Get the local player pawn.
inline u64 GetLocalPlayer(ProcessMemory& mem) {
    auto val = mem.ReadSafe<u64>(mem.Client() + Offsets::dwLocalPlayerPawn);
    return val.value_or(0);
}

/// Get the local player controller.
inline u64 GetLocalController(ProcessMemory& mem) {
    auto val = mem.ReadSafe<u64>(mem.Client() + Offsets::dwLocalPlayerController);
    return val.value_or(0);
}

/// Read a weapons vector from CPlayer_WeaponServices using a given
/// C_NetworkUtlVectorBase layout and return resolved entity pointers.
/// The offset `base` is `m_hMyWeapons` within weapon services.
/// `size_offset` is the offset of m_nSize within the C_NetworkUtlVectorBase,
/// `ptr_offset` is the offset of m_pElements.
static std::vector<u64> ReadWeaponVector(ProcessMemory& mem, u64 services_base,
                                          u64 size_offset, u64 ptr_offset) {
    std::vector<u64> out;
    auto count = mem.ReadSafe<u32>(services_base + Offsets::m_hMyWeapons + size_offset);
    if (!count || !*count) return out;

    auto entry_ptr = mem.ReadSafe<u64>(services_base + Offsets::m_hMyWeapons + ptr_offset);
    if (!entry_ptr || !*entry_ptr) return out;

    for (u64 i = 0; i < *count && i < 64; ++i) {
        auto handle = mem.ReadSafe<u32>(*entry_ptr + sizeof(u32) * i);
        if (!handle || !*handle) continue;
        u64 weapon = GetEntityByHandle(mem, *handle);
        if (weapon) out.push_back(weapon);
    }
    return out;
}

/// Get all weapons belonging to a player pawn.
/// Tries both known C_NetworkUtlVectorBase layouts.
inline std::vector<u64> GetWeapons(ProcessMemory& mem, u64 local_player) {
    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return {};

    // Pattern A  (most common):  m_nSize at +0x00,  m_pElements at +0x08
    auto weapons = ReadWeaponVector(mem, *weapon_services, 0x00, 0x08);
    if (!weapons.empty()) return weapons;

    // Pattern B (CUtlMemory-based): m_pMemory at +0x00, m_nAllocation at +0x08, m_Size at +0x10
    weapons = ReadWeaponVector(mem, *weapon_services, 0x10, 0x00);
    if (!weapons.empty()) return weapons;

    return {};
}

/// Try to get the active weapon from CPlayer_WeaponServices directly.
inline u64 GetActiveWeapon(ProcessMemory& mem, u64 local_player) {
    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return 0;

    auto handle = mem.ReadSafe<u32>(*weapon_services + Offsets::m_hActiveWeapon);
    if (!handle || !*handle) return 0;

    return GetEntityByHandle(mem, *handle);
}

/// Get the HUD view model arms.
inline u64 GetHudArms(ProcessMemory& mem) {
    u64 local = GetLocalPlayer(mem);
    if (!local) return 0;
    auto handle = mem.ReadSafe<u32>(local + Offsets::m_hHudModelArms);
    if (!handle) return 0;
    return GetEntityByHandle(mem, *handle);
}

/// Get the HUD weapon corresponding to a weapon entity by walking scene nodes.
inline u64 GetHudWeapon(ProcessMemory& mem, u64 weapon) {
    u64 arms_base = GetHudArms(mem);
    if (!arms_base) return 0;

    auto arms_node = mem.ReadSafe<u64>(arms_base + Offsets::m_pGameSceneNode);
    if (!arms_node || !*arms_node) return 0;

    for (u64 view_model = mem.Read<u64>(*arms_node + Offsets::m_pChild);
         view_model;
         view_model = mem.Read<u64>(view_model + Offsets::m_pNextSibling)) {

        if (!view_model) continue;
        // m_pOwner in CGameSceneNode is a CEntityHandle (u32), not a pointer
        auto owner_handle = mem.ReadSafe<u32>(view_model + Offsets::m_pOwner);
        if (!owner_handle || !*owner_handle) continue;
        u64 owner_entity = GetEntityByHandle(mem, *owner_handle);
        if (owner_entity == weapon)
            return view_model;
    }

    return 0;
}

/// Find the C_EconGloves entity by scanning the entity list.
/// The glove entity's m_Item pointer points to the embedded C_EconItemView
/// at local_player + m_EconGloves.
inline u64 FindGloveEntity(ProcessMemory& mem, u64 local_player) {
    if (!local_player) return 0;
    u64 target_iv = local_player + Offsets::m_EconGloves;

    auto entity_list_base = mem.ReadSafe<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list_base || !*entity_list_base) return 0;

    for (u32 chunk_idx = 0; chunk_idx < kChunkArraySize; ++chunk_idx) {
        auto list_entry = mem.ReadSafe<u64>(*entity_list_base + sizeof(u64) * chunk_idx + 0x10);
        if (!list_entry || !*list_entry) continue;

        for (u32 slot = 0; slot < kChunkSize; ++slot) {
            u64 identity = *list_entry + kEntityIdentitySize * slot;
            auto entity = mem.ReadSafe<u64>(identity);
            if (!entity || !*entity) continue;
            u64 ent = *entity;
            if (ent == local_player) continue;

            // Check if this entity's m_Item points to our embedded item view
            u64 attr_mgr = mem.Read<u64>(ent + Offsets::m_AttributeManager);
            if (!attr_mgr || attr_mgr == ~0ull) continue;
            u64 item_addr = mem.Read<u64>(attr_mgr + Offsets::m_Item);
            if (item_addr && item_addr != ~0ull && item_addr == target_iv) {
                LOG_INFO("FindGloveEntity — found at 0x{:X}", ent);
                return ent;
            }
        }
    }
    return 0;
}

/// Get the active weapon definition from the player's weapon services.
inline WeaponsEnum GetActiveWeaponDef(ProcessMemory& mem, u64 local_player) {
    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return WeaponsEnum::None;

    auto handle = mem.ReadSafe<u32>(*weapon_services + Offsets::m_hActiveWeapon);
    if (!handle || !*handle) return WeaponsEnum::None;

    u64 weapon = GetEntityByHandle(mem, *handle);
    if (!weapon) return WeaponsEnum::None;

    u64 attr_mgr = mem.Read<u64>(weapon + Offsets::m_AttributeManager);
    if (!attr_mgr || attr_mgr == ~0ull) return WeaponsEnum::None;
    u64 item = mem.Read<u64>(attr_mgr + Offsets::m_Item);
    if (!item || item == ~0ull) return WeaponsEnum::None;
    auto def = mem.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
    return static_cast<WeaponsEnum>(def.value_or(0));
}
