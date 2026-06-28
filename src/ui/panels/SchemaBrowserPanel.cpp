#include "SchemaBrowserPanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>

void SchemaBrowserPanel::Draw() noexcept {
    if (!schema_ || !*schema_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Schema system not initialised");
        return;
    }

    // Three-panel layout
    DrawScopeTree();
    ImGui::SameLine();
    DrawClassList();
    ImGui::SameLine();
    DrawClassDetail();
}

void SchemaBrowserPanel::DrawScopeTree() noexcept {
    ImGui::BeginChild("##schema_scopes", ImVec2(200, 0), true);

    ImGui::Text("Type Scopes");
    ImGui::Separator();

    for (auto* scope : schema_->GetTypeScopes()) {
        if (!scope) continue;

        u64 base = reinterpret_cast<u64>(scope);
        auto name = memory::ReadString(base + 0x50); // scope->module_name
        ImGuiTreeNodeFlags flags = (scope == selected_scope_)
                                       ? ImGuiTreeNodeFlags_Selected
                                       : ImGuiTreeNodeFlags_None;
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        ImGui::TreeNodeEx((void*)(base), flags, "%s", name.c_str());
        if (ImGui::IsItemClicked()) {
            selected_scope_ = scope;
            selected_class_ = nullptr;
        }
    }

    ImGui::EndChild();
}

void SchemaBrowserPanel::DrawClassList() noexcept {
    ImGui::BeginChild("##schema_classes", ImVec2(300, 0), true);

    ImGui::Text("Classes");
    ImGui::Separator();

    // Search filter
    ImGui::InputText("Filter", &search_filter_);
    ImGui::Checkbox("Show internal", &show_internal_);

    ImGui::Separator();

    if (!selected_scope_) {
        ImGui::Text("Select a scope");
        ImGui::EndChild();
        return;
    }

    auto classes = reflection_.SearchClasses(
        search_filter_.empty() ? "" : search_filter_);

    ImGui::Text("%zu classes", classes.size());
    ImGui::Separator();

    ImGui::BeginChild("##class_list_scroll");
    for (auto* ci : classes) {
        if (!ci || !ci->name) continue;

        // Skip internal classes unless enabled
        if (!show_internal_ && ci->name[0] == '_') continue;

        ImGuiTreeNodeFlags flags = (ci == selected_class_)
                                       ? ImGuiTreeNodeFlags_Selected
                                       : ImGuiTreeNodeFlags_None;
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        ImGui::TreeNodeEx((void*)ci, flags, "%s [%u]",
                          ci->name, ci->size);
        if (ImGui::IsItemClicked()) {
            selected_class_ = ci;
        }
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void SchemaBrowserPanel::DrawClassDetail() noexcept {
    ImGui::BeginChild("##schema_detail", ImVec2(0, 0), true);

    if (!selected_class_) {
        ImGui::Text("Select a class to inspect");
        ImGui::EndChild();
        return;
    }

    auto hierarchy = reflection_.InspectClass(selected_class_);

    ImGui::Text("Class: %s", hierarchy.class_name.c_str());
    ImGui::Text("Size:  0x%X (%u)", selected_class_->size, selected_class_->size);
    ImGui::Text("Fields: %zu (total with bases: %u)",
                hierarchy.fields.size(),
                SchemaReflection::GetTotalFieldCount(selected_class_));
    ImGui::Text("Base classes: %u", selected_class_->base_class_count);
    if (selected_class_->module) {
        ImGui::Text("Module: %s", selected_class_->module);
    }
    ImGui::Separator();

    // Fields table
    ImGui::BeginChild("##fields_list");

    if (ImGui::BeginTable("##fields", 5, ImGuiTableFlags_Borders |
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        for (auto& fv : hierarchy.fields) {
            DrawFieldRow(fv);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::EndChild();
}

void SchemaBrowserPanel::DrawFieldRow(const FieldValue& fv) noexcept {
    ImGui::TableNextRow();

    // Offset
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("+0x%04X", fv.offset);

    // Name
    ImGui::TableSetColumnIndex(1);
    if (fv.is_base)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Text("%s", fv.name.c_str());
    if (fv.is_base)
        ImGui::PopStyleColor();

    // Type
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.*s", static_cast<int>(SchemaFieldTypeName(fv.type).size()),
                SchemaFieldTypeName(fv.type).data());

    // Size (approximate)
    ImGui::TableSetColumnIndex(3);
    // Show size based on type
    ImGui::EndRow();
}
