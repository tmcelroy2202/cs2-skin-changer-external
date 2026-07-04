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
 * CAttributeList::m_Attributes is CUtlVectorEmbeddedNetworkVar<CEconItemAttribute>
 * (a vector of embedded attribute structs, stored by value).
 *
 * m_AttributeManager (C_AttributeContainer) and m_Item (C_EconItemView) are
 * EMBEDDED structs — not pointers.  The item view address is computed as a
 * flat offset chain:
 *
 *   item = weapon + m_AttributeManager + m_Item
 *
 * The CUtlVectorEmbeddedNetworkVar uses the C_NetworkUtlVectorBase layout:
 *   +0x00: u32 m_nSize
 *   +0x04: u32 pad
 *   +0x08: u64 m_pElements  (pointer to array of CEconItemAttribute structs)
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

        if (!ResolveVtable(item)) {
            LOG_WARN("EconItemAttributeManager::Create — failed to resolve vtable");
            return false;
        }

        // Elements are stored by value (not by pointer), so no pointer array needed.
        constexpr u32 kBlockSize = kAttrCount * kAttrSize;

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

        // Write attribute structs directly (no pointer indirection)
        for (u32 i = 0; i < kAttrCount; ++i) {
            if (!mem_.Write(block + i * kAttrSize, attrs[i])) {
                LOG_WARN("EconItemAttributeManager::Create — Write attrs failed at i={}", i);
                mem_.Free(block);
                return false;
            }
        }

        // NOW detach the old attributes — the new block is fully written.
        RemoveInternal(item);

        // Set the game's attribute vector: ptr points directly to the structs
        CPtrGameVector vec;
        vec.size = kAttrCount;
        vec.ptr  = block;
        if (!mem_.Write(item + Offsets::m_AttributeList + Offsets::m_Attributes, vec)) {
            LOG_WARN("EconItemAttributeManager::Create — Write vector failed");
            mem_.Free(block);
            return false;
        }

        // Track this block so RemoveInternal can free it next time.
        known_block_ = block;
        return true;
    }

    void Remove(u64 item) noexcept {
        RemoveInternal(item);
    }

private:
    ProcessMemory& mem_;
    u64 cached_vtable_ = 0;
    // Tracks the most recently allocated block so RemoveInternal can free it.
    u64 known_block_{};
    static constexpr u32 kAttrCount = 3;
    static constexpr u32 kAttrSize  = sizeof(CEconItemAttribute);

    // Remove the attribute list and try to free our allocation.
    // Returns true on success or if nothing to remove.
    bool RemoveInternal(u64 item) noexcept {
        u64 attr_addr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        auto attrs = mem_.ReadSafe<CPtrGameVector>(attr_addr);
        if (!attrs || !attrs->size || !attrs->ptr) return true;

        // Zero out the vector FIRST so CS2 stops iterating before we free.
        mem_.Write(attr_addr, CPtrGameVector{});

        // Now safe to free — CS2 no longer holds a reference.
        if (known_block_ && attrs->ptr == known_block_) {
            mem_.Free(known_block_);
        }
        known_block_ = 0;
        return true;
    }

    // Walk the player's weapons to find an existing CEconItemAttribute and
    // cache its vtable.
    // `hint_item` is an optional pre-verified item address from the caller.
    bool ResolveVtable(u64 hint_item = 0) noexcept {
        if (cached_vtable_) return true;

        // Try the caller's hint first — fastest path
        if (hint_item) {
            auto attrs = mem_.ReadSafe<CPtrGameVector>(
                hint_item + Offsets::m_AttributeList + Offsets::m_Attributes);
            if (attrs && attrs->size > 0 && attrs->ptr) {
                // Elements are stored by value — the first u64 at ptr IS the vtable.
                auto vtable = mem_.ReadSafe<u64>(attrs->ptr);
                if (vtable && *vtable && mem_.ReadSafe<u8>(*vtable)) {
                    cached_vtable_ = *vtable;
                    LOG_INFO("EconItemAttributeManager — resolved vtable (hint) to 0x{:X}", *vtable);
                    return true;
                }
            }
        }

        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;

        auto weapons = GetWeapons(mem_, *local_player);
        for (u64 weapon : weapons) {
            // m_AttributeManager and m_Item are EMBEDDED structs (not pointers).
            u64 item_addr = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
            if (hint_item && item_addr == hint_item) continue;  // already tried above

            auto attrs = mem_.ReadSafe<CPtrGameVector>(
                item_addr + Offsets::m_AttributeList + Offsets::m_Attributes);
            if (attrs && attrs->size > 0 && attrs->ptr) {
                // Elements are stored by value — the first u64 at ptr IS the vtable.
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
