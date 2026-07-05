/* ── VeneeR — DLL Entry Point ─────────────────────────────────────────────────
 *
 * Injected into CS2 via the loader (CS2SkinChanger.exe).
 * Hooks IDXGISwapChain::Present to render the ImGui overlay.
 * All memory access is in-process — no external Read/WriteProcessMemory.
 * ──────────────────────────────────────────────────────────────────────────── */

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <atomic>
#include <memory>

#include "MinHook.h"

#include "core/Core.h"
#include "core/Logging.h"
#include "core/ProcessMemory.h"
#include "core/DllAntiDetect.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/SkinTextureCache.h"
#include "cs2/ConfigManager.h"
#include "cs2/Offsets.h"
#include "cs2/EntitySystem.h"
#include "ui/Framework.h"
#include "ui/Panels.h"
#include "ui/overlay/OverlayWindow.h"
#include "ui/panels/SkinChangerPanel.h"
#include "ui/panels/DebugPanel.h"
#include "ui/panels/ConsolePanel.h"

namespace {

HMODULE g_module{};

struct App {
    PanelManager        panels;

    ProcessMemory        cs2;
    SkinDatabase         skin_db;
    SkinManager          skin_mgr;
    SkinChanger*         skin_changer{};
    SkinChangerPanel*    skin_panel{};
    SkinTextureCache*    skin_tex_cache{};
    DebugPanel*          debug_panel{};
    ConsolePanel*        console_panel{};
    HWND                 game_hwnd{};

    std::atomic<bool> initialised{false};
};

App& State() {
    static App a;
    return a;
}

void RenderUI(App& app) {
    if (!ui::IsVisible()) return;

    if (ImGui::BeginMainMenuBar()) {
        if (app.skin_panel) {
            bool open = app.skin_panel->IsOpen();
            if (ImGui::MenuItem("VeneeR", nullptr, &open))
                app.skin_panel->SetOpen(open);
            ImGui::SameLine();
        }
        if (app.debug_panel) {
            bool open = app.debug_panel->IsOpen();
            if (ImGui::MenuItem("Debug", nullptr, &open))
                app.debug_panel->SetOpen(open);
            ImGui::SameLine();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Toggle UI", "Insert")) { ui::ToggleVisibility(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Update Offsets (cs2-dumper)")) {
                if (Offsets::UpdateFromRemote())
                    LOG_INFO("Offsets updated successfully from cs2-dumper");
                else
                    LOG_WARN("Offsets update failed — check internet or cs2-dumper availability");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("##status", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav)) {
        ImGui::Text("VeneeR  v1.8.2");
        ImGui::TextColored(ImVec4(0,1,0,1), "Active");
    }
    ImGui::End();

    app.panels.DrawAll();

    if (app.skin_panel && app.skin_panel->IsOpen())
        app.skin_panel->Draw();
}

// ── Present hook ──────────────────────────────────────────────────────────────
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
PresentFn original_present = nullptr;

// Window proc chain for input forwarding
WNDPROC original_wndproc = nullptr;

LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ui::WndProcHandler(hwnd, msg, wparam, lparam))
        return 1;
    if (original_wndproc)
        return CallWindowProcW(original_wndproc, hwnd, msg, wparam, lparam);
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

// One-time UI init state
std::atomic<bool> ui_initialised{false};
std::atomic<bool> shutting_down{false};

HRESULT STDMETHODCALLTYPE PresentHook(
    IDXGISwapChain* swapchain, UINT sync_interval, UINT flags)
{
    if (shutting_down)
        return original_present(swapchain, sync_interval, flags);

    auto& app = State();

    // One-time ImGui init on the render thread (where we have the swap chain)
    if (!ui_initialised) {
        ui_initialised = true;

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        if (SUCCEEDED(swapchain->GetDevice(IID_PPV_ARGS(&device)))) {
            device->GetImmediateContext(&context);

            DXGI_SWAP_CHAIN_DESC desc;
            swapchain->GetDesc(&desc);
            HWND hwnd = desc.OutputWindow;

            // Subclass window for ImGui input (Insert toggle, mouse, etc.)
            app.game_hwnd = hwnd;
            original_wndproc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                  reinterpret_cast<LONG_PTR>(WndProcHook)));
            if (!original_wndproc) {
                LOG_WARN("SetWindowLongPtrW failed — UI input may not work (error={})",
                         GetLastError());
            }

            ui::InitParams params{};
            params.overlay_mode   = false;
            params.device         = device;
            params.context        = context;
            params.swap_chain     = swapchain;
            params.target_window  = hwnd;
            ui::Init(params);

            // Skin texture cache needs the game's D3D11 device
            {
                app.skin_tex_cache = new SkinTextureCache(device);
                app.skin_tex_cache->InitMapping();
                if (app.skin_panel)
                    app.skin_panel->SetTextureCache(app.skin_tex_cache);
            }
        }
    }

    if (app.initialised && ui_initialised) {
        if (app.skin_changer) {
            // Tick skin changer (in-process memory writes)
            app.skin_changer->Tick();

            // Update equipped weapon display
            if (app.skin_panel && app.cs2) {
                u64 local = GetLocalPlayer(app.cs2);
                if (local) {
                    auto def_id = GetActiveWeaponDef(app.cs2, local);
                    auto wep = static_cast<i32>(def_id);
                    if (wep > 0)
                        app.skin_panel->SetEquippedWeapon(WeaponIdToString(wep), wep);
                }
            }
        }

        // Poll Insert key directly — CS2 uses raw input so WM_KEYUP never fires
        static bool prev_insert = false;
        bool cur_insert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (cur_insert && !prev_insert)
            ui::ToggleVisibility();
        prev_insert = cur_insert;

        ui::NewFrame();
        RenderUI(app);
        ui::Render();
    }

    return original_present(swapchain, sync_interval, flags);
}

