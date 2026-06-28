#pragma once

#include "ui/Panels.h"
#include "engine/schema/SchemaSystem.h"
#include "engine/schema/SchemaReflection.h"

/* ── SchemaBrowserPanel ─────────────────────────────────────────────────────
 *
 * Interactive schema class browser.  Displays all registered schema types
 * organised by module scope, with search/filter and field-level detail.
 *
 * Features:
 *   - Tree view of all type scopes (client, server, engine, etc.)
 *   - Searchable class list with substring matching
 *   - Class detail view showing all fields (inherited + declared)
 *   - Field type, offset, and formatted current value display
 *   - Click-to-copy offset values
 * ──────────────────────────────────────────────────────────────────────────── */

class SchemaBrowserPanel final : public Panel {
public:
    explicit SchemaBrowserPanel(SchemaSystem* schema) noexcept
        : Panel("Schema Browser"), schema_(schema), reflection_(schema) {}

    std::string_view GetId() const noexcept override { return "SchemaBrowser"; }

    void Draw() noexcept override;

private:
    SchemaSystem*      schema_;
    SchemaReflection   reflection_;

    // UI state
    std::string        search_filter_;
    SchemaClassInfo*   selected_class_{};
    SchemaSystemTypeScope* selected_scope_{};
    bool               show_internal_{};

    void DrawScopeTree() noexcept;
    void DrawClassList() noexcept;
    void DrawClassDetail() noexcept;
    void DrawFieldRow(const FieldValue& fv) noexcept;
};
