#include "SkinChangerPanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>
#include "cs2/WeaponTypes.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

// ── Palette ────────────────────────────────────────────────────────────────────

namespace {

struct Palette {
    inline static const ImVec4 Bg          = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
    inline static const ImVec4 Sidebar     = ImVec4(0.08f, 0.08f, 0.13f, 1.00f);
    inline static const ImVec4 Card        = ImVec4(0.12f, 0.12f, 0.19f, 1.00f);
    inline static const ImVec4 CardHover   = ImVec4(0.17f, 0.17f, 0.26f, 1.00f);
    inline static const ImVec4 Accent      = ImVec4(0.10f, 0.70f, 0.80f, 1.00f);
    inline static const ImVec4 AccentDim   = ImVec4(0.08f, 0.50f, 0.60f, 1.00f);
    inline static const ImVec4 AccentGlow  = ImVec4(0.10f, 0.70f, 0.80f, 0.15f);
    inline static const ImVec4 Gold        = ImVec4(0.96f, 0.62f, 0.04f, 1.00f);
    inline static const ImVec4 Text        = ImVec4(0.92f, 0.92f, 0.96f, 1.00f);
    inline static const ImVec4 TextDim     = ImVec4(0.50f, 0.50f, 0.60f, 1.00f);
    inline static const ImVec4 TextGood    = ImVec4(0.20f, 0.85f, 0.55f, 1.00f);
    inline static const ImVec4 SideActive  = ImVec4(0.10f, 0.70f, 0.80f, 0.12f);
};

const char* SideLabel(int i) {
    static const char* labels[] = {
        "Pistols", "Rifles", "SMGs", "Snipers",
        "Heavy", "Knives", "Gloves", "Music"
    };
    return labels[i];
}

const char* WearLabel(float w) {
    if (w < 0.07f) return "Factory New";
    if (w < 0.15f) return "Minimal Wear";
    if (w < 0.38f) return "Field-Tested";
    if (w < 0.45f) return "Well-Worn";
    return "Battle-Scarred";
}

ImVec4 WearColor(float w) {
    if (w < 0.07f) return ImVec4(0.20f, 0.90f, 0.30f, 1.0f);
    if (w < 0.15f) return ImVec4(0.20f, 0.60f, 1.00f, 1.0f);
    if (w < 0.38f) return ImVec4(1.00f, 0.85f, 0.00f, 1.0f);
    if (w < 0.45f) return ImVec4(1.00f, 0.55f, 0.00f, 1.0f);
    return ImVec4(1.00f, 0.20f, 0.20f, 1.0f);
}

bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(), needle.end(),
                          [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a))
                                  == std::tolower(static_cast<unsigned char>(b));
                          });
    return it != haystack.end();
}

} // namespace

// ── Helpers ─────────────────────────────────────────────────────────────────────

const char* SkinChangerPanel::CategoryLabel(Category c) noexcept {
    switch (c) {
    case Category::Pistols: return "Pistols";
    case Category::Rifles:  return "Rifles";
    case Category::Smgs:    return "SMGs";
    case Category::Snipers: return "Snipers";
    case Category::Heavy:   return "Heavy";
    case Category::Knives:  return "Knives";
    case Category::Gloves:  return "Gloves";
    case Category::Music:   return "Music";
    }
    return "";
}

std::vector<WeaponsEnum> SkinChangerPanel::WeaponsForCategory(Category c) noexcept {
    std::vector<WeaponsEnum> result;
    if (c == Category::Knives) {
        for (auto k : kAllKnifeDefs) result.push_back(k);
        return result;
    }
    for (auto w : kAllWeaponDefs) {
        auto t = WeaponEnumToType(w);
        switch (c) {
        case Category::Pistols: if (t == CSWeaponType::Pistol) result.push_back(w); break;
        case Category::Rifles:  if (t == CSWeaponType::Rifle) result.push_back(w); break;
        case Category::Smgs:    if (t == CSWeaponType::SubmachineGun) result.push_back(w); break;
        case Category::Snipers: if (t == CSWeaponType::SniperRifle) result.push_back(w); break;
        case Category::Heavy:   if (t == CSWeaponType::Shotgun || t == CSWeaponType::MachineGun) result.push_back(w); break;
        default: break;
        }
    }
    return result;
}

