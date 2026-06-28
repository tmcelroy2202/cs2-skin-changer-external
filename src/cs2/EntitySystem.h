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
constexpr u32 kEntityIdentitySize = 0x70;

/// Resolve an entity handle (CHandle) to its pointer address.
inline u64 GetEntityByHandle(ProcessMemory& mem, u32 handle) {
    auto entity_list = mem.ReadSafe<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list || !*entity_list) return 0;

    auto list_entry = mem.ReadSafe<u64>(*entity_list + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10);
    if (!list_entry || !*list_entry) return 0;

    auto result = mem.ReadSafe<u64>(*list_entry + 0x70 * (handle & 0x1FF));
    return result.value_or(0);
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
        auto owner = mem.ReadSafe<u64>(view_model + Offsets::m_pOwner);
        if (!owner || !*owner) continue;

        auto h_owner = mem.ReadSafe<u32>(*owner + Offsets::m_hOwnerEntity);
        if (!h_owner) continue;
        u64 owner_entity = GetEntityByHandle(mem, *h_owner);
        if (owner_entity == weapon) {
            auto result = mem.ReadSafe<u64>(view_model + Offsets::m_pOwner);
            return result.value_or(0);
        }
    }

    return 0;
}

/// Get the active weapon definition from local player's clipping weapon.
inline WeaponsEnum GetActiveWeaponDef(ProcessMemory& mem, u64 local_player) {
    auto active_weapon = mem.ReadSafe<u64>(local_player + Offsets::m_pClippingWeapon);
    if (!active_weapon || !*active_weapon) return WeaponsEnum::None;

    u64 item = *active_weapon + Offsets::m_AttributeManager + Offsets::m_Item;
    auto def = mem.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
    return static_cast<WeaponsEnum>(def.value_or(0));
}
