#pragma once

#include "core/Core.h"

/* ── Framework ──────────────────────────────────────────────────────────────
 *
 * Dear ImGui integration layer.  Manages the ImGui lifecycle, input handling,
 * window creation (via Win32), and D3D11 render target setup.
 *
 * Two operating modes:
 *   1. Overlay mode  — creates/hijacks a transparent overlay window with its
 *      own D3D11 device and swap chain; fully independent of the target.
 *   2. Hook mode     — hooks an existing IDXGISwapChain::Present to render
 *      inside the target process (injection).
 *
 * Mode is selected at init time via InitParams::overlay_mode.
 * ──────────────────────────────────────────────────────────────────────────── */

struct ImGuiContext;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

namespace ui {

/// Initialisation parameters.
struct InitParams {
    /// True  = create transparent overlay window (standalone)
    /// False = hook into existing swap chain (injected DLL)
    bool     overlay_mode    = true;

    // ── Hook mode only ──
    void*    target_window   = nullptr;  // HWND to attach to
    ID3D11Device*       device      = nullptr;
    ID3D11DeviceContext* context     = nullptr;
    IDXGISwapChain*     swap_chain  = nullptr;

    // ── Overlay mode only ──
    u32      initial_width   = 1920;
    u32      initial_height  = 1080;
    i32      desired_fps     = 120;
    bool     vsync           = false;
};

/// Initialise the UI framework.
bool Init(const InitParams& params) noexcept;

/// Shutdown and release all resources.
void Shutdown() noexcept;

/// Begin a new frame.  Call before drawing panels.
void NewFrame() noexcept;

/// Render all ImGui draw data to the output.
void Render() noexcept;

/// Handle Win32 window message (hook mode input forwarding).
bool WndProcHandler(void* hwnd, u32 msg, u64 wparam, i64 lparam) noexcept;

/// Toggle UI visibility.
void ToggleVisibility() noexcept;
bool IsVisible() noexcept;

/// Get the overlay HWND (for CreateWindow-style embedding).
void* GetOverlayWindow() noexcept;

} // namespace ui
