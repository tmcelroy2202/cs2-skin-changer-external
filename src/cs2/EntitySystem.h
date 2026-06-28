#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "WeaponTypes.h"

/* ── CS2 Entity System ──────────────────────────────────────────────────────
 *
 * Port of the reference's entity list handling for CS2.
 * Uses the chunked entity list structure (3 chunks × 512 slots).
 * ──────────────────────────────────────────────────────────────────────────── */

constexpr u32 kChunkSize       = 512;
constexpr u32 kChunkArraySize  = 3;
constexpr u32 kMaxEntities     = kChunkArraySize * kChunkSize;
constexpr u32 kEntityIdentitySize = 0x70;

inline u64 GetEntityByHandle(ProcessMemory& mem, u32 handle) {
    if (!handle || handle == 0xFFFFFFFF) return 0;

    auto entity_list = mem.ReadSafe<u64>(mem.Client() + Offsets::dwEntityList);
    if (!entity_list || !*entity_list) return 0;

    u32 index = handle & 0x7FFF;
    u32 chunk = index >> 9;
    u32 slot  = index & 0x1FF;

    auto list_entry = mem.ReadSafe<u64>(*entity_list + 0x10 + chunk * 0x8);
    if (!list_entry || !*list_entry) return 0;

    auto result = mem.ReadSafe<u64>(*list_entry + slot * kEntityIdentitySize);
    return result.value_or(0);
}

inline u64 GetLocalPlayer(ProcessMemory& mem) {
    auto val = mem.ReadSafe<u64>(mem.Client() + Offsets::dwLocalPlayerPawn);
    return val.value_or(0);
}

inline u64 GetLocalController(ProcessMemory& mem) {
    auto val = mem.ReadSafe<u64>(mem.Client() + Offsets::dwLocalPlayerController);
    return val.value_or(0);
}

inline std::vector<u64> GetWeapons(ProcessMemory& mem, u64 local_player) {
    std::vector<u64> weapons;

    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return weapons;

    u64 services = *weapon_services;
    auto weapon_count = mem.ReadSafe<u64>(services + Offsets::m_hMyWeapons);
    if (!weapon_count || *weapon_count == 0 || *weapon_count > 64) return weapons;

    auto weapon_entry = mem.ReadSafe<u64>(services + Offsets::m_hMyWeapons + sizeof(u64));
    if (!weapon_entry || !*weapon_entry) return weapons;

    for (u64 i = 0; i < *weapon_count; ++i) {
        auto handle = mem.ReadSafe<u32>(*weapon_entry + sizeof(u32) * i);
        if (!handle || !*handle || *handle == 0xFFFFFFFF) continue;

        u64 weapon = GetEntityByHandle(mem, *handle);
        if (weapon) weapons.push_back(weapon);
    }

    return weapons;
}

inline u64 GetHudArms(ProcessMemory& mem) {
    u64 local = GetLocalPlayer(mem);
    if (!local) return 0;
    auto handle = mem.ReadSafe<u32>(local + Offsets::m_hHudModelArms);
    if (!handle || !*handle) return 0;
    return GetEntityByHandle(mem, *handle);
}

inline u64 GetHudWeapon(ProcessMemory& mem, u64 local_player, u64 weapon) {
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
        if (owner_entity == weapon)
            return *owner;
    }

    return 0;
}

inline WeaponsEnum GetActiveWeaponDef(ProcessMemory& mem, u64 local_player) {
    auto weapon_services = mem.ReadSafe<u64>(local_player + Offsets::m_pWeaponServices);
    if (!weapon_services || !*weapon_services) return WeaponsEnum::None;

    auto handle = mem.ReadSafe<u32>(*weapon_services + Offsets::m_hActiveWeapon);
    if (!handle || !*handle) return WeaponsEnum::None;

    u64 weapon = GetEntityByHandle(mem, *handle);
    if (!weapon) return WeaponsEnum::None;

    u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
    auto def = mem.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
    return static_cast<WeaponsEnum>(def.value_or(0));
}
