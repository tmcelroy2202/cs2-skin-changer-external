#pragma once

#include "ui/Panels.h"
#include "core/ProcessMemory.h"
#include "cs2/Offsets.h"
#include "cs2/EntitySystem.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <chrono>

/* ── DebugPanel ────────────────────────────────────────────────────────────
 *
 * Real-time diagnostic panel for debugging the skin changer.
 * Shows entity resolution, weapon discovery, write verification, and errors.
 *
 * Press F2 in the overlay to open.
 * ──────────────────────────────────────────────────────────────────────────── */

class DebugPanel final : public Panel {
public:
    explicit DebugPanel(ProcessMemory& mem) noexcept
        : Panel("VeneeR Debug"), mem_(mem) {}

    std::string_view GetId() const noexcept override { return "DebugPanel"; }
    void Draw() noexcept override;

    /// Report an error string from the skin changer.
    void ReportError(std::string_view err) noexcept;

    /// Set write verification results.
    void SetWriteResult(u64 addr, std::string_view field, u64 expected, u64 actual) noexcept;

    /// Clear accumulated state (on round restart, etc.).
    void Clear() noexcept;

private:
    ProcessMemory& mem_;

    // Error log
    static constexpr size_t kMaxErrors = 100;
    std::vector<std::string> errors_;

    // Write verification
    struct WriteCheck {
        u64 addr;
        std::string field;
        u64 expected;
        u64 actual;
    };
    std::vector<WriteCheck> write_checks_;

    // Diagnostics cache (refreshed each frame)
    void DrawGeneralDiagnostics() noexcept;
    void DrawWeaponDiagnostics() noexcept;
    void DrawGloveDiagnostics() noexcept;
    void DrawWriteVerification() noexcept;
    void DrawErrors() noexcept;
    void DrawEntityListScan() noexcept;
};

inline void DebugPanel::ReportError(std::string_view err) noexcept {
    if (errors_.size() >= kMaxErrors) errors_.erase(errors_.begin());
    errors_.push_back(std::string(err));
}

inline void DebugPanel::SetWriteResult(u64 addr, std::string_view field, u64 expected, u64 actual) noexcept {
    write_checks_.push_back({addr, std::string(field), expected, actual});
}

inline void DebugPanel::Clear() noexcept {
    errors_.clear();
    write_checks_.clear();
}

