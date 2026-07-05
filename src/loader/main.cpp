/* ── VeneeR — Standalone Overlay ───────────────────────────────────────────
 *
 * External skin changer for CS2. Creates its own transparent overlay window,
 * renders Dear ImGui UI, and reads/writes CS2 memory via RPM/WPM.
 * No DLL injection, no MinHook, no process manipulation beyond OpenProcess.
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
#include "loader/Console.h"

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

// ── Find CS2 process ─────────────────────────────────────────────────────
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

// ── Render UI ────────────────────────────────────────────────────────────
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
                    LOG_WARN("Offsets update failed — check internet");
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

// ── Init skin changer ────────────────────────────────────────────────────
bool InitSkinChanger(App& app) {
    LOG_INFO("VeneeR — external overlay mode (v1.8.2)");

    SetBasePath(std::filesystem::current_path());

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

    app.initialised = true;
    LOG_INFO("Skin changer initialised");
    return true;
}

} // anonymous namespace

// ── Entry point ──────────────────────────────────────────────────────────
int main() {
    con::Init();
    con::Banner();

    auto& app = State();

    // Find CS2
    con::Info("Looking for CS2...");
    for (int i = 0; i < 120; ++i) {
        app.cs2_pid = FindCS2();
        if (app.cs2_pid) break;
        Sleep(1000);
    }

    if (!app.cs2_pid) {
        con::Error("CS2 not found. Make sure cs2.exe is running.");
        con::WaitForExit();
        return 1;
    }

    con::Success(("CS2 found (PID: " + std::to_string(app.cs2_pid) + ")").c_str());

    // Open CS2 process for external memory access
    if (!app.cs2.Open(app.cs2_pid)) {
        con::Error("Failed to open CS2 process. Run as Administrator.");
        con::WaitForExit();
        return 1;
    }

    con::Success("Process opened successfully");

    // Init UI as standalone overlay
    if (!ui::Init({.overlay_mode = true})) {
        con::Error("Failed to create overlay. Is a compatible GPU present?");
        con::WaitForExit();
        return 1;
    }

    con::Success("Overlay created — press Insert to toggle");

    // Init skin changer
    InitSkinChanger(app);

    con::Success("VeneeR v1.8.2 is ready!");
    con::Info("Press Insert in-game to open the overlay.");
    con::Info("Press Shift+Esc in the overlay to exit the tool.");

    // ── Main loop ────────────────────────────────────────────────────────
    static bool prev_insert = false;
    bool running = true;

    while (running) {
        // Windows message loop
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running) break;

        // Check if CS2 is still running
        DWORD exit_code = 0;
        if (!GetExitCodeProcess(app.cs2.NativeHandle(), &exit_code) ||
            exit_code != STILL_ACTIVE) {
            con::Warning("CS2 has exited. Closing...");
            break;
        }

        // Poll Insert key (edge-triggered)
        bool cur_insert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (cur_insert && !prev_insert)
            ui::ToggleVisibility();
        prev_insert = cur_insert;

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

        // Render UI
        ui::NewFrame();
        RenderUI(app);
        ui::Render();

        // Cap at ~60 FPS to keep CPU usage low
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

    con::Success("VeneeR has been shut down cleanly.");
    return 0;
}