ImColor SkinChangerPanel::PreviewColor(i32 paint_id) noexcept {
    if (paint_id <= 0) return ImColor(0.20f, 0.20f, 0.28f);
    float hue = static_cast<float>((static_cast<u32>(paint_id) * 137'508u) % 360u) / 360.0f;
    return ImColor::HSV(hue, 0.55f, 0.70f);
}

// ── Draw ────────────────────────────────────────────────────────────────────────

void SkinChangerPanel::Draw() noexcept {
    if (tex_cache_) tex_cache_->ProcessPending();
    ImGui::SetNextWindowSize(ImVec2(820, 560), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, Palette::Bg);

    ImGui::Begin("VeneeR", &open_,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // ── Layout: sidebar | main content ────────────────────────────────────────
    float sidebar_w = 100.0f;
    ImVec2 content_min = ImGui::GetCursorScreenPos();
    float content_h = ImGui::GetContentRegionAvail().y;

    // ── Sidebar ────────────────────────────────────────────────────────────────
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();

    dl->AddRectFilled(win_pos, ImVec2(win_pos.x + sidebar_w, win_pos.y + win_size.y),
                      ImColor(Palette::Sidebar));
    dl->AddRectFilled(ImVec2(win_pos.x + sidebar_w, win_pos.y),
                      ImVec2(win_pos.x + sidebar_w + 1, win_pos.y + win_size.y),
                      ImColor(1.0f, 1.0f, 1.0f, 0.04f));

    ImGui::BeginChild("##sidebar", ImVec2(sidebar_w, 0), false);

    constexpr std::pair<Category, const char*> sides[] = {
        {Category::Pistols, "Pistols"},
        {Category::Rifles,  "Rifles"},
        {Category::Smgs,    "SMGs"},
        {Category::Snipers, "Snipers"},
        {Category::Heavy,   "Heavy"},
        {Category::Knives,  "Knives"},
        {Category::Gloves,  "Gloves"},
        {Category::Music,   "Music"},
    };

    for (i32 i = 0; i < 8; ++i) {
        bool active = (active_cat_ == sides[i].first);
        ImVec2 p = ImGui::GetCursorScreenPos();

        if (active) {
            dl->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 3, p.y + 36),
                              ImColor(Palette::Accent));
            dl->AddRectFilled(p, ImVec2(p.x + sidebar_w, p.y + 36),
                              ImColor(Palette::AccentGlow));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, active ? Palette::Accent : Palette::TextDim);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

        char label[64];
        std::snprintf(label, sizeof(label), "%s##s_%d", sides[i].second, i);
        if (ImGui::Selectable(label, active, ImGuiSelectableFlags_None,
                              ImVec2(sidebar_w, 32))) {
            active_cat_ = sides[i].first;
            selected_weapon_idx_ = 0;
            selected_skin_idx_ = 0;
            cat_weapons_ = WeaponsForCategory(sides[i].first);
        }

        ImGui::PopStyleColor();
    }

    // CT/T toggle — affects knife and glove config only
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::TextColored(Palette::TextDim, "Knife/Glove");
    float half = (sidebar_w - 8.0f) / 2.0f;

    ImGui::PushStyleColor(ImGuiCol_Button, ct_selected_ ? Palette::Accent : Palette::Card);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Palette::AccentDim);
    if (ImGui::Button("CT", ImVec2(half, 24))) ct_selected_ = true;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Selects knife and glove config for Counter-Terrorist side.\nWeapon skins apply to both teams.");
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 2);

    ImGui::PushStyleColor(ImGuiCol_Button, !ct_selected_ ? Palette::Accent : Palette::Card);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Palette::AccentDim);
    if (ImGui::Button("T", ImVec2(half, 24))) ct_selected_ = false;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Selects knife and glove config for Terrorist side.\nWeapon skins apply to both teams.");
    ImGui::PopStyleColor(2);

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // ── Main content ───────────────────────────────────────────────────────────
    ImGui::BeginChild("##main_content", ImVec2(0, 0), false,
                      ImGuiWindowFlags_AlwaysUseWindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

    // Header
    DrawHeader();

    // Team label for knife/glove categories
    if (active_cat_ == Category::Knives || active_cat_ == Category::Gloves) {
        ImGui::TextColored(ct_selected_ ? Palette::TextGood : Palette::Accent,
                           "%s %s", ct_selected_ ? "CT" : "T",
                           active_cat_ == Category::Knives ? "Knife" : "Gloves");
        ImGui::Spacing();
    }

    switch (active_cat_) {
    case Category::Pistols: case Category::Rifles:
    case Category::Smgs:    case Category::Snipers:
    case Category::Heavy:   case Category::Knives:
        DrawSkinsContent();
        break;
    case Category::Gloves:
        DrawGlovesContent();
        break;
    case Category::Music:
        DrawMusicContent();
        break;
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(5);
}

// ── Header ──────────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawHeader() noexcept {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;

    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 32),
                      ImColor(0.09f, 0.09f, 0.14f), 4.0f);

    ImGui::SetCursorScreenPos(ImVec2(p.x + 12, p.y + 7));
    ImGui::TextColored(Palette::Accent, "VeneeR");
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Palette::TextDim, "v1.5.7");

    if (!equipped_weapon_.empty()) {
        ImGui::SameLine(0, 18);
        ImGui::TextColored(Palette::TextDim, "Equipped:");
        ImGui::SameLine(0, 6);
        ImGui::TextColored(Palette::TextGood, "%s", equipped_weapon_.c_str());
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 36));
}

