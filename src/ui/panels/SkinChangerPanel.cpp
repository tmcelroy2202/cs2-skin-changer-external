#include "SkinChangerPanel.h"
#include "imgui.h"
#include <misc/cpp/imgui_stdlib.h>
#include "cs2/WeaponTypes.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace {

struct Palette {
    inline static const ImVec4 Bg        = ImVec4(0.04f, 0.04f, 0.08f, 1.00f);
    inline static const ImVec4 Panel     = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    inline static const ImVec4 Card      = ImVec4(0.13f, 0.13f, 0.22f, 1.00f);
    inline static const ImVec4 CardHover = ImVec4(0.18f, 0.18f, 0.30f, 1.00f);
    inline static const ImVec4 Accent    = ImVec4(0.18f, 0.49f, 0.91f, 1.00f);
    inline static const ImVec4 Gold      = ImVec4(0.96f, 0.62f, 0.04f, 1.00f);
    inline static const ImVec4 Text      = ImVec4(0.89f, 0.89f, 0.94f, 1.00f);
    inline static const ImVec4 TextDim   = ImVec4(0.42f, 0.42f, 0.50f, 1.00f);
    inline static const ImVec4 TextGood  = ImVec4(0.20f, 0.83f, 0.60f, 1.00f);
    inline static const ImVec4 TabActive = ImVec4(0.18f, 0.49f, 0.91f, 0.20f);
};

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

// ── helpers ──────────────────────────────────────────────────────────────────

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
    for (auto w : kAllWeaponDefs) {
        auto t = WeaponEnumToType(w);
        switch (c) {
        case Category::Pistols:
            if (t == CSWeaponType::Pistol) result.push_back(w);
            break;
        case Category::Rifles:
            if (t == CSWeaponType::Rifle) result.push_back(w);
            break;
        case Category::Smgs:
            if (t == CSWeaponType::SubmachineGun) result.push_back(w);
            break;
        case Category::Snipers:
            if (t == CSWeaponType::SniperRifle) result.push_back(w);
            break;
        case Category::Heavy:
            if (t == CSWeaponType::Shotgun || t == CSWeaponType::MachineGun)
                result.push_back(w);
            break;
        case Category::Knives:
            for (auto k : kAllKnifeDefs) result.push_back(k);
            break;
        default:
            break;
        }
    }
    return result;
}

ImColor SkinChangerPanel::PreviewColor(i32 paint_id) noexcept {
    if (paint_id <= 0) return ImColor(0.25f, 0.25f, 0.30f);
    float hue = static_cast<float>((static_cast<u32>(paint_id) * 137'508u) % 360u) / 360.0f;
    return ImColor::HSV(hue, 0.55f, 0.70f);
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void SkinChangerPanel::Draw() noexcept {
    if (tex_cache_) tex_cache_->ProcessPending();
    ImGui::SetNextWindowSize(ImVec2(760, 520), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, Palette::Bg);

    ImGui::Begin("VeneeR", &open_,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    DrawTitleBar();
    DrawEquippedBar();
    DrawCategories();

    // Content
    ImGui::BeginChild("##content", ImVec2(0, 0), false,
                      ImGuiWindowFlags_AlwaysUseWindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

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
    ImGui::PopStyleVar(4);
}

// ── Title bar ──────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawTitleBar() noexcept {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    float w = ImGui::GetWindowWidth();

    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + 38),
                      ImColor(0.06f, 0.06f, 0.10f));
    dl->AddRectFilled(ImVec2(pos.x, pos.y + 38),
                      ImVec2(pos.x + w, pos.y + 40),
                      ImColor(Palette::Accent));

    ImGui::SetCursorScreenPos(ImVec2(pos.x + 16, pos.y + 10));
    ImGui::TextColored(Palette::Accent, "VeneeR");
    ImGui::SameLine(0, 2);
    ImGui::TextColored(Palette::TextDim, "v1.5.3");

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 44));
}

// ── Equipped weapon ────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawEquippedBar() noexcept {
    if (equipped_weapon_.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;

    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 30),
                      ImColor(0.08f, 0.08f, 0.14f), 6.0f);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 14, p.y + 7));
    ImGui::TextColored(Palette::TextDim, "Equipped:");
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Palette::TextGood, "%s", equipped_weapon_.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 36));
}

