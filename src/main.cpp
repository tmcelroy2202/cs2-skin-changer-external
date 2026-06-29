/* ── VeneeR — Entry Point ────────────────────────────────────────────────────
 *
 * Two build modes:
 *   Standalone (CSC_BUILD_STANDALONE=ON)
 *     Creates/hijacks a transparent overlay window with its own D3D11 device
 *     and runs the full ImGui debug UI in its own render loop.  Mirrors the
 *     reference overlay approach (window class "Chrome_WidgetWin_1", Discord
 *     hijack fallback, click-through layered window).
 *
 *   DLL (default)
 *     Exposes DllMain for in-process injection.  Hooks the game's
 *     IDXGISwapChain::Present to overlay the debug UI.
 * ──────────────────────────────────────────────────────────────────────────── */

#include "core/Core.h"
#include "core/Logging.h"

#include "engine/schema/SchemaSystem.h"
#include "engine/schema/SchemaReflection.h"
#include "engine/entity/EntitySystem.h"
#include "engine/resource/ResourceSystem.h"
#include "engine/render/RenderSystem.h"
#include "engine/render/MaterialSystem.h"

#include "ui/Framework.h"
#include "ui/Panels.h"
#include "ui/overlay/OverlayWindow.h"
#include "ui/panels/SchemaBrowserPanel.h"
#include "ui/panels/EntityBrowserPanel.h"
#include "ui/panels/ResourceViewerPanel.h"
#include "ui/panels/MemoryInspectorPanel.h"
#include "ui/panels/RenderDebugPanel.h"
#include "ui/panels/ConsolePanel.h"

#include "analysis/ObjectInspector.h"
#include "analysis/AssetAnalyzer.h"

#ifdef CSC_BUILD_STANDALONE

// ── Standalone overlay mode ────────────────────────────────────────────────────
#include "core/ProcessMemory.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/SkinTextureCache.h"
#include "cs2/EntitySystem.h"
#include "cs2/Offsets.h"
#include "cs2/ConfigManager.h"
#include "ui/panels/SkinChangerPanel.h"
// ──────────────────────────────────────────────────────────────────────────────
//  STANDALONE OVERLAY MODE
// ──────────────────────────────────────────────────────────────────────────────

#include <atomic>
#include <thread>
#include <objbase.h>

namespace {

struct App {
    SchemaSystem        schema;
    EntitySystem        entities;
    ResourceSystem      resources;
    RenderSystem        render;
    MaterialSystem      materials;

    PanelManager        panels;
    SchemaBrowserPanel*  schema_panel{};
    EntityBrowserPanel*  entity_panel{};
    ResourceViewerPanel* resource_panel{};
    MemoryInspectorPanel* memory_panel{};
    RenderDebugPanel*     render_panel{};
    ConsolePanel*         console_panel{};

    ObjectInspector*    obj_inspector{};
    AssetAnalyzer*      asset_analyzer{};

    // ── VeneeR ────────────────────────────────────────────────────────────
    ProcessMemory        cs2;
    SkinDatabase         skin_db;
    SkinManager          skin_mgr;
    SkinChanger*         skin_changer{};
    SkinChangerPanel*    skin_panel{};
    SkinTextureCache*    skin_tex_cache{};