// ── Skins content ───────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinsContent() noexcept {
    if (cat_weapons_.empty())
        cat_weapons_ = WeaponsForCategory(active_cat_);

    if (cat_weapons_.empty()) {
        ImGui::TextColored(Palette::TextDim, "No weapons in this category");
        return;
    }

    float avail_w = ImGui::GetContentRegionAvail().x;
    float list_w = (avail_w < 500) ? 130.0f : 150.0f;
    float detail_w = avail_w - list_w - 8.0f;

    // Weapon filter
    ImGui::SetNextItemWidth(list_w);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputTextWithHint("##wep_filter", "Search weapon...", &weapon_filter_);
    ImGui::PopStyleVar();
    ImGui::SameLine(0, 8);

    // Skin filter
    ImGui::SetNextItemWidth(detail_w);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputTextWithHint("##skin_filter", "Search skin...", &skin_filter_);
    ImGui::PopStyleVar();

    // Filter weapons
    std::vector<WeaponsEnum> filtered;
    for (auto w : cat_weapons_) {
        if (ContainsIgnoreCase(WeaponIdToString(static_cast<i32>(w)), weapon_filter_))
            filtered.push_back(w);
    }
    if (filtered.empty()) {
        ImGui::TextColored(Palette::TextDim, "No matching weapons");
        return;
    }
    selected_weapon_idx_ = std::clamp<i32>(selected_weapon_idx_, 0,
        static_cast<i32>(filtered.size()) - 1);
    WeaponsEnum weapon = filtered[selected_weapon_idx_];
    bool is_knife = mgr_.IsKnifeDef(static_cast<i32>(weapon));

    auto skins = db_.GetWeaponSkins(weapon);
    std::vector<SkinInfo_t> filtered_skins;
    for (auto& s : skins) {
        if (ContainsIgnoreCase(s.name, skin_filter_))
            filtered_skins.push_back(s);
    }
    selected_skin_idx_ = std::clamp<i32>(selected_skin_idx_, 0,
        static_cast<i32>(filtered_skins.size()) - 1);

    // Sync selection highlight with the actually-applied skin
    SkinInfo_t applied{};
    if (is_knife) {
        i32 kp = mgr_.GetKnifePaint(ct_selected_);
        if (kp > 0) {
            applied.Paint = kp;
            applied.Wear = 0.01f;
            applied.Pattern = 0;
            applied.StatTrak = -1;
            applied.weaponType = weapon;
        }
    } else {
        applied = mgr_.GetSkin(weapon);
    }
    if (applied.Paint > 0) {
        for (i32 i = 0; i < static_cast<i32>(filtered_skins.size()); ++i) {
            if (filtered_skins[i].Paint == applied.Paint) {
                selected_skin_idx_ = i;
                break;
            }
        }
    }

    // ── Weapon list ──────────────────────────────────────────────────────────
    ImGui::BeginChild("##weapon_list", ImVec2(list_w, 0), true);
    for (i32 i = 0; i < static_cast<i32>(filtered.size()); ++i) {
        bool sel = (i == selected_weapon_idx_);
        auto name = WeaponIdToString(static_cast<i32>(filtered[i]));

        ImVec2 p = ImGui::GetCursorScreenPos();
        if (sel) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(p, ImVec2(p.x + list_w, p.y + 26),
                              ImColor(Palette::AccentGlow));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, sel ? Palette::Accent : Palette::Text);
        char label[128];
        std::snprintf(label, sizeof(label), "%s##wl_%d", name.c_str(), i);
        if (ImGui::Selectable(label, sel, ImGuiSelectableFlags_None,
                              ImVec2(0, 24))) {
            selected_weapon_idx_ = i;
            selected_skin_idx_ = 0;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 8);

    // ── Detail area ───────────────────────────────────────────────────────────
    ImGui::BeginChild("##skin_detail", ImVec2(detail_w, 0), false);

    if (filtered_skins.empty()) {
        ImGui::TextColored(Palette::TextDim, "No matching skins");
        ImGui::EndChild();
        return;
    }

    DrawSkinCards(filtered_skins, weapon, is_knife);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Detail controls ──────────────────────────────────────────────────────
    auto& chosen = filtered_skins[selected_skin_idx_];
    SkinInfo_t skin = chosen;
    if (applied.Paint > 0) {
        skin.Wear = applied.Wear;
        skin.Pattern = applied.Pattern;
        skin.StatTrak = applied.StatTrak;
    }

    ImGui::TextColored(Palette::Accent, "%s", skin.name.c_str());
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Palette::TextDim, "#%d", skin.Paint);

    bool changed = false;
    DrawWearControls(skin, changed);

    ImGui::Spacing();
    ImGui::SetNextItemWidth(100.0f);
    int st = (skin.StatTrak >= 0) ? skin.StatTrak : 0;
    if (ImGui::InputInt("##stattrak", &st, 1, 100)) {
        skin.StatTrak = std::clamp(st, -1, 999999);
        changed = true;
    }
    ImGui::SameLine(0, 8);
    bool st_enabled = (skin.StatTrak >= 0);
    if (ImGui::Checkbox("StatTrak##st_en", &st_enabled)) {
        skin.StatTrak = st_enabled ? 0 : -1;
        changed = true;
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - 60);
    ImGui::PushStyleColor(ImGuiCol_Button, Palette::Accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Palette::AccentDim);
    if (ImGui::Button("Apply Skin", ImVec2(120, 28))) {
        if (is_knife)
            mgr_.SetTeamKnife(weapon, skin.Paint, ct_selected_);
        else
            mgr_.SetSkin(skin);
        SaveAndSync();
    }
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
}

