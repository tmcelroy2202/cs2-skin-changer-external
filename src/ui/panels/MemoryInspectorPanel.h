#pragma once

#include "ui/Panels.h"

/* ── MemoryInspectorPanel ──────────────────────────────────────────────────
 *
 * Real-time memory viewer for inspecting arbitrary engine structures.
 *
 * Features:
 *   - Hex dump view with ASCII sidebar
 *   - Go-to address
 *   - Follow pointer chains
 *   - Interpret memory as different types (int, float, string, etc.)
 *   - Scan for patterns within a memory region
 *   - Search for references to a given address
 * ──────────────────────────────────────────────────────────────────────────── */

class MemoryInspectorPanel final : public Panel {
public:
    MemoryInspectorPanel() noexcept : Panel("Memory Inspector") {}

    std::string_view GetId() const noexcept override { return "MemoryInspector"; }

    void Draw() noexcept override;

    /// Programmatically navigate to an address.
    void NavigateTo(u64 address) noexcept { target_addr_ = address; }

private:
    u64  target_addr_{};
    u64  region_base_{};
    u64  region_size_{0x1000};
    bool follow_changes_{};

    // UI state
    char addr_input_[32] = "0";
    char region_base_input_[32] = "0";
    char region_size_input_[32] = "1000";
    i32  bytes_per_row_ = 16;

    void DrawHexView() noexcept;
    void DrawControls() noexcept;
    void DrawTypeInterpretation() noexcept;
};