    bool running = false;
};

App& State() {
    static App a;
    return a;
}

void RunFrame(App& app) {
    // ── Render UI ─────────────────────────────────────────────────────────
    if (ui::IsVisible()) {
        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Panels")) {
                if (ImGui::MenuItem("Schema Browser", nullptr, app.schema_panel && app.schema_panel->IsOpen()))
                    if (app.schema_panel) app.schema_panel->SetOpen(!app.schema_panel->IsOpen());
                if (ImGui::MenuItem("Entity Browser", nullptr, app.entity_panel && app.entity_panel->IsOpen()))
                    if (app.entity_panel) app.entity_panel->SetOpen(!app.entity_panel->IsOpen());
                if (ImGui::MenuItem("Resource Viewer", nullptr, app.resource_panel && app.resource_panel->IsOpen()))
                    if (app.resource_panel) app.resource_panel->SetOpen(!app.resource_panel->IsOpen());
                if (ImGui::MenuItem("Memory Inspector", nullptr, app.memory_panel && app.memory_panel->IsOpen()))
                    if (app.memory_panel) app.memory_panel->SetOpen(!app.memory_panel->IsOpen());
                if (ImGui::MenuItem("Render Debug", nullptr, app.render_panel && app.render_panel->IsOpen()))
                    if (app.render_panel) app.render_panel->SetOpen(!app.render_panel->IsOpen());
                if (ImGui::MenuItem("Console", nullptr, app.console_panel && app.console_panel->IsOpen()))
                    if (app.console_panel) app.console_panel->SetOpen(!app.console_panel->IsOpen());
                if (app.skin_panel) {
                    ImGui::Separator();
                    if (ImGui::MenuItem("VeneeR", nullptr, app.skin_panel->IsOpen()))
                        app.skin_panel->SetOpen(!app.skin_panel->IsOpen());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Toggle UI", "Insert")) { ui::ToggleVisibility(); }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) { app.running = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Status overlay
        if (app.schema) {
            ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(0.5f);
            if (ImGui::Begin("##status", nullptr,
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav)) {
                ImGui::Text("VeneeR  v1.4.1");
                ImGui::Text("Scopes: %zu  Entities: %u",
                            app.schema.GetTypeScopeCount(),
                            app.entities.GetEntityCount());
            }
            ImGui::End();
        }

        app.panels.DrawAll();
    }

    // ── Skin changer tick ─────────────────────────────────────────────────────
    if (app.skin_changer) {
        app.skin_changer->Tick();
    }

    // ── Equipped weapon display ───────────────────────────────────────────────
    if (app.skin_panel && app.cs2) {
        u64 local = GetLocalPlayer(app.cs2);
        if (local) {
            auto def_id = GetActiveWeaponDef(app.cs2, local);
            auto wep = static_cast<i32>(def_id);
            // Only update if holding a real weapon (skip in-game menus, dead, etc.)
            if (wep > 0)
                app.skin_panel->SetEquippedWeapon(WeaponIdToString(wep), wep);
        }
    }

    ui::Render();
}

} // anonymous namespace

int main() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    LOG_INFO("VeneeR — Standalone Overlay Mode");

    auto& app = State();

    // ── Init engine subsystems ───────────────────────────────────────────────
    if (!app.schema.Init())
        LOG_WARN("SchemaSystem unavailable");
    if (!app.entities.Init())
        LOG_WARN("EntitySystem unavailable");
    if (!app.resources.Init())
        LOG_WARN("ResourceSystem unavailable");
    if (!app.render.Init())
        LOG_WARN("RenderSystem unavailable");
    if (!app.materials.Init())
        LOG_WARN("MaterialSystem unavailable");

    // ── Init overlay UI ──────────────────────────────────────────────────────
    ui::InitParams params{};
    params.overlay_mode   = true;
    params.desired_fps    = 120;

    if (!ui::Init(params)) {
        LOG_FATAL("Failed to initialise UI overlay");
        return 1;
    }

    // ── Analysis tools ───────────────────────────────────────────────────────
    app.obj_inspector   = new ObjectInspector(&app.schema);
    app.asset_analyzer  = new AssetAnalyzer(&app.resources, &app.materials);

    // ── Panels ───────────────────────────────────────────────────────────────
    app.schema_panel    = new SchemaBrowserPanel(&app.schema);
    app.entity_panel    = new EntityBrowserPanel(&app.entities, &app.schema);
    app.resource_panel  = new ResourceViewerPanel(&app.resources, &app.materials);
    app.memory_panel    = new MemoryInspectorPanel();
    app.render_panel    = new RenderDebugPanel(&app.render);
    app.console_panel   = new ConsolePanel();

    app.panels.Register(app.schema_panel);
    app.panels.Register(app.entity_panel);
    app.panels.Register(app.resource_panel);
    app.panels.Register(app.memory_panel);
    app.panels.Register(app.render_panel);
    app.panels.Register(app.console_panel);

