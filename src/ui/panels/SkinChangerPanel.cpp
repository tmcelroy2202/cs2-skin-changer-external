#include "SkinChangerPanel.h"
#include "imgui.h"

#include <algorithm>

void SkinChangerPanel::Draw() noexcept {
    ImGui::SetNextWindowSize(ImVec2(420, 360), ImGuiCond_FirstUseEver);
    ImGui::Begin("CS2 Skin Changer", &open_,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    // ── Sidebar tabs ─────────────────────────────────────────────────────────
    ImGui::BeginChild("##sc_sidebar", ImVec2(110, 0), true);
    if (ImGui::Button("Skins", ImVec2(90, 28))) active_tab_ = Tab::Skins;
    if (ImGui::Button("Gloves", ImVec2(90, 28))) active_tab_ = Tab::Gloves;
    if (ImGui::Button("Music Kit", ImVec2(90, 28))) active_tab_ = Tab::MusicKit;
    ImGui::EndChild();

    ImGui::SameLine();

    // ── Content ──────────────────────────────────────────────────────────────
    ImGui::BeginChild("##sc_content", ImVec2(0, 0), false);

    switch (active_tab_) {
    case Tab::Skins:    DrawSkinsTab(); break;
    case Tab::Gloves:   DrawGlovesTab(); break;
    case Tab::MusicKit: DrawMusicTab(); break;
    }

    ImGui::EndChild();
    ImGui::End();
}

// ── Skins Tab ─────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinsTab() noexcept {
    DrawWeaponCombo();

    if (weapon_list_.empty()) return;
    WeaponsEnum selected_weapon = weapon_list_[selected_weapon_idx_];
    bool is_knife = mgr_.IsKnifeDef(static_cast<i32>(selected_weapon));

    DrawSkinCombo(selected_weapon);

    if (is_knife) {
        WeaponsEnum kdef = mgr_.GetKnifeDef();
        i32 kpaint = mgr_.GetKnifePaint();
        if (kdef != WeaponsEnum::None && kpaint > 0) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Selected: %s | Paint #%d",
                               WeaponIdToString(static_cast<i32>(kdef)).c_str(), kpaint);
        }
    } else {
        auto current = mgr_.GetSkin(selected_weapon);
        if (current.Paint) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active: %s (Paint #%d)",
                               current.name.c_str(), current.Paint);
        }
    }
}

