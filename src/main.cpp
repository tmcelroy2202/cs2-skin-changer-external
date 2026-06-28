/* ── CS2 Skin Changer — Entry Point ──────────────────────────────────────────
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
#include "cs2/Offsets.h"
#include "cs2/ConfigManager.h"
#include "ui/panels/SkinChangerPanel.h"
// ──────────────────────────────────────────────────────────────────────────────
//  STANDALONE OVERLAY MODE
// ──────────────────────────────────────────────────────────────────────────────

#include <thread>

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

    // ── CS2 Skin Changer ──────────────────────────────────────────────────
    ProcessMemory        cs2;
    SkinDatabase         skin_db;
    SkinManager          skin_mgr;
    SkinChanger*         skin_changer{};
    SkinChangerPanel*    skin_panel{};

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
                    if (ImGui::MenuItem("CS2 Skin Changer", nullptr, app.skin_panel->IsOpen()))
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
                ImGui::Text("CS2 Skin Changer  v1.0");
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

    ui::Render();
}

} // anonymous namespace

int main() {
    LOG_INFO("CS2 Skin Changer — Standalone Overlay Mode");

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

    // ── CS2 Skin Changer ────────────────────────────────────────────────────
    if (app.cs2.Open(L"cs2.exe")) {
        LOG_INFO("Connected to cs2.exe");

        // Load offsets from data file (if present); otherwise embedded defaults apply
        auto off_path = Offsets::DataFilePath();
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
        app.panels.Register(app.skin_panel);

        // Load saved config (skins, knife, gloves, music kit)
        u16 saved_music = app.skin_changer->GetMusicKit();
        LoadConfig(app.skin_mgr, saved_music);
        app.skin_changer->SetMusicKit(saved_music);
        LOG_INFO("Config loaded from {}", ConfigPath().string().c_str());
    } else {
        LOG_WARN("CS2 not running — skin changer disabled");
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
    delete app.obj_inspector;
    delete app.asset_analyzer;

    app.cs2.Close();
    ui::Shutdown();
    LOG_INFO("Framework shutdown complete");
    return 0;
}

#else // ── DLL MODE (in-process injection) ──────────────────────────────────

#include <Windows.h>
#include <d3d11.h>

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

    bool initialised = false;
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
            ImGui::Text("CS2 Skin Changer  v1.0");
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

HRESULT STDMETHODCALLTYPE PresentHook(
    IDXGISwapChain* swapchain, UINT sync_interval, UINT flags)
{
    auto& app = State();
    if (app.initialised) {
        ui::NewFrame();
        RenderUI(app);
        ui::Render();
    }
    return original_present(swapchain, sync_interval, flags);
}

DWORD WINAPI InitThread(LPVOID) noexcept {
    auto& app = State();

    LOG_INFO("CS2 Skin Changer — DLL injection mode");

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

    // Hook Present — real implementation would:
    //   1. Locate the swap chain in the target process
    //   2. Copy the vtable
    //   3. Replace Present with PresentHook
    //   (placeholder for demo)

    app.initialised = true;
    LOG_INFO("DLL initialised successfully");
    return 0;
}

void ShutdownDLL() noexcept {
    auto& app = State();
    if (!app.initialised) return;
    ui::Shutdown();
    delete app.schema_panel;
    delete app.entity_panel;
    delete app.resource_panel;
    delete app.memory_panel;
    delete app.render_panel;
    delete app.console_panel;
    delete app.obj_inspector;
    delete app.asset_analyzer;
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