    // ── VeneeR ──────────────────────────────────────────────────────────────
    if (app.cs2.Open(L"cs2.exe")) {
        LOG_INFO("Connected to cs2.exe");

        // Auto-update offsets from cs2-dumper GitHub (best-effort, no delay)
        auto off_path = Offsets::DataFilePath();
        if (Offsets::UpdateFromRemote()) {
            LOG_INFO("Offsets auto-updated from cs2-dumper");
        }

        // Load offsets from data file (cached from auto-update or pre-existing)
        if (Offsets::LoadFromFile(off_path)) {
            LOG_INFO("Offsets loaded from {}", off_path.string().c_str());
        } else {
            // First launch — write the defaults so the user can edit them
            Offsets::SaveToFile(off_path);
            LOG_INFO("Default offsets written to {}", off_path.string().c_str());
        }

        app.skin_changer = new SkinChanger(app.cs2, app.skin_db, app.skin_mgr);
        app.skin_panel   = new SkinChangerPanel(app.skin_db, app.skin_mgr,
                                                 *app.skin_changer);

        // Texture cache for skin preview images
        {
            auto& overlay_state = overlay::State();
            if (overlay_state.device) {
                app.skin_tex_cache = new SkinTextureCache(overlay_state.device);
                app.skin_tex_cache->InitMapping();
                app.skin_panel->SetTextureCache(app.skin_tex_cache);
            }
        }

        app.panels.Register(app.skin_panel);

        // Load saved config (skins, knife, gloves, music kit)
        u16 saved_music = app.skin_changer->GetMusicKit();
        LoadConfig(app.skin_mgr, saved_music);
        app.skin_changer->SetMusicKit(saved_music);
        LOG_INFO("Config loaded from {}", ConfigPath().string().c_str());
    } else {
        LOG_WARN("CS2 not running — VeneeR disabled");
    }

    // ── Main loop ────────────────────────────────────────────────────────────
    app.running = true;
    while (app.running && overlay::IsRunning()) {
        ui::NewFrame();
        RunFrame(app);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    delete app.schema_panel;
    delete app.entity_panel;
    delete app.resource_panel;
    delete app.memory_panel;
    delete app.render_panel;
    delete app.console_panel;
    delete app.skin_changer;
    delete app.skin_panel;
    delete app.skin_tex_cache;
    delete app.obj_inspector;
    delete app.asset_analyzer;

    app.cs2.Close();
    ui::Shutdown();
    CoUninitialize();
    LOG_INFO("Framework shutdown complete");
    return 0;
}

#else // ── DLL MODE (in-process injection) ──────────────────────────────────

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include "MinHook.h"

#include "core/ProcessMemory.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/SkinTextureCache.h"
#include "cs2/ConfigManager.h"
#include "cs2/Offsets.h"
#include "ui/panels/SkinChangerPanel.h"

namespace {

struct App {
    SchemaSystem        schema;
    EntitySystem        entities;
    ResourceSystem      resources;
    RenderSystem        render;
    MaterialSystem      materials;

    PanelManager        panels;
    SchemaBrowserPanel*  schema_panel{};
    EntityBrowserPanel*  entity_panel{};
    ResourceViewerPanel* resource_panel{};
    MemoryInspectorPanel* memory_panel{};
    RenderDebugPanel*     render_panel{};
    ConsolePanel*         console_panel{};

    ObjectInspector*    obj_inspector{};
    AssetAnalyzer*      asset_analyzer{};

