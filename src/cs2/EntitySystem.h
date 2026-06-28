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
    u64 entity_list = mem.Read<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list) return 0;

    u64 list_entry = mem.Read<u64>(entity_list + 0x8 * ((handle & 0x7FFF) >> 9) + 0x10);
    if (!list_entry) return 0;

    return mem.Read<u64>(list_entry + 0x70 * (handle & 0x1FF));
}

/// Get the local player pawn.
inline u64 GetLocalPlayer(ProcessMemory& mem) {
    return mem.Read<u64>(mem.Client() + Offsets::dwLocalPlayerPawn);
}

/// Get the local player controller.
inline u64 GetLocalController(ProcessMemory& mem) {
    return mem.Read<u64>(mem.Client() + Offsets::dwLocalPlayerController);
}

/// Get all weapons belonging to a player pawn.
inline std::vector<u64> GetWeapons(ProcessMemory& mem, u64 local_player) {
    std::vector<u64> weapons;

    u64 weapon_services = mem.Read<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services) return weapons;

    u32 weapon_count = mem.Read<u32>(weapon_services + Offsets::m_hMyWeapons);
    u64 weapon_entry = mem.Read<u64>(weapon_services + Offsets::m_hMyWeapons + sizeof(u64));
    if (!weapon_entry) return weapons;

    for (u64 i = 0; i < weapon_count && i < 64; ++i) {
        u32 handle = mem.Read<u32>(weapon_entry + sizeof(u32) * i);
        if (!handle) continue;

        u64 weapon = GetEntityByHandle(mem, handle);
        if (weapon) weapons.push_back(weapon);
    }

    return weapons;
}

/// Get the HUD view model arms.
inline u64 GetHudArms(ProcessMemory& mem) {
    u64 local = GetLocalPlayer(mem);
    if (!local) return 0;
    u32 handle = mem.Read<u32>(local + Offsets::m_hHudModelArms);
    return GetEntityByHandle(mem, handle);
}

/// Get the HUD weapon corresponding to a weapon entity by walking scene nodes.
inline u64 GetHudWeapon(ProcessMemory& mem, u64 weapon) {
    u64 arms_base = GetHudArms(mem);
    if (!arms_base) return 0;

    u64 arms_node = mem.Read<u64>(arms_base + Offsets::m_pGameSceneNode);
    if (!arms_node) return 0;

    for (u64 view_model = mem.Read<u64>(arms_node + Offsets::m_pChild);
         view_model;
         view_model = mem.Read<u64>(view_model + Offsets::m_pNextSibling)) {

        if (!view_model) continue;
        u64 owner = mem.Read<u64>(view_model + Offsets::m_pOwner);
        if (!owner) continue;

        u64 owner_entity = GetEntityByHandle(
            mem, mem.Read<u32>(owner + Offsets::m_hOwnerEntity));
        if (owner_entity == weapon) {
            return mem.Read<u64>(view_model + Offsets::m_pOwner);
        }
    }

    return 0;
}

/// Get the active weapon definition from local player's clipping weapon.
inline WeaponsEnum GetActiveWeaponDef(ProcessMemory& mem, u64 local_player) {
    u64 active_weapon = mem.Read<u64>(local_player + Offsets::m_pClippingWeapon);
    if (!active_weapon) return WeaponsEnum::None;

    u64 item = active_weapon + Offsets::m_AttributeManager + Offsets::m_Item;
    return static_cast<WeaponsEnum>(mem.Read<u16>(item + Offsets::m_iItemDefinitionIndex));
}
