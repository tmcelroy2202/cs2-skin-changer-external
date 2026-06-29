#include "EntitySystem.h"
#include "engine/schema/SchemaTypes.h"
#include "engine/schema/SchemaReflection.h"

// ── Init ──────────────────────────────────────────────────────────────────────
bool EntitySystem::Init() noexcept {
    // The entity system is typically accessed via global pointer or interface.
    // Try interface discovery first:
    auto mod = Module::Get("server.dll");
    if (!mod) mod = Module::Get("server.so");
    if (!mod) mod = Module::Get("engine2.dll");

    if (!mod) {
        LOG_ERROR("No suitable module found for entity system");
        return false;
    }

    auto* iface = InterfaceDiscovery::Get<void>(mod,
        "Source2EngineToEntitySystemV001");
    if (iface) {
        entity_system_addr_ = reinterpret_cast<u64>(iface);
        LOG_INFO("EntitySystem via interface at 0x{:X}", entity_system_addr_);
    }

    // Fallback: scan for a known global pointer pattern
    if (!entity_system_addr_) {
        // Pattern: "48 8B 05 ? ? ? ? 48 8B 40 ? 48 85 C0"
        auto found = PatternScanner::FindString(
            mod, "48 8B 05 ? ? ? ? 48 8B 40 ? 48 85 C0");
        if (found) {
            i32 disp = memory::Read<i32>(*found + 3);
            u64 global = *found + 7 + static_cast<u64>(disp);
            entity_system_addr_ = memory::Read<u64>(global);
            LOG_INFO("EntitySystem via scan at 0x{:X} (global 0x{:X})",
                     entity_system_addr_, global);
        }
    }

    if (!entity_system_addr_) {
        LOG_ERROR("Failed to locate CEntitySystem");
        return false;
    }

    initialised_ = true;
    return true;
}

// ── GetEntityCount ────────────────────────────────────────────────────────────
u32 EntitySystem::GetEntityCount() const noexcept {
    if (!initialised_) return 0;
    return memory::Read<u32>(entity_system_addr_ + offsets_.identity_count);
}

// ── GetIdentity ───────────────────────────────────────────────────────────────
u64 EntitySystem::GetIdentity(u32 index) const noexcept {
    if (!initialised_) return 0;
    u64 list_base = memory::Read<u64>(entity_system_addr_ + offsets_.identity_list);
    if (!list_base) return 0;

    // The list is typically a flat array of CEntityIdentity objects
    u32 stride = memory::Read<u32>(entity_system_addr_ + offsets_.identity_size);
    if (stride == 0) stride = 0x40; // fallback default stride

    return list_base + static_cast<u64>(index) * stride;
}

// ── ReadIdentity ──────────────────────────────────────────────────────────────
EntityInfo EntitySystem::ReadIdentity(u64 addr) const noexcept {
    EntityInfo info;
    if (!addr) return info;

    info.identity_address = addr;
    info.index     = memory::Read<u32>(addr + offsets_.identity_index);
    info.serial    = memory::Read<u32>(addr + offsets_.identity_serial);
    info.flags     = memory::Read<u32>(addr + offsets_.identity_flags);
    info.name      = memory::ReadString(memory::Read<u64>(addr + offsets_.identity_name));
    info.instance_address = memory::Read<u64>(addr + offsets_.identity_instance);
    info.active    = (info.flags & static_cast<u32>(EntityFlags::Active)) != 0;

    // Try to read class name from the entity instance's schema info
    if (info.instance_address) {
        u64 vtable = memory::Read<u64>(info.instance_address);
        // Many Source 2 vtables have a schema class info pointer at a known offset
        // This is version-dependent; we attempt common offsets
        for (i32 off : { -0x10, -0x08, 0x00, 0x08 }) {
            u64 possible = memory::Read<u64>(vtable + off);
            if (possible) {
                auto* ci = reinterpret_cast<SchemaClassInfo*>(possible);
                if (ci->name && ci->field_count < 500) { // sanity check
                    info.class_name = ci->name;
                    break;
                }
            }
        }
    }

    return info;
}

// ── GetAllEntities ────────────────────────────────────────────────────────────
std::vector<EntityInfo> EntitySystem::GetAllEntities() const noexcept {
    std::vector<EntityInfo> result;
    if (!initialised_) return result;

    u32 count = GetEntityCount();
    result.reserve(count);

    for (u32 i = 0; i < count; ++i) {
        u64 addr = GetIdentity(i);
        if (!addr) continue;
        auto info = ReadIdentity(addr);
        // Only include active entities that have been allocated
        if (info.serial != 0xFFFFFFFF && info.index == i) {
            result.push_back(std::move(info));
        }
    }

    return result;
}

// ── GetEntity ─────────────────────────────────────────────────────────────────
std::optional<EntityInfo> EntitySystem::GetEntity(u32 index) const noexcept {
    u64 addr = GetIdentity(index);
    if (!addr) return std::nullopt;
    auto info = ReadIdentity(addr);
    if (info.serial == 0xFFFFFFFF) return std::nullopt;
    return info;
}

// ── FindEntities ──────────────────────────────────────────────────────────────
std::vector<EntityInfo>
EntitySystem::FindEntities(std::string_view class_substr) const noexcept {
    std::vector<EntityInfo> result;
    for (auto& e : GetAllEntities()) {
        if (e.class_name.find(class_substr) != std::string::npos) {
            result.push_back(e);
        }
    }
    return result;
}
