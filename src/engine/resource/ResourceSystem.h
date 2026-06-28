#pragma once

#include "engine/Engine.h"
#include "ResourceTypes.h"

/* ── ResourceSystem ─────────────────────────────────────────────────────────
 *
 * Wrapper around CResourceSystem for observing asset loading, resource
 * metadata, and material/texture information.
 *
 * This system is read-only and does not modify resource loading behaviour.
 * It provides insight into which assets are currently resident in memory,
 * their load states, and basic metadata.
 * ──────────────────────────────────────────────────────────────────────────── */

class ResourceSystem {
public:
    /// Locate and bind to the resource system.
    bool Init() noexcept;

    explicit operator bool() const noexcept { return initialised_; }

    // ── Resource queries ──────────────────────────────────────────────────────

    /// Get materials matching a name filter.
    [[nodiscard]] std::vector<MaterialInfo> GetMaterials(
        std::string_view filter = "") const noexcept;

    /// Get textures matching a name filter.
    [[nodiscard]] std::vector<TextureInfo> GetTextures(
        std::string_view filter = "") const noexcept;

    /// Get resource load state by handle.
    [[nodiscard]] ResourceState GetResourceState(
        const CResourceHandle& handle) const noexcept;

    /// Check if a resource is currently loaded.
    [[nodiscard]] bool IsResourceLoaded(
        const CResourceHandle& handle) const noexcept;

    // ── Memory / statistics ───────────────────────────────────────────────────

    /// Get total number of tracked resources.
    [[nodiscard]] u32 GetResourceCount() const noexcept;

    /// Get estimated total resource memory usage in bytes.
    [[nodiscard]] u64 GetTotalMemoryUsage() const noexcept;

    /// Get number of currently loading resources.
    [[nodiscard]] u32 GetPendingLoadCount() const noexcept;

private:
    bool initialised_{};
    u64  resource_system_addr_{};

    // Internal helpers for walking resource tables
    u64 GetResourceList() const noexcept;
    u32 GetResourceListCount() const noexcept;

    MaterialInfo ReadMaterialInfo(u64 addr) const noexcept;
    TextureInfo  ReadTextureInfo(u64 addr) const noexcept;
};