inline void DebugPanel::Draw() noexcept {
    if (ImGui::BeginTabBar("DebugTabs")) {
        if (ImGui::BeginTabItem("General")) {
            DrawGeneralDiagnostics();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Weapons")) {
            DrawWeaponDiagnostics();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Gloves")) {
            DrawGloveDiagnostics();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Entity Scan")) {
            DrawEntityListScan();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Write Verify")) {
            DrawWriteVerification();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Errors")) {
            DrawErrors();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

inline void DebugPanel::DrawGeneralDiagnostics() noexcept {
    ImGui::Text("Process Memory: %s", mem_ ? "Connected" : "Not connected");
    ImGui::Separator();

    u64 client = mem_.Client();
    ImGui::Text("client.dll base: 0x%llX", client);

    auto entity_list = mem_.ReadSafe<u64>(client + Offsets::dwEntityList);
    ImGui::Text("dwEntityList:    0x%llX", client + Offsets::dwEntityList);
    ImGui::Text("  → Entity List:     0x%llX", entity_list.value_or(0));

    auto local_pawn = mem_.ReadSafe<u64>(client + Offsets::dwLocalPlayerPawn);
    ImGui::Text("dwLocalPlayerPawn: 0x%llX", client + Offsets::dwLocalPlayerPawn);
    ImGui::Text("  → Local Pawn:      0x%llX", local_pawn.value_or(0));

    auto local_ctrl = mem_.ReadSafe<u64>(client + Offsets::dwLocalPlayerController);
    ImGui::Text("dwLocalPlayerCtrl: 0x%llX", client + Offsets::dwLocalPlayerController);
    ImGui::Text("  → Local Ctrl:      0x%llX", local_ctrl.value_or(0));

    if (local_pawn && *local_pawn) {
        auto team = mem_.ReadSafe<u8>(*local_pawn + Offsets::m_iTeamNum);
        ImGui::Text("Team: %d (%s)", team.value_or(0),
                    team.value_or(0) == 3 ? "CT" :
                    team.value_or(0) == 2 ? "T" : "Other");

        auto ws = mem_.ReadSafe<u64>(*local_pawn + Offsets::m_pWeaponServices);
        ImGui::Text("WeaponServices: 0x%llX", ws.value_or(0));

        if (ws && *ws) {
            auto active_h = mem_.ReadSafe<u32>(*ws + Offsets::m_hActiveWeapon);
            ImGui::Text("m_hActiveWeapon: 0x%08X", active_h.value_or(0));

            auto vec_size0 = mem_.ReadSafe<u32>(*ws + Offsets::m_hMyWeapons);
            auto vec_ptr0  = mem_.ReadSafe<u64>(*ws + Offsets::m_hMyWeapons + 0x08);
            auto vec_size1 = mem_.ReadSafe<u32>(*ws + Offsets::m_hMyWeapons + 0x10);

            ImGui::Text("m_hMyWeapons (+0x00 count): %u", vec_size0.value_or(0));
            ImGui::Text("m_hMyWeapons (+0x08 ptr):   0x%llX", vec_ptr0.value_or(0));
            ImGui::Text("m_hMyWeapons (+0x10 count): %u", vec_size1.value_or(0));
        }

        // Read-back: try active weapon fallback
        u64 active = GetActiveWeapon(mem_, *local_pawn);
        ImGui::Text("GetActiveWeapon → 0x%llX", active);
        if (active) {
            u64 item = active + Offsets::m_AttributeManager + Offsets::m_Item;
            ImGui::Text("  item view → 0x%llX", item);
            auto def = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
            ImGui::Text("  m_iItemDefinitionIndex: %d", def.value_or(0));
        }
    }

    if (local_ctrl && *local_ctrl) {
        auto inv = mem_.ReadSafe<u64>(*local_ctrl + Offsets::m_pInventoryServices);
        ImGui::Text("InventoryServices: 0x%llX", inv.value_or(0));
    }
}

inline void DebugPanel::DrawWeaponDiagnostics() noexcept {
    auto local_pawn = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_pawn || !*local_pawn) {
        ImGui::TextColored(ImVec4(1,0,0,1), "No local player pawn");
        return;
    }

    auto weapons = GetWeapons(mem_, *local_pawn);
    ImGui::Text("Weapons found: %zu", weapons.size());

    if (weapons.empty()) {
        // Try active weapon fallback
        u64 active = GetActiveWeapon(mem_, *local_pawn);
        if (active) {
            ImGui::TextColored(ImVec4(1,1,0,1), "Active weapon fallback: 0x%llX", active);
            weapons.push_back(active);
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "Active weapon fallback also failed");
        }
    }

    ImGui::Separator();
    for (size_t i = 0; i < weapons.size(); ++i) {
        u64 w = weapons[i];
        ImGui::Text("[%zu] Weapon: 0x%llX", i, w);

        u64 item = w + Offsets::m_AttributeManager + Offsets::m_Item;
        ImGui::Text("     item view: 0x%llX", item);

        if (item && item != ~0ull) {
            auto def = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
            auto paint = mem_.ReadSafe<u32>(w + Offsets::m_nFallbackPaintKit);
            auto wear = mem_.ReadSafe<f32>(w + Offsets::m_flFallbackWear);
            auto seed = mem_.ReadSafe<u32>(w + Offsets::m_nFallbackSeed);
            auto stattrak = mem_.ReadSafe<u32>(w + Offsets::m_nFallbackStatTrak);
            auto iid_high = mem_.ReadSafe<u32>(item + Offsets::m_iItemIDHigh);
            auto iid_low = mem_.ReadSafe<u32>(item + Offsets::m_iItemIDLow);
            auto iid = mem_.ReadSafe<u64>(item + Offsets::m_iItemID);
            auto aid = mem_.ReadSafe<u32>(item + Offsets::m_iAccountID);

            ImGui::Text("     Def: %d | Paint: %u | Wear: %.4f | Seed: %u",
                        def.value_or(-1), paint.value_or(0), wear.value_or(-1.0f), seed.value_or(0));
            ImGui::Text("     StatTrak: %u | ItemID: 0x%llX | High: 0x%08X | Low: 0x%08X",
                        stattrak.value_or(0xFFFFFFFF), iid.value_or(0),
                        iid_high.value_or(0), iid_low.value_or(0));
            ImGui::Text("     AccountID: %u", aid.value_or(0));

            // Range check
            if (def && *def > 0 && *def < 2000) {
                ImGui::TextColored(ImVec4(0,1,0,1), "     → Looks valid");
            } else {
                ImGui::TextColored(ImVec4(1,0,0,1), "     → Def out of range (invalid)");
            }
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "     m_Item invalid (~0 or null)");
        }
        ImGui::Separator();
    }
}

