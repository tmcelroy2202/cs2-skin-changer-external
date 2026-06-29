#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "WeaponTypes.h"
#include "EntitySystem.h"

/* ── CS2 Econ System ────────────────────────────────────────────────────────
 *
 * Manages econ item attributes (paint kit, pattern, wear) on weapon items.
 * Reads a valid vtable from existing attributes in the CS2 process so that
 * the game doesn't crash when iterating the attribute list.
 * ──────────────────────────────────────────────────────────────────────────── */

struct CPtrGameVector {
    u32 size{};
    u32 pad{};
    u64 ptr{};
};

struct CEconItemAttribute {
    u64     vtable{};          // 0x00  — must point to valid CEconItemAttribute vtable
    u64     owner{};           // 0x08  — pointer to the owning C_EconItemView
    u8      pad_0010[32]{};   // 0x10
    u16     defIndex{};        // 0x30  — attribute definition index
    u8      pad_0032[2]{};
    f32     value{};           // 0x34
    f32     initValue{};       // 0x38
    i32     refundableCurrency{};
    bool    setBonus{};
    u8      pad_0041[7]{};
};

enum ItemAttributeDefinition : u16 {
    Attr_Default = 0,
    Attr_Paint   = 6,
    Attr_Pattern = 7,
    Attr_Wear    = 8,
};

class EconItemAttributeManager {
public:
    explicit EconItemAttributeManager(ProcessMemory& mem) : mem_(mem) {}

    bool Create(u64 item, const SkinInfo_t& skin) noexcept {
        if (!skin.Paint) return false;

        // Must have a valid vtable — without it CS2 crashes when iterating attrs
        if (!ResolveVtable()) return false;

        std::vector<CEconItemAttribute> attrs;
        attrs.push_back(MakeAttribute(Attr_Paint,   static_cast<f32>(skin.Paint), item));
        attrs.push_back(MakeAttribute(Attr_Pattern, static_cast<f32>(skin.Pattern), item));
        attrs.push_back(MakeAttribute(Attr_Wear,    skin.Wear, item));

        auto existing = mem_.ReadSafe<CPtrGameVector>(
            item + Offsets::m_AttributeList + Offsets::m_Attributes);
        if (!existing) return false;
        if (existing->size || existing->ptr) {
            Remove(item);
        }

        u64 block = mem_.Allocate();
        if (!block) return false;

        for (usize i = 0; i < attrs.size(); ++i) {
            if (!mem_.Write(block + i * sizeof(CEconItemAttribute), attrs[i])) {
                mem_.Free(block);
                return false;
            }
        }

        CPtrGameVector vec;
        vec.size = attrs.size();
        vec.ptr  = block;
        mem_.Write(item + Offsets::m_AttributeList + Offsets::m_Attributes, vec);
        return true;
    }

    void Remove(u64 item) noexcept {
        u64 attr_addr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        auto attrs = mem_.ReadSafe<CPtrGameVector>(attr_addr);
        if (!attrs || !attrs->size || !attrs->ptr) return;

        mem_.Write(attr_addr, CPtrGameVector{});
        mem_.Free(attrs->ptr);
    }

private:
    ProcessMemory& mem_;
    u64 cached_vtable_ = 0;

    // Walk the player's weapons to find an existing CEconItemAttribute
    // and cache its vtable.  Called once; subsequent calls are no-ops.
    bool ResolveVtable() noexcept {
        if (cached_vtable_) return true;

        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;

        u64 weapons_addr = *local_player + Offsets::m_pWeaponServices;
        auto svc = mem_.ReadSafe<u64>(weapons_addr);
        if (!svc || !*svc) return false;

        auto count = mem_.ReadSafe<u32>(*svc + Offsets::m_hMyWeapons);
        if (!count || !*count) return false;

        auto list_ptr = mem_.ReadSafe<u64>(*svc + Offsets::m_hMyWeapons + sizeof(u64));
        if (!list_ptr || !*list_ptr) return false;

        for (u32 i = 0; i < *count && i < 64; ++i) {
            auto handle = mem_.ReadSafe<u32>(*list_ptr + sizeof(u32) * i);
            if (!handle || !*handle) continue;

            // Resolve handle → entity pointer (same formula as GetEntityByHandle)
            auto entity_list = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwEntityList);
            if (!entity_list || !*entity_list) continue;

            auto list_entry = mem_.ReadSafe<u64>(*entity_list + sizeof(u64) * ((*handle & 0x7FFF) >> 9) + 0x10);
            if (!list_entry || !*list_entry) continue;

            auto weapon = mem_.ReadSafe<u64>(*list_entry + kEntityIdentitySize * (*handle & (kChunkSize - 1)));
            if (!weapon || !*weapon) continue;

            u64 item_addr = mem_.Read<u64>(*weapon + Offsets::m_AttributeManager + Offsets::m_Item);
            if (!item_addr || item_addr == ~0ull) continue;
            auto attrs = mem_.ReadSafe<CPtrGameVector>(
                item_addr + Offsets::m_AttributeList + Offsets::m_Attributes);
            if (attrs && attrs->size > 0 && attrs->ptr) {
                auto vtable = mem_.ReadSafe<u64>(attrs->ptr);
                if (vtable && *vtable) {
                    cached_vtable_ = *vtable;
                    return true;
                }
            }
        }
        return false;
    }

    CEconItemAttribute MakeAttribute(u16 def, f32 value, u64 owner) noexcept {
        CEconItemAttribute attr{};
        attr.vtable    = cached_vtable_;
        attr.owner     = owner;
        attr.defIndex  = def;
        attr.initValue = value;
        attr.value     = value;
        return attr;
    }
};
