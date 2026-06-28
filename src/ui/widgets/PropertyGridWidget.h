#pragma once

#include "core/Core.h"
#include <any>
#include <string>
#include <vector>
#include <functional>

/* ── PropertyGridWidget ─────────────────────────────────────────────────────
 *
 * Generic key-value property grid for displaying structured data.
 * Each property has a name, type, value string, and optional edit callback.
 *
 * Used by entity detail, schema field view, and resource info panels.
 * ──────────────────────────────────────────────────────────────────────────── */

struct Property {
    std::string             name;
    std::string             value;
    std::string             type_hint;       // e.g. "int", "float", "string"
    std::string             tooltip;         // tooltip on hover
    bool                    read_only{true};
    std::function<void()>   on_click;        // optional action on click

    Property() = default;
    Property(std::string_view name_, std::string_view value_,
             std::string_view type_hint_ = "",
             std::string_view tooltip_ = "")
        : name(name_), value(value_), type_hint(type_hint_), tooltip(tooltip_) {}
};

namespace ui {

/// Draw a property grid from a list of properties.
/// Returns true if any property was modified.
bool DrawPropertyGrid(std::string_view id,
                      std::vector<Property>& properties,
                      bool show_type_hints = true) noexcept;

/// Draw a property grid with a fixed set of read-only properties.
void DrawPropertyGrid(std::string_view id,
                      std::initializer_list<Property> properties) noexcept;

} // namespace ui
