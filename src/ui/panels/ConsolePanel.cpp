#include "ConsolePanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>
#include <algorithm>
#include <cctype>

void ConsolePanel::AddMessage(std::string_view text, LogLevel level) noexcept {
    {
        std::lock_guard lock(messages_mutex_);
        messages_.push_back(LogMessage{
            .text = std::string(text),
            .level = level,
            .timestamp = static_cast<f32>(ImGui::GetTime())
        });
        if (messages_.size() > max_messages_) {
            messages_.pop_front();
        }
        scroll_to_bottom_ = auto_scroll_;
    }
}

void ConsolePanel::Clear() noexcept {
    std::lock_guard lock(messages_mutex_);
    messages_.clear();
}

ImVec4 ConsolePanel::ColorForLevel(LogLevel lv) noexcept {
    switch (lv) {
    case LogLevel::Trace: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // grey
    case LogLevel::Debug: return ImVec4(0.6f, 0.6f, 0.8f, 1.0f); // blue-grey
    case LogLevel::Info:  return ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // white
    case LogLevel::Warn:  return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // yellow
    case LogLevel::Error: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // red
    case LogLevel::Fatal: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // bright red
    }
    return ImVec4(1,1,1,1);
}

void ConsolePanel::Draw() noexcept {
    // Toolbar
    if (ImGui::Button("Clear")) { Clear(); }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);
    ImGui::SameLine();
    ImGui::InputText("Filter", &filter_);

    ImGui::Separator();

    ImGui::BeginChild("##console_output", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard lock(messages_mutex_);
        for (auto& msg : messages_) {
            if (!filter_.empty()) {
                auto text_lower = msg.text;
                std::ranges::transform(text_lower, text_lower.begin(),
                                       [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); });
                auto filter_lower = filter_;
                std::ranges::transform(filter_lower, filter_lower.begin(),
                                       [](char c) { return char(std::tolower(static_cast<unsigned char>(c))); });
                if (text_lower.find(filter_lower) == std::string::npos)
                    continue;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ColorForLevel(msg.level));
            ImGui::Text("[%.1f] %s", msg.timestamp, msg.text.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (scroll_to_bottom_) {
        ImGui::SetScrollHereY(1.0f);
        scroll_to_bottom_ = false;
    }

    ImGui::EndChild();
}
