#pragma once

#include "ui/Panels.h"
#include "engine/resource/ResourceSystem.h"
#include "engine/render/MaterialSystem.h"

/* ── ResourceViewerPanel ───────────────────────────────────────────────────
 *
 * Displays loaded game resources — materials, textures, models — with
 * metadata including memory usage, load state, and shader information.
 *
 * Features:
 *   - Tab-based view (Materials / Textures / Resources)
 *   - Per-resource memory size and format information
 *   - Load state tracking (pending / loading / loaded / error)
 *   - Search/filter by name
 *   - Click to inspect resource details
 * ──────────────────────────────────────────────────────────────────────────── */

class ResourceViewerPanel final : public Panel {
public:
    explicit ResourceViewerPanel(ResourceSystem* resources,
                                  MaterialSystem* materials) noexcept
        : Panel("Resource Viewer")
        , resources_(resources)
        , materials_(materials) {}

    std::string_view GetId() const noexcept override { return "ResourceViewer"; }

    void Draw() noexcept override;

private:
    ResourceSystem* resources_;
    MaterialSystem* materials_;

    enum class Tab { Materials, Textures, Stats };
    Tab active_tab_ = Tab::Materials;

    std::string filter_;
    bool        auto_refresh_{true};

    void DrawMaterialTab() noexcept;
    void DrawTextureTab() noexcept;
    void DrawStatsTab() noexcept;
};
