#include "HexViewWidget.h"
#include "imgui.h"
#include <cctype>

void ui::DrawHexView(const HexViewWidgetConfig& cfg) noexcept {
    if (!cfg.base_address) {
        ImGui::Text("(null address)");
        return;
    }

    auto* data = reinterpret_cast<const u8*>(cfg.base_address);
    usize rows = cfg.data_size / cfg.bytes_per_row;

    ImGui::BeginChild(cfg.id, ImVec2(0, 0), true);

    for (usize row = 0; row < rows; ++row) {
        u64 row_addr = cfg.base_address + row * cfg.bytes_per_row;

        // Address
        if (cfg.show_address) {
            ImGui::Text("%016llX  ", row_addr);
            ImGui::SameLine(0, 0);
        }

        // Hex bytes
        for (u8 i = 0; i < cfg.bytes_per_row; ++i) {
            usize idx = row * cfg.bytes_per_row + i;
            if (idx >= cfg.data_size) break;

            if (i == cfg.bytes_per_row / 2)
                ImGui::SameLine(0, 4);
            else if (i > 0)
                ImGui::SameLine(0, 0);

            u64 byte_addr = row_addr + i;
            bool highlight = (byte_addr == cfg.highlight_byte);

            if (highlight)
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%02X", data[idx]);
            else
                ImGui::Text("%02X", data[idx]);

            if (i < cfg.bytes_per_row - 1)
                ImGui::SameLine(0, 2);
        }

        // ASCII
        if (cfg.show_ascii) {
            ImGui::SameLine(0, 8);
            char ascii[17]{};
            for (u8 i = 0; i < cfg.bytes_per_row; ++i) {
                usize idx = row * cfg.bytes_per_row + i;
                if (idx >= cfg.data_size) break;
                ascii[i] = std::isprint(data[idx]) ? static_cast<char>(data[idx]) : '.';
            }
            ImGui::Text("|%s|", ascii);
        }
    }

    ImGui::EndChild();
}
