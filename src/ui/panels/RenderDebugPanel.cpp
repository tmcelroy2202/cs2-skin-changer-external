#include "RenderDebugPanel.h"
#include "imgui.h"

void RenderDebugPanel::Draw() noexcept {
    if (!render_ || !*render_) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Render system not initialised");
        return;
    }

    auto stats = render_->GetFrameStats();
    UpdateHistory(stats);

    DrawFrameStats();
    ImGui::Separator();
    DrawGraphs();
    ImGui::Separator();
    DrawRenderState();
}

void RenderDebugPanel::UpdateHistory(const FrameStats& stats) noexcept {
    frame_time_history_[history_index_] = stats.frame_time_ms;
    draw_call_history_[history_index_]  = stats.draw_call_count;
    history_index_ = (history_index_ + 1) % kHistorySize;
}

void RenderDebugPanel::DrawFrameStats() noexcept {
    auto stats = render_->GetFrameStats();

    ImGui::Text("Frame Stats");
    ImGui::Separator();

    ImGui::Text("Frame count:      %llu", stats.frame_count);
    ImGui::Text("Frame time:       %.2f ms (%.1f FPS)",
                stats.frame_time_ms,
                stats.frame_time_ms > 0 ? 1000.0f / stats.frame_time_ms : 0);
    ImGui::Text("Avg frame time:   %.2f ms", stats.avg_frame_time_ms);
    ImGui::Text("Draw calls:       %.0f", stats.draw_call_count);
    ImGui::Text("Triangles:        %u", stats.triangle_count);
    ImGui::Text("Vertices:         %u", stats.vertex_count);

    auto [w, h] = render_->GetResolution();
    ImGui::Text("Resolution:       %ux%u", w, h);

    ImGui::Text("GPU memory:       %.2f MB",
                render_->GetGPUMemoryUsage() / (1024.0 * 1024.0));
}

void RenderDebugPanel::DrawRenderState() noexcept {
    auto state = render_->GetRenderState();

    ImGui::Text("Current Render State");
    ImGui::Separator();

    ImGui::Text("Render target:    0x%llX", state.current_render_target);
    ImGui::Text("Depth stencil:    0x%llX", state.current_depth_stencil);
    ImGui::Text("Viewport:         %ux%u", state.viewport_width, state.viewport_height);
    ImGui::Text("Current material: 0x%llX", state.current_material);
    ImGui::Text("Current shader:   0x%llX", state.current_shader);
    ImGui::Text("Is rendering:     %s", state.is_rendering ? "Yes" : "No");
}

void RenderDebugPanel::DrawGraphs() noexcept {
    ImGui::Text("Frame Time History (last %zu frames)", kHistorySize);
    ImGui::PlotLines("##frametime", frame_time_history_, kHistorySize,
                     static_cast<int>(history_index_),
                     nullptr, 0, 50.0f, ImVec2(0, 80));

    ImGui::Text("Draw Call History (last %zu frames)", kHistorySize);
    ImGui::PlotLines("##drawcalls", draw_call_history_, kHistorySize,
                     static_cast<int>(history_index_),
                     nullptr, 0, 5000.0f, ImVec2(0, 80));
}
