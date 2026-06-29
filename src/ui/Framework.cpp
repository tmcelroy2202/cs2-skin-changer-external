#include "Framework.h"
#include "overlay/OverlayWindow.h"
#include "core/Logging.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam);

namespace {

enum class Mode { None, Overlay, Hook };

struct UIData {
    Mode mode            = Mode::None;
    bool visible         = true;
    bool initialised     = false;

    // Hook-mode resources
    ID3D11Device*        hook_device     = nullptr;
    ID3D11DeviceContext* hook_context    = nullptr;
    IDXGISwapChain*      hook_swapchain  = nullptr;
};

UIData& Data() {
    static UIData d;
    return d;
}

} // anonymous namespace

// ── Init ──────────────────────────────────────────────────────────────────────

bool ui::Init(const InitParams& params) noexcept {
    auto& d = Data();
    if (d.initialised) return true;

    if (params.overlay_mode) {
        LOG_INFO("UI Framework starting in OVERLAY mode");

        if (!overlay::Init()) {
            LOG_ERROR("Overlay initialisation failed");
            return false;
        }
        // Register toggle callback so overlay-mode Insert key can toggle visibility
        overlay::State().toggle_callback = []() { ToggleVisibility(); };
        d.mode = Mode::Overlay;

    } else {
        LOG_INFO("UI Framework starting in HOOK mode");

        if (!params.device || !params.context || !params.swap_chain) {
            LOG_ERROR("Hook mode requires valid D3D11 device/context/swapchain");
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        HWND hwnd = static_cast<HWND>(params.target_window);

        if (!ImGui_ImplWin32_Init(hwnd)) {
            LOG_ERROR("ImGui_ImplWin32_Init failed");
            return false;
        }
        if (!ImGui_ImplDX11_Init(params.device, params.context)) {
            LOG_ERROR("ImGui_ImplDX11_Init failed");
            return false;
        }

        d.hook_device    = params.device;
        d.hook_context   = params.context;
        d.hook_swapchain = params.swap_chain;

        d.mode = Mode::Hook;
    }

    overlay::ApplyCustomStyle();

    d.initialised = true;
    LOG_INFO("UI Framework initialised");
    return true;
}

// ── Shutdown ──────────────────────────────────────────────────────────────────

void ui::Shutdown() noexcept {
    auto& d = Data();
    if (!d.initialised) return;

    switch (d.mode) {
    case Mode::Overlay:
        overlay::Shutdown();
        break;
    case Mode::Hook:
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        break;
    case Mode::None:
        break;
    }

    d = {};
}

// ── Frame lifecycle ───────────────────────────────────────────────────────────

void ui::NewFrame() noexcept {
    auto& d = Data();
    if (!d.initialised) return;

    switch (d.mode) {
    case Mode::Overlay:
        overlay::NewFrame();
        break;
    case Mode::Hook:
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        break;
    case Mode::None:
        break;
    }
}

void ui::Render() noexcept {
    auto& d = Data();
    if (!d.initialised) return;

    switch (d.mode) {
    case Mode::Overlay:
        overlay::EndFrame();
        break;
    case Mode::Hook:
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        break;
    case Mode::None:
        break;
    }
}

// ── WndProc handling ─────────────────────────────────────────────────────────

bool ui::WndProcHandler(void* hwnd, u32 msg, u64 wparam, i64 lparam) noexcept {
    if (!Data().initialised) return false;

    // Only let ImGui consume input when the UI overlay is visible
    if (Data().visible) {
        if (ImGui_ImplWin32_WndProcHandler(
                static_cast<HWND>(hwnd), msg, wparam, lparam))
            return true;
    }

    if (msg == WM_KEYUP && wparam == VK_INSERT) {
        ToggleVisibility();
        return true;
    }
    return false;
}

// ── Visibility ────────────────────────────────────────────────────────────────

void ui::ToggleVisibility() noexcept {
    Data().visible = !Data().visible;
    if (Data().mode == Mode::Overlay) {
        overlay::SetClickthrough(!Data().visible);
        if (Data().visible) {
            ::ClipCursor(nullptr);
            HWND hwnd = static_cast<HWND>(overlay::State().hwnd);
            if (hwnd) {
                ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                ::SetForegroundWindow(hwnd);
                ::SetFocus(hwnd);
            }
        }
    }
}

bool ui::IsVisible() noexcept {
    return Data().visible && Data().initialised;
}

void* ui::GetOverlayWindow() noexcept {
    if (Data().mode == Mode::Overlay)
        return overlay::State().hwnd;
    return nullptr;
}