    // ── VeneeR ────────────────────────────────────────────────────────────
    ProcessMemory        cs2;
    SkinDatabase         skin_db;
    SkinManager          skin_mgr;
    SkinChanger*         skin_changer{};
    SkinChangerPanel*    skin_panel{};
    SkinTextureCache*    skin_tex_cache{};
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
        if (ImGui::BeginMenu("Panels")) {
            if (ImGui::MenuItem("Schema Browser", nullptr, app.schema_panel && app.schema_panel->IsOpen()))
                if (app.schema_panel) app.schema_panel->SetOpen(!app.schema_panel->IsOpen());
            if (ImGui::MenuItem("Entity Browser", nullptr, app.entity_panel && app.entity_panel->IsOpen()))
                if (app.entity_panel) app.entity_panel->SetOpen(!app.entity_panel->IsOpen());
            if (ImGui::MenuItem("Resource Viewer", nullptr, app.resource_panel && app.resource_panel->IsOpen()))
                if (app.resource_panel) app.resource_panel->SetOpen(!app.resource_panel->IsOpen());
            if (ImGui::MenuItem("Memory Inspector", nullptr, app.memory_panel && app.memory_panel->IsOpen()))
                if (app.memory_panel) app.memory_panel->SetOpen(!app.memory_panel->IsOpen());
            if (ImGui::MenuItem("Render Debug", nullptr, app.render_panel && app.render_panel->IsOpen()))
                if (app.render_panel) app.render_panel->SetOpen(!app.render_panel->IsOpen());
            if (ImGui::MenuItem("Console", nullptr, app.console_panel && app.console_panel->IsOpen()))
                if (app.console_panel) app.console_panel->SetOpen(!app.console_panel->IsOpen());
            if (app.skin_panel) {
                ImGui::Separator();
                if (ImGui::MenuItem("VeneeR", nullptr, app.skin_panel->IsOpen()))
                    app.skin_panel->SetOpen(!app.skin_panel->IsOpen());
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Toggle UI", "Insert")) { ui::ToggleVisibility(); }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (app.schema) {
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.5f);
        if (ImGui::Begin("##status", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav)) {
            ImGui::Text("VeneeR  v1.4.1");
            ImGui::Text("Scopes: %zu  Entities: %u",
                        app.schema.GetTypeScopeCount(),
                        app.entities.GetEntityCount());
        }
        ImGui::End();
    }

    app.panels.DrawAll();
}

// ── Present hook ──────────────────────────────────────────────────────────────
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
PresentFn original_present = nullptr;

// Window proc chain for input forwarding
WNDPROC original_wndproc = nullptr;

LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ui::WndProcHandler(hwnd, msg, wparam, lparam))
        return 1;
    return CallWindowProcW(original_wndproc, hwnd, msg, wparam, lparam);
}

// One-time UI init state
std::atomic<bool> ui_initialised{false};

HRESULT STDMETHODCALLTYPE PresentHook(
    IDXGISwapChain* swapchain, UINT sync_interval, UINT flags)
{
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
    auto& app = State();

    LOG_INFO("VeneeR — DLL injection mode");

    app.schema.Init();
    app.entities.Init();
    app.resources.Init();
    app.render.Init();
    app.materials.Init();

    app.obj_inspector  = new ObjectInspector(&app.schema);
    app.asset_analyzer = new AssetAnalyzer(&app.resources, &app.materials);

    app.schema_panel   = new SchemaBrowserPanel(&app.schema);
    app.entity_panel   = new EntityBrowserPanel(&app.entities, &app.schema);
    app.resource_panel = new ResourceViewerPanel(&app.resources, &app.materials);
    app.memory_panel   = new MemoryInspectorPanel();
    app.render_panel   = new RenderDebugPanel(&app.render);
    app.console_panel  = new ConsolePanel();

    app.panels.Register(app.schema_panel);
    app.panels.Register(app.entity_panel);
    app.panels.Register(app.resource_panel);
    app.panels.Register(app.memory_panel);
    app.panels.Register(app.render_panel);
    app.panels.Register(app.console_panel);

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

        app.skin_changer = new SkinChanger(app.cs2, app.skin_db, app.skin_mgr);
        app.skin_panel   = new SkinChangerPanel(app.skin_db, app.skin_mgr,
                                                 *app.skin_changer);

        app.panels.Register(app.skin_panel);

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
        MH_STATUS s = MH_CreateHook(present_addr, &PresentHook,
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

    // Restore original window proc so the game doesn't crash after DLL unload
    if (original_wndproc && app.game_hwnd && IsWindow(app.game_hwnd)) {
        SetWindowLongPtrW(app.game_hwnd, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(original_wndproc));
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    ui::Shutdown();

    delete app.skin_changer;
    delete app.skin_panel;
    delete app.skin_tex_cache;
    delete app.schema_panel;
    delete app.entity_panel;
    delete app.resource_panel;
    delete app.memory_panel;
    delete app.render_panel;
    delete app.console_panel;
    delete app.obj_inspector;
    delete app.asset_analyzer;

    app.cs2.Close();
    app = {};
}

} // anonymous namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        ShutdownDLL();
    }
    return TRUE;
}

#endif // CSC_BUILD_STANDALONE
