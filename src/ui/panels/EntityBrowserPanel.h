#pragma once

#include "ui/Panels.h"
#include "engine/entity/EntitySystem.h"

/* ── EntityBrowserPanel ─────────────────────────────────────────────────────
 *
 * Live entity enumeration and browsing interface.
 *
 * Features:
 *   - Table of all active entities with index, name, class, address
 *   - Search/filter by class name or entity name
 *   - Click to inspect entity details
 *   - Detail view showing identity fields and schema fields
 *   - Auto-refresh toggle with configurable interval
 * ──────────────────────────────────────────────────────────────────────────── */

class EntityBrowserPanel final : public Panel {
public:
    explicit EntityBrowserPanel(EntitySystem* entities,
                                SchemaSystem* schema) noexcept
        : Panel("Entity Browser"), entities_(entities), schema_(schema) {}

    std::string_view GetId() const noexcept override { return "EntityBrowser"; }

    void Draw() noexcept override;

private:
    EntitySystem* entities_;
    SchemaSystem* schema_;

    // UI state
    std::string  filter_;
    u32          selected_entity_index_{};
    bool         auto_refresh_{true};
    f32          refresh_interval_{0.5f};
    f32          time_since_refresh_{};
    bool         show_inactive_{};

    std::vector<EntityInfo> cached_entities_;

    void DrawEntityList() noexcept;
    void DrawEntityDetail() noexcept;
    void RefreshEntities() noexcept;
};
