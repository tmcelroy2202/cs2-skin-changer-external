#pragma once

#include "core/Core.h"

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

    u32             width               = 1920;
    u32             height              = 1080;
    i32             desired_fps         = 120;
    bool            vsync               = false;
    bool            running             = false;
    bool            initialised         = false;

    void (*toggle_callback)() = nullptr;
};

/// Initialise the overlay window (hijack or create).
bool Init() noexcept;

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

} // namespace overlay