// ── Category tabs ──────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawCategories() noexcept {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

    constexpr Category cats[] = {
        Category::Pistols, Category::Rifles, Category::Smgs,
        Category::Snipers, Category::Heavy, Category::Knives,
        Category::Gloves, Category::Music,
    };

    float avail = ImGui::GetContentRegionAvail().x;
    float btn_w = (avail - 8.0f * 7.0f) / 8.0f;
    if (btn_w < 60.0f) btn_w = 60.0f;

    // CT / T toggle
    {
        ImU32 bg_ct = ct_selected_ ? ImColor(Palette::Accent) : ImColor(0,0,0,0);
        ImU32 bg_t  = !ct_selected_ ? ImColor(Palette::Accent) : ImColor(0,0,0,0);
        ImU32 bg_ho = ImColor(Palette::Accent);

        ImGui::PushStyleColor(ImGuiCol_Button, bg_ct);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_ho);
        ImGui::PushStyleColor(ImGuiCol_Text, ct_selected_ ? ImVec4(1,1,1,1) : Palette::TextDim);
        if (ImGui::Button("CT", ImVec2(30, 30))) { ct_selected_ = true; }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 4);

        ImGui::PushStyleColor(ImGuiCol_Button, bg_t);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_ho);
        ImGui::PushStyleColor(ImGuiCol_Text, !ct_selected_ ? ImVec4(1,1,1,1) : Palette::TextDim);
        if (ImGui::Button("T", ImVec2(30, 30))) { ct_selected_ = false; }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 12);
    }

    for (i32 i = 0; i < 8; ++i) {
        if (i > 0) ImGui::SameLine(0, 8);

        bool active = active_cat_ == cats[i];
        ImU32 bg = active ? ImColor(Palette::Accent) : ImColor(0, 0, 0, 0);
        ImU32 bg_hover = active ? ImColor(Palette::Accent)
                                : ImColor(1.0f, 1.0f, 1.0f, 0.06f);
        ImU32 text_col = active ? ImColor(1.0f, 1.0f, 1.0f)
                                : ImColor(Palette::TextDim);

        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_hover);
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);

        if (ImGui::Button(CategoryLabel(cats[i]), ImVec2(btn_w, 30))) {
            active_cat_ = cats[i];
            selected_weapon_idx_ = 0;
            selected_skin_idx_ = 0;
            cat_weapons_ = WeaponsForCategory(cats[i]);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();
}

