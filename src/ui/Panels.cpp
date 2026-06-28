#include "Panels.h"
#include "imgui.h"

#include <algorithm>

void PanelManager::Register(Panel* panel) noexcept {
    if (!panel || Find(panel->GetId().data())) return;
    panels_.push_back(panel);
}

void PanelManager::Unregister(Panel* panel) noexcept {
    auto it = std::ranges::find(panels_, panel);
    if (it != panels_.end())
        panels_.erase(it);
}

void PanelManager::DrawAll() noexcept {
    for (auto* panel : panels_) {
        if (!panel || !panel->IsOpen()) continue;
        if (!panel->IsVisible()) continue;

        if (ImGui::Begin(panel->GetTitle().data(), panel->OpenPtr(),
                         ImGuiWindowFlags_None)) {
            panel->Draw();
        }
        ImGui::End();
    }
}

Panel* PanelManager::Find(std::string_view id) const noexcept {
    for (auto* p : panels_) {
        if (p->GetId() == id) return p;
    }
    return nullptr;
}

void PanelManager::SetAllVisible(bool visible) noexcept {
    for (auto* p : panels_) {
        p->SetVisible(visible);
    }
}