// ── Find Present function address ─────────────────────────────────────────────
void* FindPresentFunction() {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapchain = nullptr;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 1;
    sd.BufferDesc.Height = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetDesktopWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &swapchain,
        &device, nullptr, &context);

    if (FAILED(hr)) return nullptr;

    // vtable[8] = IDXGISwapChain::Present
    void* present = (*reinterpret_cast<void***>(swapchain))[8];

    swapchain->Release();
    context->Release();
    device->Release();

    return present;
}

// ── Init thread ───────────────────────────────────────────────────────────────
DWORD WINAPI InitThread(LPVOID) noexcept {
    // Set base path outside DllMain (std::filesystem is unsafe under loader lock)
    char dll_path[MAX_PATH]{};
    GetModuleFileNameA(g_module, dll_path, MAX_PATH);
    SetBasePath(dll_path);

    auto& app = State();

    LOG_INFO("VeneeR — DLL injection mode (v1.8.2)");

    // Anti-debugging check
    if (antidebug::DetectDebugger()) {
        LOG_WARN("Debugger detected — VeneeR disabled for safety");
        return 0;
    }

    // ── VeneeR ────────────────────────────────────────────────────────────
    if (app.cs2.OpenCurrentProcess()) {
        LOG_INFO("In-process memory access established");

        // Auto-update offsets from cs2-dumper (best-effort)
        Offsets::UpdateFromRemote();

        // Load offsets from cache or defaults
        auto off_path = Offsets::DataFilePath();
        if (Offsets::LoadFromFile(off_path)) {
            LOG_INFO("Offsets loaded from {}", off_path.string().c_str());
        } else {
            Offsets::SaveToFile(off_path);
            LOG_INFO("Default offsets written to {}", off_path.string().c_str());
        }
        Offsets::ValidateLoadedOffsets();

        app.debug_panel  = new DebugPanel(app.cs2);
        app.skin_changer = new SkinChanger(app.cs2, app.skin_db, app.skin_mgr,
                                           app.debug_panel);
        app.skin_panel   = new SkinChangerPanel(app.skin_db, app.skin_mgr,
                                                  *app.skin_changer);
        app.panels.Register(app.debug_panel);

        app.console_panel = new ConsolePanel();
        Logger::Instance().SetCallback(ConsolePanel::LoggerCallback);
        app.panels.Register(app.console_panel);

        u16 saved_music = app.skin_changer->GetMusicKit();
        LoadConfig(app.skin_mgr, saved_music);
        app.skin_changer->SetMusicKit(saved_music);
        LOG_INFO("Config loaded from {}", ConfigPath().string().c_str());
    } else {
        LOG_WARN("CS2 in-process memory access failed — VeneeR disabled");
    }

    // ── Hook IDXGISwapChain::Present via MinHook ─────────────────────────
    MH_Initialize();
    void* present_addr = FindPresentFunction();
    if (present_addr) {
        MH_STATUS s = MH_CreateHook(present_addr,
                                     reinterpret_cast<LPVOID>(&PresentHook),
                                     reinterpret_cast<void**>(&original_present));
        if (s == MH_OK) {
            MH_EnableHook(present_addr);
            LOG_INFO("Present hook installed successfully");
        } else {
            LOG_WARN("MH_CreateHook failed: {}", MH_StatusToString(s));
        }
    } else {
        LOG_WARN("Failed to locate IDXGISwapChain::Present");
    }

    app.initialised = true;
    LOG_INFO("DLL initialised successfully");
    return 0;
}

// ── Shutdown ──────────────────────────────────────────────────────────────────
void ShutdownDLL() noexcept {
    auto& app = State();
    if (!app.initialised) return;

    // Signal PresentHook to stop accessing app state
    shutting_down = true;

    // Restore original window proc so the game doesn't crash after DLL unload
    if (original_wndproc && app.game_hwnd && IsWindow(app.game_hwnd)) {
        SetWindowLongPtrW(app.game_hwnd, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(original_wndproc));
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    ui::Shutdown();

    Logger::Instance().SetCallback(nullptr);
    delete app.skin_changer;
    delete app.skin_panel;
    delete app.debug_panel;
    delete app.console_panel;
    delete app.skin_tex_cache;

    app.cs2.Close();
    std::destroy_at(&app);
    std::construct_at(&app);
}

} // anonymous namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g_module = module;
        HANDLE thread = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        ShutdownDLL();
    }
    return TRUE;
}
