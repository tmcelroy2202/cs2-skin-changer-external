#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "WeaponTypes.h"

/* ── CS2 Econ System ────────────────────────────────────────────────────────
 *
 * Port of CEconItemAttributeManager from the reference.
 * Manages econ item attributes (paint kit, pattern, wear) on weapon items.
 * ──────────────────────────────────────────────────────────────────────────── */

struct CPtrGameVector {
    u64 size{};
    u64 ptr{};
};

struct CEconItemAttribute {
    u64     vtable{};          // 0x00
    u64     owner{};           // 0x08
    u8      pad_0010[32]{};   // 0x10
    u16     defIndex{};        // 0x30
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

    void Create(u64 item, const SkinInfo_t& skin) noexcept {
        std::vector<CEconItemAttribute> attrs;

        if (skin.Paint) {
            attrs.push_back(MakeAttribute(Attr_Paint,   static_cast<f32>(skin.Paint)));
            attrs.push_back(MakeAttribute(Attr_Pattern, 0.0f));
            attrs.push_back(MakeAttribute(Attr_Wear,    0.01f));
        }

        CPtrGameVector existing = mem_.Read<CPtrGameVector>(
            item + Offsets::m_AttributeList + Offsets::m_Attributes);

        if (attrs.empty() || existing.size || existing.ptr)
            return;

        u64 block = mem_.Allocate();
        if (!block) return;

        for (usize i = 0; i < attrs.size(); ++i) {
            mem_.Write(block + i * sizeof(CEconItemAttribute), attrs[i]);
        }

        CPtrGameVector vec;
        vec.size = attrs.size();
        vec.ptr  = block;
        mem_.Write(item + Offsets::m_AttributeList + Offsets::m_Attributes, vec);
    }

    void Remove(u64 item) noexcept {
        u64 attr_addr = item + Offsets::m_AttributeList + Offsets::m_Attributes;
        CPtrGameVector attrs = mem_.Read<CPtrGameVector>(attr_addr);
        if (!attrs.size || !attrs.ptr) return;

        mem_.Write(attr_addr, CPtrGameVector{});
        mem_.Free(attrs.ptr);
    }

private:
    ProcessMemory& mem_;

    CEconItemAttribute MakeAttribute(u16 def, f32 value) noexcept {
        CEconItemAttribute attr{};
        attr.defIndex  = def;
        attr.initValue = value;
        attr.value     = value;
        return attr;
    }
};
