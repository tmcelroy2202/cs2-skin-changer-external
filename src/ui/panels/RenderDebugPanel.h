#pragma once

#include "ui/Panels.h"
#include "engine/render/RenderSystem.h"

/* ── RenderDebugPanel ──────────────────────────────────────────────────────
 *
 * Real-time rendering statistics and diagnostics.
 *
 * Features:
 *   - Frame time graph (CPU + GPU)
 *   - Draw call and triangle count history
 *   - Current render state (render targets, viewport, shaders)
 *   - GPU memory usage
 *   - Resolution information
 * ──────────────────────────────────────────────────────────────────────────── */

class RenderDebugPanel final : public Panel {
public:
    explicit RenderDebugPanel(RenderSystem* render) noexcept
        : Panel("Render Debug"), render_(render) {}

    std::string_view GetId() const noexcept override { return "RenderDebug"; }

    void Draw() noexcept override;

private:
    RenderSystem* render_;

    // History buffers for graphs
    static constexpr usize kHistorySize = 120;
    f32 frame_time_history_[kHistorySize]{};
    f32 draw_call_history_[kHistorySize]{};
    usize history_index_{};

    void DrawFrameStats() noexcept;
    void DrawRenderState() noexcept;
    void DrawGraphs() noexcept;
    void UpdateHistory(const FrameStats& stats) noexcept;
};