// ── Skins content ──────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinsContent() noexcept {
    // Build weapon list for this category on first access
    if (cat_weapons_.empty())
        cat_weapons_ = WeaponsForCategory(active_cat_);

    if (cat_weapons_.empty()) {
        ImGui::TextColored(Palette::TextDim, "No weapons in this category");
        return;
    }

    // ── Weapon search + filter chips ─────────────────────────────────────────
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
    ImGui::InputTextWithHint("##wep_filter", "Filter weapons...", &weapon_filter_);
    ImGui::PopStyleVar();

    // Build filtered weapon list
    std::vector<WeaponsEnum> filtered;
    for (auto w : cat_weapons_) {
        auto name = WeaponIdToString(static_cast<i32>(w));
        if (ContainsIgnoreCase(name, weapon_filter_))
            filtered.push_back(w);
    }

    // ── Weapon chips ─────────────────────────────────────────────────────────
    ImGui::BeginChild("##weapon_chips", ImVec2(0, 34), false);
    float cx = ImGui::GetCursorPosX();
    float cw = ImGui::GetContentRegionAvail().x;

    if (!filtered.empty())
        selected_weapon_idx_ = std::clamp<i32>(selected_weapon_idx_, 0,
            static_cast<i32>(filtered.size()) - 1);

    float chip_x = cx;
    for (i32 i = 0; i < static_cast<i32>(filtered.size()); ++i) {
        auto name = WeaponIdToString(static_cast<i32>(filtered[i]));
        bool sel = (i == selected_weapon_idx_);

        ImVec2 txt_size = ImGui::CalcTextSize(name.c_str());
        float cw_chip = txt_size.x + 20.0f;
        if (chip_x + cw_chip > cx + cw && i > 0) break;

        ImGui::SetCursorPosX(chip_x);

        ImU32 chip_bg = sel ? ImColor(Palette::Accent) : ImColor(Palette::Card);
        ImU32 chip_bg_hov = sel ? ImColor(Palette::Accent) : ImColor(Palette::CardHover);
        ImU32 chip_txt = sel ? ImColor(1.0f, 1.0f, 1.0f) : ImColor(Palette::Text);

        ImGui::PushStyleColor(ImGuiCol_Button, chip_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chip_bg_hov);
        ImGui::PushStyleColor(ImGuiCol_Text, chip_txt);

        char chip_label[64];
        std::snprintf(chip_label, sizeof(chip_label), "%s##wc_%d", name.c_str(), i);

        if (ImGui::Button(chip_label, ImVec2(cw_chip, 28))) {
            selected_weapon_idx_ = i;
            selected_skin_idx_ = 0;
        }

        ImGui::PopStyleColor(3);
        chip_x += cw_chip + 6.0f;
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // Show team indicator when editing knives
    if (active_cat_ == Category::Knives) {
        ImGui::TextColored(ct_selected_ ? Palette::TextGood : Palette::TextDim,
                           "Editing: %s Knife", ct_selected_ ? "CT" : "T");
        ImGui::Spacing();
    }

    // ── Skin cards ───────────────────────────────────────────────────────────
    WeaponsEnum weapon = filtered.empty() ? cat_weapons_[0] : filtered[selected_weapon_idx_];
    bool is_knife = mgr_.IsKnifeDef(static_cast<i32>(weapon));

    auto skins = db_.GetWeaponSkins(weapon);

    // Skin search
    ImGui::SetNextItemWidth(220.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
    ImGui::InputTextWithHint("##skin_filter", "Search skins...", &skin_filter_);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    if (skins.empty()) {
        ImGui::TextColored(Palette::TextDim, "No skins available for this weapon");
        return;
    }

    // Filter skins
    std::vector<SkinInfo_t> filtered_skins;
    for (auto& s : skins) {
        if (ContainsIgnoreCase(s.name, skin_filter_))
            filtered_skins.push_back(s);
    }

    if (filtered_skins.empty()) {
        ImGui::TextColored(Palette::TextDim, "No matching skins");
        return;
    }

    selected_skin_idx_ = std::clamp<i32>(selected_skin_idx_, 0,
        static_cast<i32>(filtered_skins.size()) - 1);

    // Grid of skin cards
    DrawSkinCards(filtered_skins, weapon, is_knife);

    ImGui::Spacing();
    ImGui::Separator();

    // ── Detail controls ──────────────────────────────────────────────────────
    auto& chosen = filtered_skins[selected_skin_idx_];
    SkinInfo_t skin = chosen;

    // Pre-fill wear/pattern/statTrak from existing selection
    SkinInfo_t existing{};
    if (is_knife) {
        i32 kp = mgr_.GetKnifePaint(ct_selected_);
        if (kp > 0) {
            existing.Paint = kp;
            existing.Wear = 0.01f;
            existing.Pattern = 0;
            existing.StatTrak = -1;
            existing.weaponType = weapon;
        }
    } else {
        existing = mgr_.GetSkin(weapon);
    }
    if (existing.Paint > 0) {
        skin.Wear = existing.Wear;
        skin.Pattern = existing.Pattern;
        skin.StatTrak = existing.StatTrak;
    }

    // Name header
    ImGui::TextColored(Palette::Accent, "%s", skin.name.c_str());
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Palette::TextDim, "#%d", skin.Paint);

    bool changed = false;
    DrawWearControls(skin, changed);

    // StatTrak input
    ImGui::Spacing();
    ImGui::SetNextItemWidth(120.0f);
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Enable StatTrak counter on this weapon");

    // Apply button
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.5f - 50);
    ImGui::PushStyleColor(ImGuiCol_Button, Palette::Accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Palette::Accent);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    if (ImGui::Button("Apply Skin", ImVec2(120, 32))) {
        if (is_knife) {
            mgr_.SetTeamKnife(weapon, skin.Paint, ct_selected_);
        } else {
            mgr_.SetSkin(skin);
        }
        SaveAndSync();
    }
    ImGui::PopStyleColor(3);
}