void SkinChangerPanel::DrawWeaponCombo() noexcept {
    if (weapon_list_.empty()) {
        for (auto def : kAllWeaponDefs)
            weapon_list_.push_back(def);
    }

    if (weapon_list_.empty()) return;

    selected_weapon_idx_ = std::clamp<i32>(selected_weapon_idx_, 0,
        static_cast<i32>(weapon_list_.size()) - 1);

    WeaponsEnum current = weapon_list_[selected_weapon_idx_];
    std::string preview = WeaponIdToString(static_cast<i32>(current));

    ImGui::Text("Weapon");
    if (ImGui::BeginCombo("##weapon_sel", preview.c_str())) {
        for (i32 i = 0; i < static_cast<i32>(weapon_list_.size()); ++i) {
            auto def = weapon_list_[i];
            auto name = WeaponIdToString(static_cast<i32>(def));
            bool sel = (i == selected_weapon_idx_);
            if (ImGui::Selectable(name.c_str(), sel)) {
                selected_weapon_idx_ = i;
                selected_skin_idx_ = 0;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void SkinChangerPanel::DrawSkinCombo(WeaponsEnum weapon) noexcept {
    bool is_knife = mgr_.IsKnifeDef(static_cast<i32>(weapon));
    auto skins = db_.GetWeaponSkins(weapon);

    if (is_knife) {
        // For knives, sync with manager's knife selection
        i32 current_def = static_cast<i32>(mgr_.GetKnifeDef());
        i32 current_paint = mgr_.GetKnifePaint();

        // Find matching skin index
        selected_skin_idx_ = 0;
        for (i32 i = 0; i < static_cast<i32>(skins.size()); ++i) {
            if (skins[i].Paint == current_paint) {
                selected_skin_idx_ = i;
                break;
            }
        }
    } else {
        auto current = mgr_.GetSkin(weapon);
        selected_skin_idx_ = mgr_.GetSkinIndex(skins, current);
    }

    selected_skin_idx_ = std::clamp<i32>(selected_skin_idx_, 0,
        static_cast<i32>(skins.size()) - 1);

    if (skins.empty()) {
        ImGui::Text("No skins available");
        return;
    }

    std::string preview = skins[selected_skin_idx_].name;

    ImGui::Text("Skin");
    if (ImGui::BeginCombo("##skin_sel", preview.c_str())) {
        for (i32 i = 0; i < static_cast<i32>(skins.size()); ++i) {
            bool sel = (i == selected_skin_idx_);
            std::string label = skins[i].name;
            if (skins[i].Paint > 0)
                label += " [" + std::to_string(skins[i].Paint) + "]";

            if (ImGui::Selectable(label.c_str(), sel)) {
                selected_skin_idx_ = i;
                if (is_knife) {
                    mgr_.SetKnife(weapon, skins[i].Paint);
                } else {
                    mgr_.SetSkin(skins[i]);
                }
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// ── Gloves Tab ────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawGlovesTab() noexcept {
    DrawGloveTypeCombo();

    if (glove_list_.empty()) return;
    GlovesEnum selected_glove = glove_list_[selected_glove_type_idx_];

    DrawGloveSkinCombo(selected_glove);

    i32 active_paint = mgr_.GetGlovePaint();
    if (active_paint) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active Paint: #%d", active_paint);
    }
}

void SkinChangerPanel::DrawGloveTypeCombo() noexcept {
    if (glove_list_.empty()) {
        for (auto g : kAllGloveDefs)
            glove_list_.push_back(g);
    }

    if (glove_list_.empty()) return;

    selected_glove_type_idx_ = std::clamp<i32>(selected_glove_type_idx_, 0,
        static_cast<i32>(glove_list_.size()) - 1);

    GlovesEnum current = glove_list_[selected_glove_type_idx_];
    std::string preview = GloveIdToString(static_cast<i32>(current));

    ImGui::Text("Glove Type");
    if (ImGui::BeginCombo("##glove_type", preview.c_str())) {
        for (i32 i = 0; i < static_cast<i32>(glove_list_.size()); ++i) {
            auto def = glove_list_[i];
            auto name = GloveIdToString(static_cast<i32>(def));
            bool sel = (i == selected_glove_type_idx_);
            if (ImGui::Selectable(name.c_str(), sel)) {
                selected_glove_type_idx_ = i;
                selected_glove_skin_idx_ = 0;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void SkinChangerPanel::DrawGloveSkinCombo(GlovesEnum glove) noexcept {
    auto skins = db_.GetGloveSkins(glove);

    selected_glove_skin_idx_ = std::clamp<i32>(selected_glove_skin_idx_, 0,
        static_cast<i32>(skins.size()) - 1);

    if (skins.empty()) {
        ImGui::Text("No skins available");
        return;
    }

    std::string preview = skins[selected_glove_skin_idx_].name;

    ImGui::Text("Paint Kit");
    if (ImGui::BeginCombo("##glove_skin", preview.c_str())) {
        for (i32 i = 0; i < static_cast<i32>(skins.size()); ++i) {
            bool sel = (i == selected_glove_skin_idx_);
            std::string label = skins[i].name;
            if (skins[i].Paint > 0)
                label += " [Paint #" + std::to_string(skins[i].Paint) + "]";

            if (ImGui::Selectable(label.c_str(), sel)) {
                selected_glove_skin_idx_ = i;
                // Store the paint kit in SkinManager with a sentinel weapon type
                SkinInfo_t glove_skin{skins[i].Paint, false, "Glove", WeaponsEnum::None};
                mgr_.SetGloveSkin(glove_skin);
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// ── Music Kit Tab ─────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawMusicTab() noexcept {
    struct MusicKitEntry { u16 id; const char* name; };
    static const MusicKitEntry kits[] = {
        {0,   "Default"},
        {1,   "Counter-Strike 2"},
        {2,   "Valve CSGO 2 (hidden)"},
        {3,   "Crimson Assault"},
        {4,   "Sharpened"},
        {5,   "Insurgency"},
        {6,   "A_D_8"},
        {7,   "High Noon"},
        {8,   "Death's Head Demolition"},
        {9,   "Desert Fire"},
        {10,  "LNOE"},
        {11,  "Metal"},
        {12,  "All I Want For Christmas"},
        {13,  "Iso Rhythm"},
        {14,  "For No Mankind"},
        {15,  "Hotline Miami"},
        {16,  "The 8-Bit Kit"},
        {17,  "The Talos Principle"},
        {18,  "Battlepack"},
        {19,  "Molotov"},
        {20,  "Uber Blasto Phone"},
        {21,  "Hazardous Environments"},
        {22,  "Headshot"},
        {23,  "Total Domination"},
        {24,  "I Am"},
        {25,  "Diamonds"},
        {26,  "Invasion!"},
        {27,  "Lion's Mouth"},
        {28,  "Sponge Fingerz"},
        {29,  "Aggressive"},
        {30,  "Java Havana Funkaloo"},
        {31,  "Moments CSGO"},
        {32,  "Disgusting"},
        {33,  "The Good Youth"},
        {34,  "FREE"},
        {35,  "Life's Not Out To Get You"},
        {36,  "Backbone"},
        {37,  "GLA"},
        {38,  "Arena"},
        {39,  "EZ4ENCE"},
        {40,  "Halo"},
        {41,  "King, Scar"},
        {42,  "Anti-Citizen"},
        {43,  "Bachram"},
        {44,  "Taco Truck"},
        {45,  "Eye of the Dragon"},
        {46,  "M.U.D.D. FORCE"},
        {47,  "Neo Noir"},
        {48,  "Bodacious"},
        {49,  "Drifter"},
        {50,  "All For Dust"},
        {51,  "Hades"},
        {52,  "The Lowfile Pack"},
        {53,  "CHAINSAW Lxadxut"},
        {54,  "Mocha Petal"},
        {55,  "Yellow Magic"},
        {56,  "VICI"},
        {57,  "Atro Bellum"},
        {58,  "Work Hard, Play Hard"},
        {59,  "Kolibri"},
        {60,  "U Mad!"},
        {61,  "Flashbang Dance"},
        {62,  "Heading For The Source"},
        {63,  "Void"},
        {64,  "Shooters"},
        {65,  "Dashstar*"},
        {66,  "Gothic Luxury"},
        {67,  "Lock Me Up"},
        {68,  "Hua Lian"},
        {69,  "Ultimate"},
        {76,  "Under Bright Lights"},
    };
    static constexpr i32 kit_count = sizeof(kits) / sizeof(kits[0]);

    u16 current_id = changer_.GetMusicKit();
    selected_music_idx_ = 0;
    for (i32 i = 0; i < kit_count; ++i) {
        if (kits[i].id == current_id) {
            selected_music_idx_ = i;
            break;
        }
    }

    std::string preview = kits[selected_music_idx_].name;

    ImGui::Text("Select Music Kit");
    if (ImGui::BeginCombo("##musickit", preview.c_str())) {
        for (i32 i = 0; i < kit_count; ++i) {
            bool sel = (i == selected_music_idx_);
            if (ImGui::Selectable(kits[i].name, sel)) {
                selected_music_idx_ = i;
                changer_.SetMusicKit(kits[i].id);
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "Music Kit ID: %u",
                       kits[selected_music_idx_].id);
}
