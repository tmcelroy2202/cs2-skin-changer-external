/* ── VeneeR — Standalone Overlay ───────────────────────────────────────────
 *
 * External skin changer for CS2. Creates its own transparent overlay window,
 * renders Dear ImGui UI, and reads/writes CS2 memory via RPM/WPM.
 * ──────────────────────────────────────────────────────────────────────────── */

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "core/Core.h"
#include "core/Logging.h"
#include "core/ProcessMemory.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/SkinTextureCache.h"
#include "cs2/ConfigManager.h"
#include "cs2/Offsets.h"
#include "cs2/EntitySystem.h"
#include "ui/Framework.h"
#include "ui/Panels.h"
#include "ui/panels/SkinChangerPanel.h"
#include "ui/panels/DebugPanel.h"
#include "ui/panels/ConsolePanel.h"

using namespace std::chrono_literals;

namespace {

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

    std::atomic<bool>    initialised{false};
    DWORD                cs2_pid{};
};

App& State() {
    static App a;
    return a;
}

DWORD FindCS2() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, "cs2.exe") == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
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
                    LOG_WARN("Offsets update failed");
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
        ImGui::Text("CS2 PID: %lu", app.cs2_pid);
    }
    ImGui::End();

    app.panels.DrawAll();

    if (app.skin_panel && app.skin_panel->IsOpen())
        app.skin_panel->Draw();
}

bool InitSkinChanger(App& app) {
    LOG_INFO("VeneeR — external overlay mode (v1.8.2)");

    SetBasePath(std::filesystem::current_path());

    Offsets::UpdateFromRemote();

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

    app.initialised = true;
    LOG_INFO("Skin changer initialised");
    return true;
}

} // anonymous namespace

int main() {
    auto& app = State();

    // Find CS2
    for (int i = 0; i < 120; ++i) {
        app.cs2_pid = FindCS2();
        if (app.cs2_pid) break;
        Sleep(1000);
    }

    if (!app.cs2_pid) {
        MessageBoxA(nullptr,
            "CS2 not found. Launch Counter-Strike 2 and try again.",
            "VeneeR", MB_ICONERROR);
        return 1;
    }

    // Open CS2 process for external memory access
    if (!app.cs2.Open(app.cs2_pid)) {
        MessageBoxA(nullptr,
            "Failed to open CS2 process.\n\n"
            "Make sure you run this tool as Administrator.",
            "VeneeR", MB_ICONERROR);
        return 1;
    }

    // Init UI as standalone overlay
    if (!ui::Init({.overlay_mode = true})) {
        MessageBoxA(nullptr,
            "Failed to create overlay.\n\n"
            "Is a DirectX 11 compatible GPU present?",
            "VeneeR", MB_ICONERROR);
        return 1;
    }

    // Init skin changer
    InitSkinChanger(app);

    // ── Main loop ────────────────────────────────────────────────────────
    while (true) {
        // Tick skin changer (RPM/WPM external writes)
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

        // Render UI (handles Insert key + message pumping internally)
        ui::NewFrame();
        RenderUI(app);
        ui::Render();

        // Check if CS2 is still running
        if (!app.cs2.IsAlive())
            break;

        // Cap at ~60 FPS
        std::this_thread::sleep_for(16ms);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────
    ui::Shutdown();
    Logger::Instance().SetCallback(nullptr);
    delete app.skin_changer;
    delete app.skin_panel;
    delete app.debug_panel;
    delete app.console_panel;
    delete app.skin_tex_cache;
    app.cs2.Close();

    return 0;
}
