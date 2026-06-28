#pragma once

#include "engine/Engine.h"
#include "EntityTypes.h"

/* ── EntitySystem ───────────────────────────────────────────────────────────
 *
 * Wrapper around CEntitySystem for enumerating and inspecting live entities.
 * Provides safe access to the global entity list with type information
 * resolved through the schema system.
 *
 * Typical use:
 *   EntitySystem ents;
 *   if (ents.Init()) {
 *       for (auto& e : ents.GetAllEntities()) {
 *           LOG_INFO("Entity {}: {}", e.index, e.name);
 *       }
 *   }
 * ──────────────────────────────────────────────────────────────────────────── */

struct EntityInfo {
    u64                     identity_address{};
    u64                     instance_address{};
    u32                     index{};
    u32                     serial{};
    u32                     flags{};
    std::string             name;
    std::string             class_name;
    bool                    active{};
};

class EntitySystem {
public:
    /// Locate the entity system in process memory.
    bool Init() noexcept;

    explicit operator bool() const noexcept { return initialised_; }

    /// Enumerate all entities currently in the entity list.
    [[nodiscard]] std::vector<EntityInfo> GetAllEntities() const noexcept;

    /// Find a specific entity by index.
    [[nodiscard]] std::optional<EntityInfo> GetEntity(u32 index) const noexcept;

    /// Filter entities by class name substring.
    [[nodiscard]] std::vector<EntityInfo> FindEntities(
        std::string_view class_substr) const noexcept;

    /// Get the total number of tracked entity slots.
    [[nodiscard]] u32 GetEntityCount() const noexcept;

    /// Get the raw CEntityIdentity pointer for index.
    [[nodiscard]] u64 GetIdentity(u32 index) const noexcept;

private:
    bool    initialised_{};
    u64     entity_system_addr_{};

    // Resolved offsets (populated by Init, defaults for initial search)
    struct {
        u64 identity_list     = 0x18;   // CEntitySystem -> identity array
        u64 identity_count    = 0x20;   // CEntitySystem -> count
        u64 identity_size     = 0x28;   // CEntitySystem -> identity size (stride)
        u64 inst_to_identity  = 0x08;   // CEntityInstance -> CEntityIdentity
        u64 identity_instance = 0x08;   // CEntityIdentity -> CEntityInstance
        u64 identity_index    = 0x20;   // CEntityIdentity -> index
        u64 identity_serial   = 0x24;   // CEntityIdentity -> serial
        u64 identity_flags    = 0x28;   // CEntityIdentity -> flags
        u64 identity_name     = 0x30;   // CEntityIdentity -> name pointer
    } offsets_{};

    EntityInfo ReadIdentity(u64 addr) const noexcept;
};
