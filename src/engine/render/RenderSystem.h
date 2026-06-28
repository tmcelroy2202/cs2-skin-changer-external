#pragma once

#include "core/Core.h"

/* ── RenderSystem ───────────────────────────────────────────────────────────
 *
 * Observes rendering state transitions and frame-level statistics.
 * Tracks frame timing, draw call counts, render target switches, and
 * shader/program changes for performance analysis.
 *
 * This module does NOT modify rendering behaviour — it passively reads
 * render state from engine memory.
 *
 * Design note: Source 2 rendering architecture is multi-threaded with
 * a job-based render queue.  The data collected here represents the
 * composited state at query time and may lag behind the current frame.
 * ──────────────────────────────────────────────────────────────────────────── */

struct FrameStats {
    u64     frame_count{};          // Total frames rendered
    f32     frame_time_ms{};        // Last frame time in ms
    f32     avg_frame_time_ms{};    // Smoothed average
    f32     draw_call_count{};      // Draw calls this frame
    u32     triangle_count{};       // Triangles this frame
    u32     vertex_count{};         // Vertices this frame
    f32     gpu_time_ms{};          // GPU time for last frame
    f32     cpu_time_ms{};          // CPU time for last frame
};

struct RenderStateSnapshot {
    u64     current_render_target{};// Address of current render target
    u64     current_depth_stencil{};
    u32     viewport_width{};
    u32     viewport_height{};
    u64     current_material{};     // Address of last bound material
    u64     current_shader{};       // Address of last bound shader
    u64     current_command_list{}; // Address of active command list
    bool    is_rendering{};         // Whether a frame is in progress
};

class RenderSystem {
public:
    /// Initialise render state tracking.
    bool Init() noexcept;

    explicit operator bool() const noexcept { return initialised_; }

    /// Query current frame statistics.
    [[nodiscard]] FrameStats GetFrameStats() const noexcept;

    /// Query current rendering state snapshot.
    [[nodiscard]] RenderStateSnapshot GetRenderState() const noexcept;

    /// Get total GPU memory used by render resources (approximate).
    [[nodiscard]] u64 GetGPUMemoryUsage() const noexcept;

    /// Get current resolution.
    [[nodiscard]] std::pair<u32, u32> GetResolution() const noexcept;

private:
    bool initialised_{};
    u64  render_system_addr_{};
};
