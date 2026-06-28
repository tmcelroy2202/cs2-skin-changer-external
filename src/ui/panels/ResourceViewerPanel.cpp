#include "ResourceViewerPanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>

void ResourceViewerPanel::Draw() noexcept {
    // Tabs
    if (ImGui::BeginTabBar("##resource_tabs")) {
        if (ImGui::BeginTabItem("Materials")) {
            active_tab_ = Tab::Materials;
            DrawMaterialTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Textures")) {
            active_tab_ = Tab::Textures;
            DrawTextureTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stats")) {
            active_tab_ = Tab::Stats;
            DrawStatsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void ResourceViewerPanel::DrawMaterialTab() noexcept {
    if (!materials_ || !*materials_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Material system not initialised");
        return;
    }

    ImGui::InputText("Search materials", &filter_);
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {}

    auto materials = materials_->GetAllMaterials();

    if (ImGui::BeginTable("##materials", 5, ImGuiTableFlags_Borders |
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Shader", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Translucent", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableHeadersRow();

        for (auto& m : materials) {
            if (!filter_.empty()) {
                auto n = m.name;
                std::ranges::transform(n, n.begin(), [](char c) { return char(std::tolower(c)); });
                auto f = filter_;
                std::ranges::transform(f, f.begin(), [](char c) { return char(std::tolower(c)); });
                if (n.find(f) == std::string_view::npos) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", m.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", m.shader_name.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", m.group_name.c_str());

            ImGui::TableSetColumnIndex(3);
            if (m.memory_size > 1024 * 1024)
                ImGui::Text("%.1f MB", m.memory_size / (1024.0 * 1024.0));
            else if (m.memory_size > 1024)
                ImGui::Text("%.1f KB", m.memory_size / 1024.0);
            else
                ImGui::Text("%llu B", m.memory_size);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", m.translucent ? "Yes" : "No");
        }

        ImGui::EndTable();
    }
}

void ResourceViewerPanel::DrawTextureTab() noexcept {
    if (!resources_ || !*resources_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Resource system not initialised");
        return;
    }

    ImGui::InputText("Search textures", &filter_);

    auto textures = resources_->GetTextures(filter_);

    if (ImGui::BeginTable("##textures", 6, ImGuiTableFlags_Borders |
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Width", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Height", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Loaded", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableHeadersRow();

        for (auto& t : textures) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", t.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", t.width);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", t.height);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%X", t.format);

            ImGui::TableSetColumnIndex(4);
            if (t.memory_size > 1024 * 1024)
                ImGui::Text("%.1f MB", t.memory_size / (1024.0 * 1024.0));
            else if (t.memory_size > 1024)
                ImGui::Text("%.1f KB", t.memory_size / 1024.0);
            else
                ImGui::Text("%llu B", t.memory_size);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", t.loaded ? "Yes" : "No");
        }

        ImGui::EndTable();
    }
}

void ResourceViewerPanel::DrawStatsTab() noexcept {
    if (!resources_ || !*resources_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Resource system not initialised");
        return;
    }

    ImGui::Text("Resource System Statistics");
    ImGui::Separator();

    ImGui::Text("Total resources:     %u", resources_->GetResourceCount());
    ImGui::Text("Pending loads:       %u", resources_->GetPendingLoadCount());
    ImGui::Text("GPU memory usage:    %.2f MB",
                resources_->GetTotalMemoryUsage() / (1024.0 * 1024.0));

    ImGui::Separator();

    if (materials_ && *materials_) {
        auto mats = materials_->GetAllMaterials();
        u32 translucent_count = 0;
        u64 total_mat_memory = 0;
        for (auto& m : mats) {
            if (m.translucent) ++translucent_count;
            total_mat_memory += m.memory_size;
        }

        ImGui::Text("Materials:           %zu", mats.size());
        ImGui::Text("  Translucent:       %u", translucent_count);
        ImGui::Text("  Opaque:            %zu", mats.size() - translucent_count);
        ImGui::Text("  Material memory:   %.2f MB",
                    total_mat_memory / (1024.0 * 1024.0));
    }
}
