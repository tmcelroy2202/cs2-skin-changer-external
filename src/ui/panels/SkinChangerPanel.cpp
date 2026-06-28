#include "SkinChangerPanel.h"
#include "imgui.h"

#include <algorithm>

// ── CS2 wear condition thresholds & colors ───────────────────────────────────
static const char* WearLabel(f32 wear) {
    if (wear < 0.07f) return "Factory New";
    if (wear < 0.15f) return "Minimal Wear";
    if (wear < 0.38f) return "Field-Tested";
    if (wear < 0.45f) return "Well-Worn";
    return "Battle-Scarred";
}

static ImVec4 WearColor(f32 wear) {
    if (wear < 0.07f) return ImVec4(0.30f, 0.85f, 0.30f, 1.0f);
    if (wear < 0.15f) return ImVec4(0.20f, 0.60f, 1.00f, 1.0f);
    if (wear < 0.38f) return ImVec4(1.00f, 0.85f, 0.00f, 1.0f);
    if (wear < 0.45f) return ImVec4(1.00f, 0.55f, 0.00f, 1.0f);
    return ImVec4(1.00f, 0.20f, 0.20f, 1.0f);
}

static void DrawWearGradient() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    f32 w = ImGui::GetContentRegionAvail().x;
    f32 h = 6.0f;
    dl->AddRectFilledMultiColor(
        ImVec2(p.x, p.y), ImVec2(p.x + w * 0.07f, p.y + h),
        ImColor(0.30f, 0.85f, 0.30f), ImColor(0.30f, 0.85f, 0.30f),
        ImColor(0.30f, 0.85f, 0.30f), ImColor(0.30f, 0.85f, 0.30f));
    dl->AddRectFilledMultiColor(
        ImVec2(p.x + w * 0.07f, p.y), ImVec2(p.x + w * 0.15f, p.y + h),
        ImColor(0.30f, 0.85f, 0.30f), ImColor(0.20f, 0.60f, 1.00f),
        ImColor(0.20f, 0.60f, 1.00f), ImColor(0.30f, 0.85f, 0.30f));
    dl->AddRectFilledMultiColor(
        ImVec2(p.x + w * 0.15f, p.y), ImVec2(p.x + w * 0.38f, p.y + h),
        ImColor(0.20f, 0.60f, 1.00f), ImColor(1.00f, 0.85f, 0.00f),
        ImColor(1.00f, 0.85f, 0.00f), ImColor(0.20f, 0.60f, 1.00f));
    dl->AddRectFilledMultiColor(
        ImVec2(p.x + w * 0.38f, p.y), ImVec2(p.x + w * 0.45f, p.y + h),
        ImColor(1.00f, 0.85f, 0.00f), ImColor(1.00f, 0.55f, 0.00f),
        ImColor(1.00f, 0.55f, 0.00f), ImColor(1.00f, 0.85f, 0.00f));
    dl->AddRectFilledMultiColor(
        ImVec2(p.x + w * 0.45f, p.y), ImVec2(p.x + w, p.y + h),
        ImColor(1.00f, 0.55f, 0.00f), ImColor(1.00f, 0.20f, 0.20f),
        ImColor(1.00f, 0.20f, 0.20f), ImColor(1.00f, 0.55f, 0.00f));
    ImGui::Dummy(ImVec2(0, h + 4));
}

