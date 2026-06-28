#include "PropertyGridWidget.h"
#include "imgui.h"

namespace {

void DrawPropertyRow(Property& prop, bool show_type_hints) noexcept {
    ImGui::TableNextRow();

    // Name column
    ImGui::TableSetColumnIndex(0);
    if (!prop.tooltip.empty()) {
        ImGui::TextDisabled("?");
        ImGui::SameLine();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", prop.tooltip.c_str());
        }
    }
    ImGui::Text("%s", prop.name.c_str());

    // Type hint column
    if (show_type_hints && !prop.type_hint.empty()) {
        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("%s", prop.type_hint.c_str());
    }

    // Value column
    int value_col = show_type_hints ? 2 : 1;
    ImGui::TableSetColumnIndex(value_col);

    if (prop.on_click) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::Text("%s", prop.value.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemClicked()) {
            prop.on_click();
        }
    } else {
        if (prop.read_only) {
            ImGui::TextWrapped("%s", prop.value.c_str());
        } else {
            // Editable field (future extension)
            ImGui::Text("%s", prop.value.c_str());
        }
    }
}

} // anonymous namespace

bool ui::DrawPropertyGrid(std::string_view id,
                          std::vector<Property>& properties,
                          bool show_type_hints) noexcept {
    int num_cols = show_type_hints ? 3 : 2;

    bool modified = false;

    if (ImGui::BeginTable(id.data(), num_cols,
                          ImGuiTableFlags_Borders |
                          ImGuiTableFlags_Resizable |
                          ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Property");
        if (show_type_hints)
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (auto& prop : properties) {
            DrawPropertyRow(prop, show_type_hints);
        }

        ImGui::EndTable();
    }

    return modified;
}

void ui::DrawPropertyGrid(std::string_view id,
                          std::initializer_list<Property> properties) noexcept {
    // Mutable copy for the non-const overload
    std::vector<Property> props(properties);
    DrawPropertyGrid(id, props, true);
}
