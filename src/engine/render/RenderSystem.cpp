#include "RenderSystem.h"
#include "core/Module.h"
#include "core/Memory.h"
#include "core/PatternScanner.h"
#include "core/Logging.h"

bool RenderSystem::Init() noexcept {
    // The render system may be accessed through the engine's global state.
    // Try multiple known modules:
    auto mod = Module::Get("rendersystemdx11.dll");
    if (!mod) mod = Module::Get("rendersystemvulkan.dll");
    if (!mod) mod = Module::Get("engine2.dll");
    if (!mod) return false;

    // Scan for the render system singleton reference
    auto found = PatternScanner::FindString(
        mod, "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 48 8B F0 48 85 F6");
    if (found) {
        i32 disp = memory::Read<i32>(*found + 3);
        u64 global = *found + 7 + disp;
        render_system_addr_ = memory::Read<u64>(global);
        LOG_INFO("RenderSystem at 0x{:X}", render_system_addr_);
        initialised_ = true;
        return true;
    }

    // Alternative: try the render context
    // Many Source 2 games expose an IRenderContext interface
    auto* iface = InterfaceDiscovery::Get<void>(mod,
        "Source2EngineToRenderContextV001");
    if (iface) {
        render_system_addr_ = reinterpret_cast<u64>(iface);
        LOG_INFO("RenderContext at 0x{:X}", render_system_addr_);
        initialised_ = true;
        return true;
    }

    LOG_WARN("RenderSystem not found");
    return false;
}

FrameStats RenderSystem::GetFrameStats() const noexcept {
    FrameStats stats{};
    if (!initialised_) return stats;

    // Offsets are version-dependent.  These represent common locations.
    stats.frame_count      = memory::Read<u64>(render_system_addr_ + 0x08);
    stats.frame_time_ms    = memory::Read<f32>(render_system_addr_ + 0x18);
    stats.avg_frame_time_ms= memory::Read<f32>(render_system_addr_ + 0x1C);
    stats.draw_call_count  = memory::Read<f32>(render_system_addr_ + 0x38);
    stats.triangle_count   = memory::Read<u32>(render_system_addr_ + 0x40);
    stats.vertex_count     = memory::Read<u32>(render_system_addr_ + 0x44);

    return stats;
}

RenderStateSnapshot RenderSystem::GetRenderState() const noexcept {
    RenderStateSnapshot state{};
    if (!initialised_) return state;

    state.current_render_target  = memory::Read<u64>(render_system_addr_ + 0x50);
    state.current_depth_stencil  = memory::Read<u64>(render_system_addr_ + 0x58);
    state.viewport_width         = memory::Read<u32>(render_system_addr_ + 0x68);
    state.viewport_height        = memory::Read<u32>(render_system_addr_ + 0x6C);
    state.current_material       = memory::Read<u64>(render_system_addr_ + 0x78);
    state.current_shader         = memory::Read<u64>(render_system_addr_ + 0x80);
    state.is_rendering           = memory::Read<bool>(render_system_addr_ + 0x90);

    return state;
}

u64 RenderSystem::GetGPUMemoryUsage() const noexcept {
    if (!initialised_) return 0;
    return memory::Read<u64>(render_system_addr_ + 0xA0);
}

std::pair<u32, u32> RenderSystem::GetResolution() const noexcept {
    if (!initialised_) return {};
    return {
        memory::Read<u32>(render_system_addr_ + 0x68),
        memory::Read<u32>(render_system_addr_ + 0x6C)
    };
}
