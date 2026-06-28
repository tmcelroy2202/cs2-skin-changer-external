#include "OverlayWindow.h"
#include "core/Logging.h"
#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace {

// Toggle UI visibility from overlay mode (callback set by Framework)
void ToggleOverlayVisibility() {
    auto& s = overlay::State();
    if (s.toggle_callback) s.toggle_callback();
}

// ── Forward declare the WndProc ──────────────────────────────────────────────
LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ── Windows procedure for ImGui ──────────────────────────────────────────────
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // anonymous namespace

namespace overlay {

OverlayState& State() {
    static OverlayState s;
    return s;
}

bool IsRunning() noexcept {
    return State().running;
}

// ── Internal helpers ─────────────────────────────────────────────────────────

static bool CreateRenderTarget() noexcept {
    auto& s = State();
    ID3D11Texture2D* back_buffer = nullptr;
    HRESULT hr = s.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (FAILED(hr)) {
        LOG_ERROR("GetBuffer failed: 0x{:X}", hr);
        return false;
    }
    hr = s.device->CreateRenderTargetView(back_buffer, nullptr, &s.rt_view);
    back_buffer->Release();
    if (FAILED(hr)) {
        LOG_ERROR("CreateRenderTargetView failed: 0x{:X}", hr);
        return false;
    }
    return true;
}

static void CreateBlendState() noexcept {
    auto& s = State();
    D3D11_BLEND_DESC desc{};
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    s.device->CreateBlendState(&desc, &s.blend_state);
}

static bool InitD3D11(HWND hwnd) noexcept {
    auto& s = State();
    s.width  = GetSystemMetrics(SM_CXSCREEN);
    s.height = GetSystemMetrics(SM_CYSCREEN);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = s.width;
    sd.BufferDesc.Height                  = s.height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = s.vsync ? 60 : 0;
    sd.BufferDesc.RefreshRate.Denominator = s.vsync ? 1 : 0;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, _countof(levels), D3D11_SDK_VERSION,
        &sd, &s.swap_chain, &s.device, &feature_level, &s.device_context);

    if (FAILED(hr)) {
        LOG_ERROR("D3D11CreateDeviceAndSwapChain failed: 0x{:X}", hr);
        return false;
    }

    if (!CreateRenderTarget()) return false;
    CreateBlendState();

    // ── ImGui init ────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    if (!ImGui_ImplWin32_Init(hwnd)) {
        LOG_ERROR("ImGui_ImplWin32_Init failed");
        return false;
    }
    if (!ImGui_ImplDX11_Init(s.device, s.device_context)) {
        LOG_ERROR("ImGui_ImplDX11_Init failed");
        return false;
    }

    return true;
}

// ── Create: make our own overlay window ──────────────────────────────────────

static std::wstring RandomString(int len) noexcept {
    static const wchar_t chars[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::wstring out(len, L'\0');
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, sizeof(chars) / sizeof(chars[0]) - 2);
    for (int i = 0; i < len; ++i) out[i] = chars[dist(rng)];
    return out;
}

static bool CreateOverlayWindow() noexcept {
    auto& s = State();

    std::wstring class_name = RandomString(16);
    std::wstring win_title  = RandomString(12);

    WNDCLASSEXW wcex{};
    wcex.cbSize        = sizeof(WNDCLASSEXW);
    wcex.style         = 0;
    wcex.lpfnWndProc   = OverlayWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = GetModuleHandleW(nullptr);
    wcex.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName  = nullptr;
    wcex.lpszClassName = class_name.c_str();
    wcex.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) {
        LOG_ERROR("RegisterClassExW failed");
        return false;
    }
    s.hinstance = wcex.hInstance;

    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);

    HWND hwnd = CreateWindowExW(
        0, class_name.c_str(), win_title.c_str(), WS_POPUP,
        rect.left, rect.top, rect.right, rect.bottom,
        nullptr, nullptr, wcex.hInstance, nullptr);

    if (!hwnd) {
        LOG_ERROR("CreateWindowExW failed");
        return false;
    }

    s.hwnd = hwnd;

    SetWindowLongPtrA(hwnd, GWL_EXSTYLE,
        WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED);

    MARGINS margin{-1};
    DwmExtendFrameIntoClientArea(hwnd, &margin);

    ShowWindow(hwnd, SW_SHOW);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    UpdateWindow(hwnd);

    LOG_INFO("Created overlay window: 0x{:X}", reinterpret_cast<u64>(hwnd));
    return true;
}

