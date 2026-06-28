#pragma once

#include "core/Core.h"

/* ── Entity Types ───────────────────────────────────────────────────────────
 *
 * Core Source 2 entity system data structures.
 *
 * Entity identity hierarchy:
 *   CEntityIdentity  ── base identity; every entity has one
 *     ├── index, serial, flags, name
 *     ├── linked list pointers (prev/next)
 *     └── pointer to CEntityInstance
 *
 *   CEntityInstance  ── base class for all entity instances
 *     ├── vtable, identity pointer
 *     └── custom data
 *
 *   CBaseEntity      ── game-level base; inherits from CEntityInstance
 *
 * Entity System:
 *   CEntitySystem manages the global entity list.
 *   Contains arrays/lists of CEntityIdentity entries.
 *
 * These structures use offsets determined at runtime rather than hardcoded
 * values, as they vary between Source 2 games and updates.
 * ──────────────────────────────────────────────────────────────────────────── */

/// Entity identity flags.
enum class EntityFlags : u32 {
    None                = 0,
    Active              = 1 << 0,
    CreatedThisFrame    = 1 << 3,
    ShouldBeInList      = 1 << 6,
    NeedsInitialisation = 1 << 7,
    MarkedForDeletion   = 1 << 8,
    IsScriptInstance    = 1 << 16,
};

/// Entity identity — the core tracking struct for every entity.
struct CEntityIdentity {
    // Offsets are dynamic; typical layout:
    // +0x00  vtable
    // +0x08  CEntityInstance*  (the actual entity object)
    // +0x10  CEntityIdentity*  (prev in list)
    // +0x18  CEntityIdentity*  (next in list)
    // +0x20  u32 index
    // +0x24  u32 serial
    // +0x28  u32 flags
    // +0x30  char* name
    // +0x38  ...
};

/// Entity instance — every entity inherits from this.
struct CEntityInstance {
    // +0x00  void** vtable
    // +0x08  CEntityIdentity* identity
    // +0x10  ... class-specific data
};

/// Entity system — manages all entities in a world.
struct CEntitySystem {
    // +0x00  void** vtable
    // +0x08  ... internal state
    // Entity list is typically at +0x18 or +0x20
    // and is an array of CEntityIdentity* pointers.
};

// ── Helper ────────────────────────────────────────────────────────────────────

inline constexpr std::string_view EntityFlagsToString(EntityFlags f) {
    switch (f) {
    case EntityFlags::None:                return "None";
    case EntityFlags::Active:              return "Active";
    case EntityFlags::CreatedThisFrame:    return "CreatedThisFrame";
    case EntityFlags::ShouldBeInList:      return "ShouldBeInList";
    case EntityFlags::NeedsInitialisation: return "NeedsInit";
    case EntityFlags::MarkedForDeletion:   return "MarkedForDeletion";
    case EntityFlags::IsScriptInstance:    return "ScriptInstance";
    default:                               return "Unknown";
    }
}

inline std::string FormatEntityFlags(u32 flags) {
    std::string result;
    if (flags & static_cast<u32>(EntityFlags::Active))              result += "Active ";
    if (flags & static_cast<u32>(EntityFlags::CreatedThisFrame))    result += "New ";
    if (flags & static_cast<u32>(EntityFlags::ShouldBeInList))      result += "InList ";
    if (flags & static_cast<u32>(EntityFlags::NeedsInitialisation)) result += "NeedsInit ";
    if (flags & static_cast<u32>(EntityFlags::MarkedForDeletion))   result += "PendingDel ";
    if (flags & static_cast<u32>(EntityFlags::IsScriptInstance))    result += "Script ";
    if (result.empty()) result = "None";
    return result;
}
