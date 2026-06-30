#pragma once

#include "core/ProcessMemory.h"
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
constexpr u32 kEntityIdentitySize = 0x78;

/// Resolve an entity handle (CHandle) to its C_BaseEntity pointer.
/// Internally walks the chunked entity list, reads the CEntityIdentity,
/// then follows the m_pEntity field to the actual entity object.
/// Returns 0 for invalid handles (0xFFFFFFFF, out-of-range, etc.).
inline u64 GetEntityByHandle(ProcessMemory& mem, u32 handle) {
    if (!handle || handle == 0xFFFFFFFF) return 0;

    u32 chunk_idx = (handle & 0x7FFF) >> 9;
    if (chunk_idx >= kChunkArraySize) return 0;

    auto entity_list = mem.ReadSafe<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list || !*entity_list) return 0;

    auto list_entry = mem.ReadSafe<u64>(*entity_list + sizeof(u64) * chunk_idx + 0x10);
    if (!list_entry || !*list_entry) return 0;

    // CEntityIdentity at this slot — do NOT dereference it; read m_pEntity via offset
    u64 identity = *list_entry + kEntityIdentitySize * (handle & (kChunkSize - 1));
    auto entity = mem.ReadSafe<u64>(identity + Offsets::m_pEntity);
    return entity.value_or(0);
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

/// Get all weapons belonging to a player pawn.
inline std::vector<u64> GetWeapons(ProcessMemory& mem, u64 local_player) {
    std::vector<u64> weapons;

    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return weapons;

    auto weapon_count = mem.ReadSafe<u32>(*weapon_services + Offsets::m_hMyWeapons);
    if (!weapon_count) return weapons;

    auto weapon_entry = mem.ReadSafe<u64>(*weapon_services + Offsets::m_hMyWeapons + sizeof(u64));
    if (!weapon_entry || !*weapon_entry) return weapons;

    for (u64 i = 0; i < *weapon_count && i < 64; ++i) {
        auto handle = mem.ReadSafe<u32>(*weapon_entry + sizeof(u32) * i);
        if (!handle || !*handle) continue;

        u64 weapon = GetEntityByHandle(mem, *handle);
        if (weapon) weapons.push_back(weapon);
    }

    return weapons;
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
            auto entity = mem.ReadSafe<u64>(identity + Offsets::m_pEntity);
            if (!entity || !*entity) continue;
            u64 ent = *entity;
            if (ent == local_player) continue;

            // Check if this entity's m_Item points to our embedded item view
            u64 item_addr = mem.Read<u64>(ent + Offsets::m_AttributeManager + Offsets::m_Item);
            if (item_addr && item_addr != ~0ull && item_addr == target_iv)
                return ent;
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

    u64 item = mem.Read<u64>(weapon + Offsets::m_AttributeManager + Offsets::m_Item);
    if (!item || item == ~0ull) return WeaponsEnum::None;
    auto def = mem.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
    return static_cast<WeaponsEnum>(def.value_or(0));
}