// ── Init / Shutdown ──────────────────────────────────────────────────────────

bool Init() noexcept {
    auto& s = State();
    if (s.initialised) return true;

    if (!CreateOverlayWindow()) {
        LOG_ERROR("Failed to create overlay window");
        return false;
    }

    if (!InitD3D11(static_cast<HWND>(s.hwnd))) {
        LOG_ERROR("D3D11 initialisation failed");
        Shutdown();
        return false;
    }

    s.running     = true;
    s.initialised = true;
    LOG_INFO("Overlay initialised ({}x{}, {} FPS cap)",
             s.width, s.height, s.desired_fps);
    return true;
}

void Shutdown() noexcept {
    auto& s = State();
    if (!s.initialised) return;

    LOG_INFO("Overlay shutting down...");

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (s.rt_view)       { s.rt_view->Release();       s.rt_view       = nullptr; }
    if (s.blend_state)   { s.blend_state->Release();    s.blend_state   = nullptr; }
    if (s.swap_chain)    { s.swap_chain->Release();     s.swap_chain    = nullptr; }
    if (s.device_context){ s.device_context->Release(); s.device_context = nullptr; }
    if (s.device)        { s.device->Release();         s.device        = nullptr; }

    if (s.hwnd) {
        DestroyWindow(static_cast<HWND>(s.hwnd));
        s.hwnd = nullptr;
    }
    s.hinstance = nullptr;

    s.running     = false;
    s.initialised = false;
}

// ── Frame lifecycle ──────────────────────────────────────────────────────────

void NewFrame() noexcept {
    auto& s = State();
    if (!s.running) return;

    // FPS limiter (relative delta, no drift)
    static auto last_frame = std::chrono::high_resolution_clock::now();
    auto interval = std::chrono::duration_cast<
        std::chrono::high_resolution_clock::duration>(
            std::chrono::duration<double>(1.0 / s.desired_fps));

    // Pump messages
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            s.running = false;
            return;
        }
    }

    // Handle Insert key for visibility toggle (edge-triggered)
    static bool insert_was_down = false;
    bool insert_down = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (insert_down && !insert_was_down) {
        ToggleOverlayVisibility();
    }
    insert_was_down = insert_down;

    // Manual mouse input (no WndProc needed for click-through)
    POINT pt;
    GetCursorPos(&pt);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos      = ImVec2(static_cast<f32>(pt.x), static_cast<f32>(pt.y));
    io.MouseDown[0]  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    io.MouseDown[1]  = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    // Compute real delta time for smooth animation
    auto now = std::chrono::high_resolution_clock::now();
    f32 dt = std::chrono::duration<f32>(now - last_frame).count();
    io.DeltaTime = std::max(dt, 1.0f / 120.0f);

    // Sleep to maintain target framerate (relative to last frame)
    auto elapsed = std::chrono::high_resolution_clock::now() - last_frame;
    if (elapsed < interval) {
        std::this_thread::sleep_for(interval - elapsed);
    }
    last_frame = std::chrono::high_resolution_clock::now();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EndFrame() noexcept {
    auto& s = State();
    if (!s.running) return;

    ImGui::Render();

    const f32 clear[4] = {0, 0, 0, 0};
    s.device_context->OMSetRenderTargets(1, &s.rt_view, nullptr);
    s.device_context->ClearRenderTargetView(s.rt_view, clear);

    f32 blend_factor[4] = {0, 0, 0, 0};
    s.device_context->OMSetBlendState(s.blend_state, blend_factor, 0xFFFFFFFF);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    s.swap_chain->Present(s.vsync ? 1 : 0, 0);
}

} // namespace overlay

// ── WndProc ───────────────────────────────────────────────────────────────────

namespace {

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // anonymous namespace