// ── Skin cards grid ─────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinCards(std::span<const SkinInfo_t> skins,
                                      WeaponsEnum weapon, bool is_knife) noexcept {
    float avail_w = ImGui::GetContentRegionAvail().x;
    int cols = std::max(2, static_cast<int>(avail_w / 150.0f));
    if (cols > static_cast<int>(skins.size())) cols = static_cast<int>(skins.size());
    if (cols < 1) cols = 1;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    if (ImGui::BeginTable("##skin_grid", cols,
                          ImGuiTableFlags_SizingStretchSame |
                          ImGuiTableFlags_NoPadInnerX |
                          ImGuiTableFlags_NoPadOuterX)) {
        for (i32 i = 0; i < static_cast<i32>(skins.size()); ++i) {
            ImGui::TableNextColumn();
            DrawSkinCard(i, skins[i], i == selected_skin_idx_, weapon, is_knife);
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

// ── Single skin card ────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinCard(i32 idx, const SkinInfo_t& skin, bool selected,
                                      WeaponsEnum weapon, bool is_knife) noexcept {
    ImGui::PushID(idx);

    float card_w = ImGui::GetContentRegionAvail().x;
    float preview_h = 60.0f;
    float total_h = preview_h + 28.0f;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImColor preview_col = PreviewColor(skin.Paint);

    // Card shadow
    dl->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1),
                      ImVec2(pos.x + card_w + 1, pos.y + total_h + 1),
                      ImColor(0, 0, 0, 50), 3.0f);

    bool hovered = ImGui::IsWindowHovered() &&
                   ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + card_w, pos.y + total_h));
    ImU32 card_bg = selected          ? ImColor(Palette::AccentDim)
                  : hovered           ? ImColor(Palette::CardHover)
                                     : ImColor(Palette::Card);

    // Card border highlight for selected
    if (selected)
        dl->AddRect(pos, ImVec2(pos.x + card_w, pos.y + total_h),
                    ImColor(Palette::Accent), 3.0f, 0, 1.5f);
    dl->AddRectFilled(pos, ImVec2(pos.x + card_w, pos.y + total_h), card_bg, 3.0f);

    // Preview
    ImVec2 preview_min(pos.x + 2, pos.y + 2);
    ImVec2 preview_max(pos.x + card_w - 2, pos.y + preview_h - 2);

    ImTextureID tex_id = tex_cache_ ? tex_cache_->Get(skin.Paint) : ImTextureID(0);
    if (tex_id) {
        dl->AddImage(tex_id, preview_min, preview_max,
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255,255,255,255));
    } else {
        dl->AddRectFilled(preview_min, preview_max, preview_col, 3.0f);
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x + 2, pos.y + preview_h * 0.5f),
            ImVec2(pos.x + card_w - 2, pos.y + preview_h - 2),
            ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
            ImColor(0, 0, 0, 80), ImColor(0, 0, 0, 80));
    }

    if (tex_cache_ && !tex_cache_->IsLoaded(skin.Paint) && skin.weaponType != WeaponsEnum{})
        tex_cache_->Request(skin.Paint, skin.weaponType);

    // Paint ID badge
    float badge_w = ImGui::CalcTextSize("#000").x + 10.0f;
    dl->AddRectFilled(ImVec2(pos.x + card_w - badge_w - 2, pos.y + preview_h - 18),
                      ImVec2(pos.x + card_w - 2, pos.y + preview_h - 2),
                      ImColor(0, 0, 0, 160), 2.0f);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + card_w - badge_w, pos.y + preview_h - 16));
    char badge[32];
    std::snprintf(badge, sizeof(badge), "#%d", skin.Paint);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", badge);

    // Skin name
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 6, pos.y + preview_h + 4));
    ImColor name_col = selected ? ImColor(255,255,255,255) : ImColor(Palette::Text);
    ImGui::PushStyleColor(ImGuiCol_Text, name_col.Value);
    ImGui::PushTextWrapPos(pos.x + card_w - 6.0f);
    ImGui::TextUnformatted(skin.name.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();

    // Clickable area
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##card", ImVec2(card_w, total_h));
    if (ImGui::IsItemClicked()) {
        selected_skin_idx_ = idx;
        if (is_knife)
            mgr_.SetTeamKnife(weapon, skin.Paint, ct_selected_);
        else
            mgr_.SetSkin(skin);
        SaveAndSync();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("%s\nPaint Kit #%d", skin.name.c_str(), skin.Paint);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + total_h + 4));
    ImGui::PopID();
}