void SkinChangerPanel::Draw() noexcept {
    ImGui::SetNextWindowSize(ImVec2(460, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("CS2 Skin Changer", &open_,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    // ── Sidebar tabs ─────────────────────────────────────────────────────────
    ImGui::BeginChild("##sc_sidebar", ImVec2(110, 0), true);

    const char* tab_labels[] = { "Skins", "Gloves", "Music Kit" };
    Tab tab_ids[] = { Tab::Skins, Tab::Gloves, Tab::MusicKit };
    for (i32 i = 0; i < 3; ++i) {
        ImGui::PushStyleColor(ImGuiCol_Button,
            active_tab_ == tab_ids[i]
                ? ImVec4(0.92f, 0.35f, 0.14f, 0.70f)
                : ImVec4(0.13f, 0.13f, 0.19f, 0.54f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImVec4(0.92f, 0.35f, 0.14f, 0.50f));
        if (ImGui::Button(tab_labels[i], ImVec2(92, 32))) {
            active_tab_ = tab_ids[i];
        }
        ImGui::PopStyleColor(2);
    }

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
    // Currently equipped weapon display
    if (!equipped_weapon_.empty()) {
        ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.14f, 1), "Equipped:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.30f, 1), "%s",
                           equipped_weapon_.c_str());
        ImGui::Spacing();
    }

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Weapon Selection");
    ImGui::PopStyleColor();

    DrawWeaponCombo();
    ImGui::Spacing();

    if (weapon_list_.empty()) return;
    WeaponsEnum selected_weapon = weapon_list_[selected_weapon_idx_];
    bool is_knife = mgr_.IsKnifeDef(static_cast<i32>(selected_weapon));

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Skin");
    ImGui::PopStyleColor();

    DrawSkinCombo(selected_weapon);
    ImGui::Spacing();

    if (is_knife) {
        WeaponsEnum kdef = mgr_.GetKnifeDef();
        i32 kpaint = mgr_.GetKnifePaint();
        if (kdef != WeaponsEnum::None && kpaint > 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.14f, 1), "Selected Knife:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s",
                               WeaponIdToString(static_cast<i32>(kdef)).c_str());
            ImGui::Text("Paint Kit #%d", kpaint);
        }
    } else {
        auto current = mgr_.GetSkin(selected_weapon);
        if (current.Paint) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.14f, 1), "Active Skin:");
            ImGui::SameLine();
            ImGui::TextColored(WearColor(current.Wear), "%s",
                               current.name.c_str());
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "Paint #%d  |  %s (%.4f)",
                               current.Paint, WearLabel(current.Wear), current.Wear);
        }

        ImGui::Spacing();
        if (equipped_weapon_id_ > 0 &&
            equipped_weapon_id_ != static_cast<i32>(selected_weapon)) {
            ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f),
                "Tip: Apply to your equipped weapon for instant results.");
            if (ImGui::Button("Apply Selected Skin to Equipped Weapon", ImVec2(-1, 0))) {
                auto skins = db_.GetWeaponSkins(selected_weapon);
                if (!skins.empty() &&
                    selected_skin_idx_ >= 0 &&
                    selected_skin_idx_ < static_cast<i32>(skins.size())) {
                    auto skin = skins[selected_skin_idx_];
                    skin.weaponType = static_cast<WeaponsEnum>(equipped_weapon_id_);
                    auto cur = mgr_.GetSkin(skin.weaponType);
                    if (cur.Paint) {
                        skin.Wear    = cur.Wear;
                        skin.Pattern = cur.Pattern;
                    }
                    mgr_.SetSkin(skin);
                    SaveAndSync();
                }
            }
        }
        ImGui::Spacing();
        DrawFloatPatternControls();
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, 300.0f));
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, 300.0f));
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
                    auto cur = mgr_.GetSkin(weapon);
                    skins[i].Wear    = cur.Paint ? cur.Wear : 0.01f;
                    skins[i].Pattern = cur.Paint ? cur.Pattern : 0;
                    if (skins[i].Paint == 0) {
                        SkinInfo_t cleared{};
                        cleared.weaponType = weapon;
                        mgr_.SetSkin(cleared);
                    } else {
                        mgr_.SetSkin(skins[i]);
                    }
                }
                SaveAndSync();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// ── Gloves Tab ────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawGlovesTab() noexcept {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Glove Type");
    ImGui::PopStyleColor();

    DrawGloveTypeCombo();
    ImGui::Spacing();

    if (glove_list_.empty()) return;
    GlovesEnum selected_glove = glove_list_[selected_glove_type_idx_];

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Paint Kit");
    ImGui::PopStyleColor();

    DrawGloveSkinCombo(selected_glove);

    i32 active_paint = mgr_.GetGlovePaint();
    if (active_paint) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.14f, 1), "Active Paint:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "#%d", active_paint);
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, 300.0f));
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, 300.0f));
    if (ImGui::BeginCombo("##glove_skin", preview.c_str())) {
        for (i32 i = 0; i < static_cast<i32>(skins.size()); ++i) {
            bool sel = (i == selected_glove_skin_idx_);
            std::string label = skins[i].name;
            if (skins[i].Paint > 0)
                label += " [Paint #" + std::to_string(skins[i].Paint) + "]";

            if (ImGui::Selectable(label.c_str(), sel)) {
                selected_glove_skin_idx_ = i;
                // Store the paint kit in SkinManager with a sentinel weapon type
                SkinInfo_t glove_skin{skins[i].Paint, 0.01f, 0, false, "Glove", WeaponsEnum::None};
                mgr_.SetGloveSkin(glove_skin);
                SaveAndSync();
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

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Select Music Kit");
    ImGui::PopStyleColor();

    ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1), ImVec2(-1, 300.0f));
    if (ImGui::BeginCombo("##musickit", preview.c_str())) {
        for (i32 i = 0; i < kit_count; ++i) {
            bool sel = (i == selected_music_idx_);
            if (ImGui::Selectable(kits[i].name, sel)) {
                selected_music_idx_ = i;
                changer_.SetMusicKit(kits[i].id);
                SaveAndSync();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "Currently selected:  %s  (ID: %u)",
                       kits[selected_music_idx_].name,
                       kits[selected_music_idx_].id);
}

// ── Float & Pattern Controls ──────────────────────────────────────────────────

void SkinChangerPanel::DrawFloatPatternControls() noexcept {
    if (weapon_list_.empty()) return;
    WeaponsEnum weapon = weapon_list_[selected_weapon_idx_];
    auto skin = mgr_.GetSkin(weapon);
    if (!skin.Paint) return;

    bool changed = false;

    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.92f, 0.35f, 0.14f, 0.30f));
    ImGui::SeparatorText("Wear & Pattern");
    ImGui::PopStyleColor();

    // ── Wear gradient bar ────────────────────────────────────────────────────
    DrawWearGradient();

    // ── Wear slider ──────────────────────────────────────────────────────────
    f32 wear = skin.Wear;
    ImGui::PushStyleColor(ImGuiCol_Text, WearColor(wear));
    if (ImGui::SliderFloat("##wear", &wear, 0.0f, 1.0f, "%.4f")) {
        skin.Wear = wear;
        changed = true;
    }
    ImGui::PopStyleColor();

    // Wear condition label
    ImVec4 cond_color = WearColor(wear);
    ImGui::SameLine();
    ImGui::TextColored(cond_color, "%s", WearLabel(wear));

    ImGui::Spacing();

    // ── Pattern seed ─────────────────────────────────────────────────────────
    i32 pattern = skin.Pattern;
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::InputInt("##pattern", &pattern, 1, 100)) {
        skin.Pattern = std::clamp(pattern, 0, 9999);
        changed = true;
    }
    ImGui::SameLine();
    ImGui::Text("Pattern Seed");

    if (changed) {
        mgr_.SetSkin(skin);
        SaveAndSync();
    }
}

// ── Config Persistence ─────────────────────────────────────────────────────────

void SkinChangerPanel::SaveAndSync() noexcept {
    SaveConfig(mgr_, changer_.GetMusicKit());
}
