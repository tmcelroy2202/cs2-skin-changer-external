#pragma once

#include "core/Core.h"

/* ── Resource Types ─────────────────────────────────────────────────────────
 *
 * Source 2 resource system types.  Resources are identified by resource
 * handles (CResourceHandle / CStrongHandle) and loaded through the
 * resource system.
 *
 * Resource types include:
 *   - CMaterial (materials)
 *   - CTexture (textures)
 *   - CModel (model files)
 *   - CSound (audio)
 *   - CParticleSystem (particle definitions)
 *   - CWorld (map geometry)
 *   - etc.
 *
 * Each resource is reference-counted and managed by the resource system.
 * The resource system tracks loading state, dependencies, and memory usage.
 * ──────────────────────────────────────────────────────────────────────────── */

/// Resource binding flags.
enum class ResourceBindingFlags : u32 {
    None            = 0,
    NoCreate        = 1 << 0,
    NoCache         = 1 << 1,
    NoReference     = 1 << 2,
    ForceLoad       = 1 << 3,
    Async           = 1 << 4,
    Defer           = 1 << 5,
    Invalid         = 1u << 31,
};

/// Resource handle — 64-bit identifier for a resource.
struct CResourceHandle {
    u64 handle{};

    explicit operator bool() const noexcept { return handle != 0; }
};

/// Resource data — pointer to the actual resource object.
struct CResource {
    // +0x00  void** vtable
    // +0x08  char* name
    // +0x10  u64   size
    // +0x18  u32   reference_count
    // +0x1C  u8    loading_state
    // +0x20  ...
};

/// Resource system — global manager for all resources.
struct CResourceSystem {
    // +0x00  void** vtable
    // +0x08  ... resource tracking data
};

/// Material — a render material resource.
struct CMaterial {
    // +0x00  void** vtable
    // +0x08  char* name
    // +0x10  char* shader_name
    // +0x18  ...
};

/// Texture info for display.
struct TextureInfo {
    std::string name;
    u32         width{};
    u32         height{};
    u32         format{};      // DXGI_FORMAT
    u64         memory_size{};
    bool        loaded{};
};

/// Material info for display.
struct MaterialInfo {
    std::string name;
    std::string shader_name;
    std::string group_name;
    u64         memory_size{};
    bool        loaded{};
    bool        translucent{};
};

/// Resource load state.
enum class ResourceState : u8 {
    None        = 0,
    Pending     = 1,
    Loading     = 2,
    Loaded      = 3,
    Unloaded    = 4,
    Error       = 5,
};

inline constexpr std::string_view ResourceStateName(ResourceState s) {
    switch (s) {
    case ResourceState::None:    return "None";
    case ResourceState::Pending: return "Pending";
    case ResourceState::Loading: return "Loading";
    case ResourceState::Loaded:  return "Loaded";
    case ResourceState::Unloaded: return "Unloaded";
    case ResourceState::Error:   return "Error";
    }
    return "Unknown";
}
