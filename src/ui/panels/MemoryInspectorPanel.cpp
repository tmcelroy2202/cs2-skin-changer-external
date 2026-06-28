#include "MemoryInspectorPanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>
#include <cctype>

void MemoryInspectorPanel::Draw() noexcept {
    DrawControls();
    ImGui::Separator();
    DrawHexView();
    ImGui::Separator();
    DrawTypeInterpretation();
}

void MemoryInspectorPanel::DrawControls() noexcept {
    ImGui::InputText("Address", addr_input_, sizeof(addr_input_));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        target_addr_ = std::strtoull(addr_input_, nullptr, 16);
    }

    ImGui::SameLine();
    if (ImGui::Button("Go")) {
        target_addr_ = std::strtoull(addr_input_, nullptr, 16);
    }

    ImGui::InputText("Region base", region_base_input_, sizeof(region_base_input_));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        region_base_ = std::strtoull(region_base_input_, nullptr, 16);
    }

    ImGui::SameLine();
    ImGui::InputText("Size (hex)", region_size_input_, sizeof(region_size_input_));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        region_size_ = std::strtoull(region_size_input_, nullptr, 16);
        if (region_size_ > 0x100000) region_size_ = 0x100000; // cap at 1MB
    }

    ImGui::SameLine();
    ImGui::Checkbox("Follow", &follow_changes_);
}

void MemoryInspectorPanel::DrawHexView() noexcept {
    if (!target_addr_) {
        ImGui::Text("Enter an address to inspect");
        return;
    }

    u64 base = target_addr_ & ~0xF;  // align to 16 bytes
    usize rows = static_cast<usize>(region_size_) / bytes_per_row_;

    ImGui::BeginChild("##hex_scroll", ImVec2(0, 300), true);

    if (ImGui::BeginTable("##hex", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
                          ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, 200);
        ImGui::TableHeadersRow();

        for (usize row = 0; row < rows; ++row) {
            u64 row_addr = base + row * bytes_per_row_;

            // Check readability
            if (!row_addr || row_addr < 0x10000) continue;

            auto* data = reinterpret_cast<const u8*>(row_addr);

            ImGui::TableNextRow();

            // Address column
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%016llX", row_addr);

            // Hex column
            ImGui::TableSetColumnIndex(1);
            for (i32 i = 0; i < bytes_per_row_; ++i) {
                if (i == bytes_per_row_ / 2) ImGui::SameLine(0, 8);
                else if (i > 0) ImGui::SameLine(0, 2);

                bool is_target = (row_addr + i == target_addr_);
                if (is_target) {
                    ImGui::TextColored(ImVec4(1,1,0,1), "%02X", data[i]);
                } else {
                    ImGui::Text("%02X", data[i]);
                }
            }

            // ASCII column
            ImGui::TableSetColumnIndex(2);
            char ascii_buf[256]{};
            for (i32 i = 0; i < bytes_per_row_; ++i) {
                ascii_buf[i] = std::isprint(data[i]) ? static_cast<char>(data[i]) : '.';
            }
            ImGui::Text("%s", ascii_buf);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void MemoryInspectorPanel::DrawTypeInterpretation() noexcept {
    if (!target_addr_) return;

    auto* ptr = reinterpret_cast<const void*>(target_addr_);

    ImGui::Text("Interpretation at 0x%llX:", target_addr_);
    ImGui::Separator();

    ImGui::Text("u8:   %hhu",    *static_cast<const u8*>(ptr));
    ImGui::Text("i8:   %hhd",    *static_cast<const i8*>(ptr));
    ImGui::Text("u16:  %hu",     *static_cast<const u16*>(ptr));
    ImGui::Text("i16:  %hd",     *static_cast<const i16*>(ptr));
    ImGui::Text("u32:  %u",      *static_cast<const u32*>(ptr));
    ImGui::Text("i32:  %d",      *static_cast<const i32*>(ptr));
    ImGui::Text("u64:  0x%llX",  *static_cast<const u64*>(ptr));
    ImGui::Text("i64:  %lld",    *static_cast<const i64*>(ptr));
    ImGui::Text("f32:  %f",      *static_cast<const f32*>(ptr));
    ImGui::Text("f64:  %lf",     *static_cast<const f64*>(ptr));

    // Null-terminated string
    auto str = memory::ReadString(target_addr_, 64);
    if (!str.empty()) {
        ImGui::Text("str:  \"%s\"", str.c_str());
    }
}