inline void DebugPanel::DrawGloveDiagnostics() noexcept {
    auto local_pawn = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_pawn || !*local_pawn) {
        ImGui::TextColored(ImVec4(1,0,0,1), "No local player pawn");
        return;
    }

    u64 glove_iv = *local_pawn + Offsets::m_EconGloves;
    ImGui::Text("Embedded Glove ItemView: 0x%llX", glove_iv);

    // Read glove entity
    u64 glove_entity = FindGloveEntity(mem_, *local_pawn);
    ImGui::Text("Glove Entity (from list scan): 0x%llX", glove_entity);

    if (glove_entity) {
        auto paint = mem_.ReadSafe<u32>(glove_entity + Offsets::m_nFallbackPaintKit);
        auto wear = mem_.ReadSafe<f32>(glove_entity + Offsets::m_flFallbackWear);
        ImGui::Text("  Paint: %u  Wear: %.4f", paint.value_or(0), wear.value_or(0));
    }

    // Read item view fields
    auto iid = mem_.ReadSafe<u64>(glove_iv + Offsets::m_iItemID);
    auto iid_high = mem_.ReadSafe<u32>(glove_iv + Offsets::m_iItemIDHigh);
    auto iid_low = mem_.ReadSafe<u32>(glove_iv + Offsets::m_iItemIDLow);
    auto aid = mem_.ReadSafe<u32>(glove_iv + Offsets::m_iAccountID);
    auto init = mem_.ReadSafe<bool>(glove_iv + Offsets::m_bInitialized);
    auto restore = mem_.ReadSafe<bool>(glove_iv + Offsets::m_bRestoreCustomMaterialAfterPrecache);

    ImGui::Text("ItemView fields:");
    ImGui::Text("  m_iItemID:  0x%llX", iid.value_or(0));
    ImGui::Text("  m_iItemIDHigh: 0x%08X", iid_high.value_or(0));
    ImGui::Text("  m_iItemIDLow:  0x%08X", iid_low.value_or(0));
    ImGui::Text("  m_iAccountID:  %u", aid.value_or(0));
    ImGui::Text("  m_bInitialized: %s", init.value_or(false) ? "true" : "false");
    ImGui::Text("  m_bRestoreCustom: %s", restore.value_or(false) ? "true" : "false");
}

inline void DebugPanel::DrawEntityListScan() noexcept {
    auto entity_list = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwEntityList);
    if (!entity_list || !*entity_list) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Entity list pointer null");
        return;
    }

    // Scan first 3 chunks
    for (u32 chunk = 0; chunk < 3; ++chunk) {
        auto list_entry = mem_.ReadSafe<u64>(*entity_list + sizeof(u64) * chunk + 0x10);
        if (!list_entry || !*list_entry) continue;

        u32 found = 0;
        for (u32 slot = 0; slot < kChunkSize; ++slot) {
            u64 identity = *list_entry + kEntityIdentitySize * slot;
            auto entity = mem_.ReadSafe<u64>(identity);
            if (!entity || !*entity) continue;
            if (++found > 10) break;  // cap display per chunk
        }
        ImGui::Text("Chunk %u: %u entities found (showing first 10)", chunk, found);

        // Reset and actually show details
        found = 0;
        for (u32 slot = 0; slot < kChunkSize; ++slot) {
            u64 identity = *list_entry + kEntityIdentitySize * slot;
            auto entity = mem_.ReadSafe<u64>(identity);
            if (!entity || !*entity) continue;
            if (++found > 10) break;

            u64 ent = *entity;
            // m_AttributeManager and m_Item are EMBEDDED structs (not pointers).
            u64 item = ent + Offsets::m_AttributeManager + Offsets::m_Item;

            auto def = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
            ImGui::Text("  [slot %3u] ent=0x%llX def=%d%s",
                        slot, ent, def.value_or(-1),
                        (def && *def > 0 && *def < 2000) ? " (weapon?)" : "");
        }
    }
}

inline void DebugPanel::DrawWriteVerification() noexcept {
    if (write_checks_.empty()) {
        ImGui::Text("No write checks recorded yet.");
        ImGui::Text("Write verification runs after each Tick()");
        ImGui::Text("when a weapon is processed.");
        return;
    }

    ImGui::BeginChild("WriteChecks", ImVec2(0, 0), true);
    for (auto& c : write_checks_) {
        bool ok = (c.expected == c.actual);
        ImGui::Text("0x%llX %s: expected=0x%llX actual=0x%llX %s",
                    c.addr, c.field.c_str(), c.expected, c.actual,
                    ok ? "✓" : "✗ MISMATCH");
    }
    ImGui::EndChild();

    if (ImGui::Button("Clear")) write_checks_.clear();
}

inline void DebugPanel::DrawErrors() noexcept {
    if (errors_.empty()) {
        ImGui::Text("No errors reported.");
        return;
    }

    ImGui::BeginChild("Errors", ImVec2(0, 0), true);
    for (auto& e : errors_) {
        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s", e.c_str());
    }
    ImGui::EndChild();

    if (ImGui::Button("Clear Errors")) errors_.clear();
}
