#include "EntityBrowserPanel.h"
#include "engine/schema/SchemaReflection.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>
#include <cctype>

void EntityBrowserPanel::Draw() noexcept {
    if (!entities_ || !*entities_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Entity system not initialised");
        return;
    }

    // Auto-refresh timer
    if (auto_refresh_) {
        time_since_refresh_ += ImGui::GetIO().DeltaTime;
        if (time_since_refresh_ >= refresh_interval_) {
            RefreshEntities();
            time_since_refresh_ = 0.0f;
        }
    }

    DrawEntityList();
    ImGui::SameLine();
    DrawEntityDetail();
}

void EntityBrowserPanel::RefreshEntities() noexcept {
    if (!entities_) return;
    cached_entities_ = entities_->GetAllEntities();
}

void EntityBrowserPanel::DrawEntityList() noexcept {
    ImGui::BeginChild("##entity_list", ImVec2(500, 0), true);

    ImGui::Text("Entities");
    ImGui::Separator();

    // Controls
    ImGui::Checkbox("Auto-refresh", &auto_refresh_);
    ImGui::SameLine();
    if (ImGui::Button("Refresh now")) {
        RefreshEntities();
        time_since_refresh_ = 0.0f;
    }
    ImGui::InputText("Search", &filter_);
    ImGui::Checkbox("Show inactive", &show_inactive_);
    ImGui::Text("%zu entities cached", cached_entities_.size());

    ImGui::Separator();

    ImGui::BeginChild("##entity_scroll");
    if (ImGui::BeginTable("##entities", 5, ImGuiTableFlags_Borders |
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (auto& e : cached_entities_) {
            if (!show_inactive_ && !e.active) continue;

            // Apply name filter
            if (!filter_.empty()) {
                auto name_lower = e.name;
                auto class_lower = e.class_name;
                std::ranges::transform(name_lower, name_lower.begin(),
                                       [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); });
                std::ranges::transform(class_lower, class_lower.begin(),
                                       [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); });
                auto filter_lower = filter_;
                std::ranges::transform(filter_lower, filter_lower.begin(),
                                       [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); });

                if (name_lower.find(filter_lower) == std::string::npos &&
                    class_lower.find(filter_lower) == std::string::npos) {
                    continue;
                }
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", e.index);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(e.index);
            if (ImGui::Selectable(e.name.c_str(),
                                  selected_entity_index_ == e.index,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                selected_entity_index_ = e.index;
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", e.class_name.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", e.active ? "Active" : "Inactive");

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("0x%llX", e.instance_address);
        }

        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void EntityBrowserPanel::DrawEntityDetail() noexcept {
    ImGui::BeginChild("##entity_detail", ImVec2(0, 0), true);

    auto found = std::ranges::find_if(cached_entities_,
        [this](const EntityInfo& e) {
            return e.index == selected_entity_index_;
        });

    if (found == cached_entities_.end()) {
        ImGui::Text("Select an entity to inspect");
        ImGui::EndChild();
        return;
    }

    auto& e = *found;
    ImGui::Text("Entity Detail");
    ImGui::Separator();

    ImGui::Text("Index:     %u", e.index);
    ImGui::Text("Serial:    %u", e.serial);
    ImGui::Text("Name:      %s", e.name.c_str());
    ImGui::Text("Class:     %s", e.class_name.c_str());
    ImGui::Text("Flags:     %s", FormatEntityFlags(e.flags).c_str());
    ImGui::Text("Active:    %s", e.active ? "Yes" : "No");
    ImGui::Text("Identity:  0x%llX", e.identity_address);
    ImGui::Text("Instance:  0x%llX", e.instance_address);

    ImGui::Separator();

    // Show schema fields if available
    if (schema_ && *schema_ && !e.class_name.empty()) {
        auto* ci = schema_->FindClass(e.class_name);
        if (ci) {
            ImGui::Text("Schema Fields (%s):", e.class_name.c_str());
            ImGui::Separator();

            if (ImGui::BeginTable("##field_vals", 3,
                                 ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Offset");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (u16 i = 0; i < ci->field_count; ++i) {
                    auto& f = ci->fields[i];
                    if (!f.name) continue;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("+0x%04X", f.single_offset);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", f.name);

                    ImGui::TableSetColumnIndex(2);
                    auto val = SchemaReflection::FormatFieldValue(
                        f, reinterpret_cast<void*>(e.instance_address));
                    ImGui::Text("%s", val.c_str());
                }

                ImGui::EndTable();
            }
        }
    }

    ImGui::EndChild();
}
