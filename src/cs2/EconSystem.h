#pragma once

#include "core/ProcessMemory.h"
#include "core/Logging.h"
#include "Offsets.h"
#include "WeaponTypes.h"
#include "EntitySystem.h"

/* ── CS2 Econ System ────────────────────────────────────────────────────────
 *
 * Manages econ item attributes (paint kit, pattern, wear) on weapon items.
 * Reads a valid vtable from existing attributes in the CS2 process so that
 * the game doesn't crash when iterating the attribute list.
 *
 * ── Layout note ──────────────────────────────────────────────────────────
 * CAttributeList::m_Attributes is CUtlVector<CEconItemAttribute*>
 * (a vector of pointers to individually-allocated attribute structs).
 * This implementation uses a single allocation per batch:
 *
 *    block[0 .. kAttrCount * sizeof(CEconItemAttribute))
 *      ─ the kAttrCount attribute structs, laid out consecutively
 *    block[offset .. offset + kAttrCount * sizeof(u64))
 *      ─ the pointer array, each entry pointing into the struct area above
 *
 *    vec.ptr = block + ptr_array_offset
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
    u16     defIndex{};        // 0x30  — attribute definition index (CUtlStringToken)
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

        if (!ResolveVtable()) {
            LOG_WARN("EconItemAttributeManager::Create — failed to resolve vtable");
            return false;
        }

        constexpr u32 kPtrArrayOffset = kAttrCount * kAttrSize;
        constexpr u32 kBlockSize = kPtrArrayOffset + kAttrCount * sizeof(u64);

        CEconItemAttribute attrs[kAttrCount];
        attrs[0] = MakeAttribute(Attr_Paint,   static_cast<f32>(skin.Paint),   item);
        attrs[1] = MakeAttribute(Attr_Pattern, static_cast<f32>(skin.Pattern), item);
        attrs[2] = MakeAttribute(Attr_Wear,    skin.Wear,                      item);

        // Allocate new block FIRST so we never leave the item with no attrs
        u64 block = mem_.Allocate(0, kBlockSize);
        if (!block) {
            LOG_WARN("EconItemAttributeManager::Create — VirtualAllocEx failed");
            return false;
        }

        // Now detach any existing attributes (safe even if this fails)
        RemoveInternal(item);

        // Write attribute structs
        for (u32 i = 0; i < kAttrCount; ++i) {
            if (!mem_.Write(block + i * kAttrSize, attrs[i])) {
                LOG_WARN("EconItemAttributeManager::Create — Write attrs failed at i={}", i);
                mem_.Free(block);
                return false;
            }
        }

        // Write pointer array (each entry points into the struct area above)
        for (u32 i = 0; i < kAttrCount; ++i) {
            u64 p = block + i * kAttrSize;
            if (!mem_.Write<u64>(block + kPtrArrayOffset + i * sizeof(u64), p)) {
                LOG_WARN("EconItemAttributeManager::Create — Write ptrs failed at i={}", i);
                mem_.Free(block);
                return false;
            }
        }

        // Set the game's attribute vector: ptr → pointer array inside our block
        CPtrGameVector vec;
        vec.size = kAttrCount;
        vec.ptr  = block + kPtrArrayOffset;
        mem_.Write(item + Offsets::m_AttributeList + Offsets::m_Attributes, vec);
        return true;
    }

    void Remove(u64 item) noexcept {
        RemoveInternal(item);
    }

private:
    ProcessMemory& mem_;
    u64 cached_vtable_ = 0;
    static constexpr u32 kAttrCount = 3;
    static constexpr u32 kAttrSize  = sizeof(CEconItemAttribute);

    // Remove the attribute list and try to free our allocation.
    // Returns true on success or if nothing to remove.
    bool RemoveInternal(u64 item) noexcept {
        u64 attr_addr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        auto attrs = mem_.ReadSafe<CPtrGameVector>(attr_addr);
        if (!attrs || !attrs->size || !attrs->ptr) return true;

        // Zero out the vector so CS2 stops iterating it.
        // We do NOT try to free the old block — it could be game-owned memory
        // (if the game created attributes with the same count), and freeing it
        // would crash CS2.  Accept ~96 bytes leaked per skin change instead.
        mem_.Write(attr_addr, CPtrGameVector{});
        return true;
    }

    // Walk the player's weapons via GetWeapons (which handles both
    // C_NetworkUtlVectorBase layouts) to find an existing
    // CEconItemAttribute and cache its vtable.
    bool ResolveVtable() noexcept {
        if (cached_vtable_) return true;

        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;

        auto weapons = GetWeapons(mem_, *local_player);
        for (u64 weapon : weapons) {
            u64 item_addr = mem_.Read<u64>(weapon + Offsets::m_AttributeManager + Offsets::m_Item);
            if (!item_addr || item_addr == ~0ull) continue;
            auto attrs = mem_.ReadSafe<CPtrGameVector>(
                item_addr + Offsets::m_AttributeList + Offsets::m_Attributes);
            if (attrs && attrs->size > 0 && attrs->ptr) {
                auto vtable = mem_.ReadSafe<u64>(attrs->ptr);
                if (vtable && *vtable) {
                    if (mem_.ReadSafe<u8>(*vtable)) {
                        cached_vtable_ = *vtable;
                        LOG_INFO("EconItemAttributeManager — resolved vtable to 0x{:X}", *vtable);
                        return true;
                    }
                    LOG_WARN("EconItemAttributeManager — vtable 0x{:X} not readable", *vtable);
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
