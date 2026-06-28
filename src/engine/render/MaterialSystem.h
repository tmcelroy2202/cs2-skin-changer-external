#pragma once

#include "core/Core.h"
#include "engine/resource/ResourceTypes.h"

/* ── MaterialSystem ─────────────────────────────────────────────────────────
 *
 * Lightweight wrapper over the material system for enumerating and
 * inspecting loaded materials, shaders, and textures at runtime.
 *
 * Used by the ResourceViewer and RenderDebug UI panels to present
 * material properties and render state diagnostics.
 * ──────────────────────────────────────────────────────────────────────────── */

class MaterialSystem {
public:
    bool Init() noexcept;
    explicit operator bool() const noexcept { return initialised_; }

    /// Enumerate all materials currently loaded.
    [[nodiscard]] std::vector<MaterialInfo> GetAllMaterials() const noexcept;

    /// Find a material by name.
    [[nodiscard]] std::optional<MaterialInfo> FindMaterial(
        std::string_view name) const noexcept;

    /// Get the address of a material by name (for inspection).
    [[nodiscard]] u64 GetMaterialPtr(std::string_view name) const noexcept;

private:
    bool initialised_{};
    u64  material_system_addr_{};
};