// ── Skin cards grid ────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinCards(std::span<const SkinInfo_t> skins,
                                      WeaponsEnum weapon, bool is_knife) noexcept {
    float avail_w = ImGui::GetContentRegionAvail().x;
    int cols = std::max(2, static_cast<int>(avail_w / 175.0f));
    if (cols > static_cast<int>(skins.size())) cols = static_cast<int>(skins.size());
    if (cols < 1) cols = 1;

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
}

// ── Single skin card ───────────────────────────────────────────────────────────

void SkinChangerPanel::DrawSkinCard(i32 idx, const SkinInfo_t& skin, bool selected,
                                     WeaponsEnum weapon, bool is_knife) noexcept {
    ImGui::PushID(idx);

    float card_w = ImGui::GetContentRegionAvail().x;
    float preview_h = 72.0f;
    float total_h = preview_h + 34.0f;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImColor preview_col = PreviewColor(skin.Paint);

    // Shadow
    dl->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1),
                      ImVec2(pos.x + card_w + 1, pos.y + total_h + 1),
                      ImColor(0, 0, 0, 80), 6.0f);

    // Card body
    bool hovered = ImGui::IsWindowHovered() &&
                   ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + card_w, pos.y + total_h));
    ImU32 card_bg = selected ? ImColor(Palette::Accent)
                   : hovered ? ImColor(Palette::CardHover)
                             : ImColor(Palette::Card);
    dl->AddRectFilled(pos, ImVec2(pos.x + card_w, pos.y + total_h), card_bg, 6.0f);

    // Preview band (texture or generated color)
    ImVec2 preview_min(pos.x + 2, pos.y + 2);
    ImVec2 preview_max(pos.x + card_w - 2, pos.y + preview_h - 2);

    ImTextureID tex_id = tex_cache_ ? tex_cache_->Get(skin.Paint) : ImTextureID(0);
    if (tex_id) {
        dl->AddImage(tex_id, preview_min, preview_max,
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255,255,255,255));
    } else {
        dl->AddRectFilled(preview_min, preview_max, preview_col, 4.0f);

        // Darken gradient at bottom of preview
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x + 2, pos.y + preview_h * 0.5f),
            ImVec2(pos.x + card_w - 2, pos.y + preview_h - 2),
            ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
            ImColor(0, 0, 0, 120), ImColor(0, 0, 0, 120));
    }

    // Request async texture load for unloaded skins
    if (tex_cache_ && !tex_cache_->IsLoaded(skin.Paint) && skin.weaponType != WeaponsEnum{})
        tex_cache_->Request(skin.Paint, skin.weaponType);

    // Paint ID badge
    float badge_w = ImGui::CalcTextSize("#00").x + 16.0f;
    dl->AddRectFilled(ImVec2(pos.x + card_w - badge_w - 4, pos.y + preview_h - 22),
                      ImVec2(pos.x + card_w - 4, pos.y + preview_h - 4),
                      ImColor(0, 0, 0, 150), 3.0f);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + card_w - badge_w, pos.y + preview_h - 20));
    char badge[32];
    std::snprintf(badge, sizeof(badge), "#%d", skin.Paint);
    ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", badge);

    // Skin name
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y + preview_h + 6));
    ImU32 name_col = selected ? ImColor(1, 1, 1) : ImColor(Palette::Text);
    ImGui::PushStyleColor(ImGuiCol_Text, name_col);
    ImGui::PushTextWrapPos(pos.x + card_w - 8.0f);
    ImGui::TextUnformatted(skin.name.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();

    // Clickable area — applies immediately (consistent with gloves / music UX)
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("%s\nPaint Kit #%d", skin.name.c_str(), skin.Paint);
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + total_h + 6));
    ImGui::PopID();
}

// ── Wear & pattern controls ────────────────────────────────────────────────────

void SkinChangerPanel::DrawWearControls(SkinInfo_t& skin, bool& changed) noexcept {
    // Wear gradient bar
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 4.0f;

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

    // Wear slider
    ImGui::SetNextItemWidth(260.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, WearColor(skin.Wear));
    if (ImGui::SliderFloat("##wear", &skin.Wear, 0.0f, 1.0f, "%.4f"))
        changed = true;
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 10);
    ImGui::TextColored(WearColor(skin.Wear), "%s", WearLabel(skin.Wear));

    // Pattern seed
    ImGui::Spacing();
    ImGui::SetNextItemWidth(160.0f);
    int pattern = skin.Pattern;
    if (ImGui::InputInt("##pattern", &pattern, 1, 100)) {
        skin.Pattern = std::clamp(pattern, 0, 9999);
        changed = true;
    }
    ImGui::SameLine(0, 8);
    ImGui::TextColored(Palette::TextDim, "Pattern Seed");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("0\u20139999");
}