// ── Wear & pattern controls ─────────────────────────────────────────────────────

void SkinChangerPanel::DrawWearControls(SkinInfo_t& skin, bool& changed) noexcept {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 3.0f;

    // Gradient bar
    dl->AddRectFilledMultiColor(
        p, ImVec2(p.x + w * 0.07f, p.y + h),
        ImColor(0.20f, 0.90f, 0.30f), ImColor(0.20f, 0.90f, 0.30f),
        ImColor(0.20f, 0.90f, 0.30f), ImColor(0.20f, 0.90f, 0.30f));
    dl->AddRectFilledMultiColor(
        ImVec2(p.x + w * 0.07f, p.y), ImVec2(p.x + w * 0.15f, p.y + h),
        ImColor(0.20f, 0.90f, 0.30f), ImColor(0.20f, 0.60f, 1.00f),
        ImColor(0.20f, 0.60f, 1.00f), ImColor(0.20f, 0.90f, 0.30f));
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

    ImGui::Dummy(ImVec2(0, h + 6));

    ImGui::SetNextItemWidth(220.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, WearColor(skin.Wear));
    if (ImGui::SliderFloat("##wear", &skin.Wear, 0.0f, 1.0f, "%.4f"))
        changed = true;
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 8);
    ImGui::TextColored(WearColor(skin.Wear), "%s", WearLabel(skin.Wear));

    ImGui::Spacing();
    ImGui::SetNextItemWidth(120.0f);
    int pattern = skin.Pattern;
    if (ImGui::InputInt("##pattern", &pattern, 1, 100)) {
        skin.Pattern = std::clamp(pattern, 0, 9999);
        changed = true;
    }
    ImGui::SameLine(0, 8);
    ImGui::TextColored(Palette::TextDim, "Pattern Seed");
}

