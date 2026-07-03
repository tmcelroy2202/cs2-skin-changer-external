#pragma once

#include "core/Core.h"
#include <string>
#include <chrono>

/* ── OverlayWindow ──────────────────────────────────────────────────────────
 *
 * Transparent click-through overlay window for rendering Dear ImGui.
 *
 * Creates a new topmost layered window with a random class/title to avoid
 * detection.  The overlay owns its own D3D11 device, swap chain, and render
 * target, completely independent of any game rendering pipeline.
 * ──────────────────────────────────────────────────────────────────────────── */

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11BlendState;

namespace overlay {

struct OverlayState {
    void*           hwnd                = nullptr;   // HWND
    void*           hinstance           = nullptr;   // HINSTANCE
    ID3D11Device*   device              = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    IDXGISwapChain* swap_chain          = nullptr;
    ID3D11RenderTargetView* rt_view     = nullptr;
    ID3D11BlendState* blend_state       = nullptr;

    std::wstring    class_name;                      // Window class (for unregister)

    u32             width               = 1920;
    u32             height              = 1080;
    i32             desired_fps         = 120;
    bool            vsync               = false;
    bool            running             = false;
    bool            initialised         = false;

    void (*toggle_callback)() = nullptr;

    // Frame timing — stored in state instead of a function-static for thread safety
    std::chrono::high_resolution_clock::time_point last_frame{};
};

/// Initialise the overlay window (hijack or create).
bool Init();

/// Shutdown and release all overlay resources.
void Shutdown() noexcept;

/// Pump Win32 messages, begin the ImGui frame.
void NewFrame() noexcept;

/// End the ImGui frame and present to the swap chain.
void EndFrame() noexcept;

/// Access the overlay state.
OverlayState& State() noexcept;

/// Check if the overlay is running.
bool IsRunning() noexcept;

/// Toggle click-through mode so the overlay captures (or passes) mouse input.
void SetClickthrough(bool enabled) noexcept;

/// Apply a custom CS2-inspired dark style to ImGui.
void ApplyCustomStyle() noexcept;

} // namespace overlay