// ── Gloves tab ─────────────────────────────────────────────────────────────────

void SkinChangerPanel::DrawGlovesContent() noexcept {
    // Glove type chips
    ImGui::TextColored(Palette::TextDim, "Glove Type");
    ImGui::Spacing();

    float cx = ImGui::GetCursorPosX();
    float cw = ImGui::GetContentRegionAvail().x;
    float chip_x = cx;

    for (i32 i = 0; i < static_cast<i32>(std::size(kAllGloveDefs)); ++i) {
        auto name = GloveIdToString(static_cast<i32>(kAllGloveDefs[i]));
        bool sel = (i == selected_glove_type_idx_);

        ImVec2 txt_size = ImGui::CalcTextSize(name.c_str());
        float chip_w = txt_size.x + 20.0f;
        if (chip_x + chip_w > cx + cw && i > 0) break;

        ImGui::SetCursorPosX(chip_x);

        ImU32 bg = sel ? ImColor(Palette::Accent) : ImColor(Palette::Card);
        ImU32 bg_hov = sel ? ImColor(Palette::Accent) : ImColor(Palette::CardHover);
        ImU32 txt = sel ? ImColor(1, 1, 1) : ImColor(Palette::Text);

        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_hov);
        ImGui::PushStyleColor(ImGuiCol_Text, txt);

        char label[64];
        std::snprintf(label, sizeof(label), "%s##gv_%d", name.c_str(), i);
        if (ImGui::Button(label, ImVec2(chip_w, 28))) {
            selected_glove_type_idx_ = i;
            selected_glove_skin_idx_ = 0;
        }

        ImGui::PopStyleColor(3);
        chip_x += chip_w + 6.0f;
    }

    ImGui::Spacing();
    ImGui::TextColored(ct_selected_ ? Palette::TextGood : Palette::TextDim,
                       "Editing: %s Gloves", ct_selected_ ? "CT" : "T");
    ImGui::Separator();
    ImGui::Spacing();

    // Glove skin cards
    GlovesEnum glove = kAllGloveDefs[selected_glove_type_idx_];
    auto skins = db_.GetGloveSkins(glove);

    ImGui::SetNextItemWidth(220.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
    ImGui::InputTextWithHint("##glove_skin_filter", "Search glove skins...", &glove_skin_search_);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    std::vector<SkinInfo_t> filtered;
    for (auto& s : skins) {
        if (ContainsIgnoreCase(s.name, glove_skin_search_))
            filtered.push_back(s);
    }

    if (!filtered.empty())
        selected_glove_skin_idx_ = std::clamp<i32>(selected_glove_skin_idx_, 0,
            static_cast<i32>(filtered.size()) - 1);

    // Glove skin cards (table-based grid)
    {
        float avail_w = ImGui::GetContentRegionAvail().x;
        int cols = std::max(2, static_cast<int>(avail_w / 175.0f));
        if (cols > static_cast<int>(filtered.size())) cols = static_cast<int>(filtered.size());
        if (cols < 1) cols = 1;

        if (ImGui::BeginTable("##glove_grid", cols,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadInnerX |
                              ImGuiTableFlags_NoPadOuterX)) {
            for (i32 i = 0; i < static_cast<i32>(filtered.size()); ++i) {
                ImGui::TableNextColumn();
                ImGui::PushID(i + 1000);

                bool sel = (i == selected_glove_skin_idx_);
                float card_w = ImGui::GetContentRegionAvail().x;
                float preview_h = 72.0f, total_h = preview_h + 34.0f;

                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImColor col = PreviewColor(filtered[i].Paint);

                // Shadow
                dl->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1),
                                  ImVec2(pos.x + card_w + 1, pos.y + total_h + 1),
                                  ImColor(0, 0, 0, 80), 6.0f);

                // Card body
                bool hovered = ImGui::IsWindowHovered() &&
                               ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + card_w, pos.y + total_h));
                ImU32 card_bg = sel ? ImColor(Palette::Accent)
                                    : (hovered ? ImColor(Palette::CardHover) : ImColor(Palette::Card));
                dl->AddRectFilled(pos, ImVec2(pos.x + card_w, pos.y + total_h), card_bg, 6.0f);

                // Preview band
                dl->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2),
                                  ImVec2(pos.x + card_w - 2, pos.y + preview_h - 2), col, 4.0f);

                // Gradient overlay
                dl->AddRectFilledMultiColor(
                    ImVec2(pos.x + 2, pos.y + preview_h * 0.5f),
                    ImVec2(pos.x + card_w - 2, pos.y + preview_h - 2),
                    ImColor(0, 0, 0, 0), ImColor(0, 0, 0, 0),
                    ImColor(0, 0, 0, 120), ImColor(0, 0, 0, 120));

                // Paint ID badge
                float badge_w = ImGui::CalcTextSize("#00").x + 16.0f;
                dl->AddRectFilled(ImVec2(pos.x + card_w - badge_w - 4, pos.y + preview_h - 22),
                                  ImVec2(pos.x + card_w - 4, pos.y + preview_h - 4),
                                  ImColor(0, 0, 0, 150), 3.0f);
                ImGui::SetCursorScreenPos(ImVec2(pos.x + card_w - badge_w, pos.y + preview_h - 20));
                char badge[32];
                std::snprintf(badge, sizeof(badge), "#%d", filtered[i].Paint);
                ImGui::TextColored(ImVec4(1, 1, 1, 0.7f), "%s", badge);

                // Name
                ImGui::SetCursorScreenPos(ImVec2(pos.x + 8, pos.y + preview_h + 6));
                ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(1, 1, 1, 1) : Palette::Text);
                ImGui::PushTextWrapPos(pos.x + card_w - 8.0f);
                ImGui::TextUnformatted(filtered[i].name.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();

                // Clickable area
                ImGui::SetCursorScreenPos(pos);
                ImGui::InvisibleButton("##gcard", ImVec2(card_w, total_h));
                if (ImGui::IsItemClicked()) {
                    selected_glove_skin_idx_ = i;
                    SkinInfo_t gs{filtered[i].Paint, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum{}};
                    mgr_.SetTeamGloveSkin(gs, glove, ct_selected_);
                    SaveAndSync();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                    ImGui::SetTooltip("%s\nPaint Kit #%d", filtered[i].name.c_str(), filtered[i].Paint);
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    {
        i32 ct_active = mgr_.GetGlovePaint(true);
        i32 t_active  = mgr_.GetGlovePaint(false);
        ImGui::TextColored(Palette::TextDim, "Active:");
        ImGui::SameLine(0, 6);
        if (ct_active > 0) {
            ImGui::TextColored(Palette::TextGood, "CT #%d", ct_active);
            ImGui::SameLine(0, 10);
        }
        if (t_active > 0) {
            ImGui::TextColored(Palette::TextGood, "T #%d", t_active);
        }
        if (ct_active <= 0 && t_active <= 0) {
            ImGui::TextColored(Palette::TextDim, "None");
        }
    }
}

// ── Music tab ──────────────────────────────────────────────────────────────────

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

    ImGui::TextColored(Palette::TextDim, "Select Music Kit");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(260.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
    ImGui::InputTextWithHint("##music_search", "Search music kits...", &music_filter_);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    ImGui::BeginChild("##music_list", ImVec2(0, 0), false);
    for (i32 i = 0; i < kit_count; ++i) {
        if (!ContainsIgnoreCase(kits[i].name, music_filter_))
            continue;

        bool sel = (i == selected_music_idx_);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text, Palette::Accent);
        else ImGui::PushStyleColor(ImGuiCol_Text, Palette::Text);

        char label[128];
        std::snprintf(label, sizeof(label), "%s##mk_%d", kits[i].name, i);
        if (ImGui::Selectable(label, sel, ImGuiSelectableFlags_None)) {
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

// ── Config persistence ─────────────────────────────────────────────────────────

void SkinChangerPanel::SaveAndSync() noexcept {
    SaveConfig(mgr_, changer_.GetMusicKit());
}