// ── Gloves tab ──────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawGlovesContent() noexcept {
    float avail_w = ImGui::GetContentRegionAvail().x;
    float list_w = (avail_w < 500) ? 130.0f : 150.0f;
    float grid_w = avail_w - list_w - 8.0f;

    // Glove type list
    ImGui::BeginChild("##glove_type_list", ImVec2(list_w, 0), true);
    for (i32 i = 0; i < static_cast<i32>(std::size(kAllGloveDefs)); ++i) {
        auto name = GloveIdToString(static_cast<i32>(kAllGloveDefs[i]));
        bool sel = (i == selected_glove_type_idx_);

        if (sel) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(ImGui::GetCursorScreenPos(),
                              ImVec2(ImGui::GetCursorScreenPos().x + list_w,
                                     ImGui::GetCursorScreenPos().y + 26),
                              ImColor(Palette::AccentGlow));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, sel ? Palette::Accent : Palette::Text);
        char label[128];
        std::snprintf(label, sizeof(label), "%s##gl_%d", name.c_str(), i);
        if (ImGui::Selectable(label, sel, ImGuiSelectableFlags_None, ImVec2(0, 24))) {
            selected_glove_type_idx_ = i;
            selected_glove_skin_idx_ = 0;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 8);

    // Glove skins
    ImGui::BeginChild("##glove_skin_area", ImVec2(grid_w, 0), false);

    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputTextWithHint("##glove_skin_filter", "Search skins...", &glove_skin_search_);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    GlovesEnum glove = kAllGloveDefs[selected_glove_type_idx_];
    auto skins = db_.GetGloveSkins(glove);

    std::vector<SkinInfo_t> filtered;
    for (auto& s : skins) {
        if (ContainsIgnoreCase(s.name, glove_skin_search_))
            filtered.push_back(s);
    }

    if (!filtered.empty())
        selected_glove_skin_idx_ = std::clamp<i32>(selected_glove_skin_idx_, 0,
            static_cast<i32>(filtered.size()) - 1);

    float g_avail = ImGui::GetContentRegionAvail().x;
    int cols = std::max(2, static_cast<int>(g_avail / 150.0f));
    if (cols > static_cast<int>(filtered.size())) cols = static_cast<int>(filtered.size());
    if (cols < 1) cols = 1;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    if (ImGui::BeginTable("##glove_grid", cols,
                          ImGuiTableFlags_SizingStretchSame |
                          ImGuiTableFlags_NoPadInnerX |
                          ImGuiTableFlags_NoPadOuterX)) {
        for (i32 i = 0; i < static_cast<i32>(filtered.size()); ++i) {
            ImGui::TableNextColumn();
            ImGui::PushID(i + 2000);

            bool sel = (i == selected_glove_skin_idx_);
            float card_w_f = ImGui::GetContentRegionAvail().x;
            float preview_h = 60.0f, total_h = preview_h + 28.0f;

            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImColor col = PreviewColor(filtered[i].Paint);

            dl->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1),
                              ImVec2(pos.x + card_w_f + 1, pos.y + total_h + 1),
                              ImColor(0, 0, 0, 50), 3.0f);

            bool hovered = ImGui::IsWindowHovered() &&
                           ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + card_w_f, pos.y + total_h));
            ImU32 card_bg = sel          ? ImColor(Palette::AccentDim)
                          : hovered      ? ImColor(Palette::CardHover)
                                        : ImColor(Palette::Card);

            if (sel)
                dl->AddRect(pos, ImVec2(pos.x + card_w_f, pos.y + total_h),
                            ImColor(Palette::Accent), 3.0f, 0, 1.5f);
            dl->AddRectFilled(pos, ImVec2(pos.x + card_w_f, pos.y + total_h), card_bg, 3.0f);

            dl->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2),
                              ImVec2(pos.x + card_w_f - 2, pos.y + preview_h - 2), col, 3.0f);
            dl->AddRectFilledMultiColor(
                ImVec2(pos.x + 2, pos.y + preview_h * 0.5f),
                ImVec2(pos.x + card_w_f - 2, pos.y + preview_h - 2),
                ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
                ImColor(0, 0, 0, 80), ImColor(0, 0, 0, 80));

            float badge_w = ImGui::CalcTextSize("#000").x + 10.0f;
            dl->AddRectFilled(ImVec2(pos.x + card_w_f - badge_w - 2, pos.y + preview_h - 18),
                              ImVec2(pos.x + card_w_f - 2, pos.y + preview_h - 2),
                              ImColor(0, 0, 0, 160), 2.0f);
            ImGui::SetCursorScreenPos(ImVec2(pos.x + card_w_f - badge_w, pos.y + preview_h - 16));
            char badge[32];
            std::snprintf(badge, sizeof(badge), "#%d", filtered[i].Paint);
            ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", badge);

            ImGui::SetCursorScreenPos(ImVec2(pos.x + 6, pos.y + preview_h + 4));
            ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(1,1,1,1) : Palette::Text);
            ImGui::PushTextWrapPos(pos.x + card_w_f - 6.0f);
            ImGui::TextUnformatted(filtered[i].name.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();

            ImGui::SetCursorScreenPos(pos);
            ImGui::InvisibleButton("##gcard", ImVec2(card_w_f, total_h));
            if (ImGui::IsItemClicked()) {
                selected_glove_skin_idx_ = i;
                SkinInfo_t gs{filtered[i].Paint, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum{}};
                mgr_.SetTeamGloveSkin(gs, glove, ct_selected_);
                SaveAndSync();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("%s\nPaint Kit #%d", filtered[i].name.c_str(), filtered[i].Paint);

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Separator();

    i32 ct_active = mgr_.GetGlovePaint(true);
    i32 t_active  = mgr_.GetGlovePaint(false);
    ImGui::TextColored(Palette::TextDim, "Active:");
    ImGui::SameLine(0, 6);
    if (ct_active > 0) {
        ImGui::TextColored(Palette::TextGood, "CT #%d", ct_active);
        ImGui::SameLine(0, 10);
    }
    if (t_active > 0)
        ImGui::TextColored(Palette::TextGood, "T #%d", t_active);
    if (ct_active <= 0 && t_active <= 0)
        ImGui::TextColored(Palette::TextDim, "None");

    ImGui::EndChild();
}

// ── Music tab ───────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawMusicContent() noexcept {
    struct Kit { u16 id; const char* name; };
    static const Kit kits[] = {
        {0,   "Default"}, {1,   "Counter-Strike 2"}, {3,   "Crimson Assault"},
        {4,   "Sharpened"}, {5,   "Insurgency"}, {6,   "A_D_8"}, {7,   "High Noon"},
        {8,   "Death's Head Demolition"}, {9,   "Desert Fire"}, {10,  "LNOE"},
        {11,  "Metal"}, {12,  "All I Want For Christmas"}, {13,  "Iso Rhythm"},
        {14,  "For No Mankind"}, {15,  "Hotline Miami"}, {16,  "The 8-Bit Kit"},
        {17,  "The Talos Principle"}, {18,  "Battlepack"}, {19,  "Molotov"},
        {20,  "Uber Blasto Phone"}, {21,  "Hazardous Environments"}, {22,  "Headshot"},
        {23,  "Total Domination"}, {24,  "I Am"}, {25,  "Diamonds"},
        {26,  "Invasion!"}, {27,  "Lion's Mouth"}, {28,  "Sponge Fingerz"},
        {29,  "Aggressive"}, {30,  "Java Havana Funkaloo"}, {31,  "Moments CSGO"},
        {32,  "Disgusting"}, {33,  "The Good Youth"}, {34,  "FREE"},
        {35,  "Life's Not Out To Get You"}, {36,  "Backbone"}, {37,  "GLA"},
        {38,  "Arena"}, {39,  "EZ4ENCE"}, {40,  "Halo"},
        {43,  "Bachram"}, {44,  "Taco Truck"}, {45,  "Eye of the Dragon"},
        {46,  "M.U.D.D. FORCE"}, {47,  "Neo Noir"}, {48,  "Bodacious"},
        {49,  "Drifter"}, {50,  "All For Dust"}, {51,  "Hades"},
        {52,  "The Lowfile Pack"}, {53,  "CHAINSAW Lxadxut"}, {54,  "Mocha Petal"},
        {55,  "Yellow Magic"}, {56,  "VICI"}, {57,  "Atro Bellum"},
        {58,  "Work Hard, Play Hard"}, {59,  "Kolibri"}, {60,  "U Mad!"},
        {61,  "Flashbang Dance"}, {62,  "Heading For The Source"}, {63,  "Void"},
        {64,  "Shooters"}, {65,  "Dashstar*"}, {66,  "Gothic Luxury"},
        {67,  "Lock Me Up"}, {68,  "Hua Lian"}, {69,  "Ultimate"},
        {76,  "Under Bright Lights"},
    };
    static constexpr i32 kit_count = sizeof(kits) / sizeof(kits[0]);

    u16 current_id = changer_.GetMusicKit();
    selected_music_idx_ = 0;
    for (i32 i = 0; i < kit_count; ++i) {
        if (kits[i].id == current_id) { selected_music_idx_ = i; break; }
    }

    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputTextWithHint("##music_search", "Search music kits...", &music_filter_);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    ImGui::BeginChild("##music_list", ImVec2(0, 0), true);
    for (i32 i = 0; i < kit_count; ++i) {
        if (!ContainsIgnoreCase(kits[i].name, music_filter_))
            continue;

        bool sel = (i == selected_music_idx_);
        if (sel) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(ImGui::GetCursorScreenPos(),
                              ImVec2(ImGui::GetCursorScreenPos().x +
                                     ImGui::GetContentRegionAvail().x,
                                     ImGui::GetCursorScreenPos().y + 26),
                              ImColor(Palette::AccentGlow));
        }

        ImGui::PushStyleColor(ImGuiCol_Text, sel ? Palette::Accent : Palette::Text);
        char label[128];
        std::snprintf(label, sizeof(label), "%s##mk_%d", kits[i].name, i);
        if (ImGui::Selectable(label, sel, ImGuiSelectableFlags_None, ImVec2(0, 24))) {
            selected_music_idx_ = i;
            changer_.SetMusicKit(kits[i].id);
            SaveAndSync();
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextColored(Palette::TextDim, "Current:");
    ImGui::SameLine(0, 6);
    ImGui::Text("%s (ID %u)", kits[selected_music_idx_].name, kits[selected_music_idx_].id);
}

// ── Config persistence ──────────────────────────────────────────────────────────

void SkinChangerPanel::SaveAndSync() noexcept {
    SaveConfig(mgr_, changer_.GetMusicKit());
}
