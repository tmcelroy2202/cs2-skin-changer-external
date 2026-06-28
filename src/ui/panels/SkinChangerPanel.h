#pragma once

#include "ui/Panels.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/ConfigManager.h"

/* ── SkinChangerPanel ──────────────────────────────────────────────────────
 *
 * ImGui panel for selecting weapon skins.  Mirrors the reference's menu.h
 * with weapon tabs, skin combo lists, and music kit selection.
 * ──────────────────────────────────────────────────────────────────────────── */

class SkinChangerPanel final : public Panel {
public:
    SkinChangerPanel(SkinDatabase& db, SkinManager& mgr, SkinChanger& changer) noexcept
        : Panel("CS2 Skin Changer")
        , db_(db), mgr_(mgr), changer_(changer) {}

    std::string_view GetId() const noexcept override { return "SkinChanger"; }

    void Draw() noexcept override;

private:
    SkinDatabase&  db_;
    SkinManager&   mgr_;
    SkinChanger&   changer_;

    enum class Tab { Skins, MusicKit, Gloves };
    Tab active_tab_ = Tab::Skins;

    i32      selected_weapon_idx_ = 0;
    i32      selected_skin_idx_  = 0;
    i32      selected_music_idx_ = 0;
    i32      selected_glove_type_idx_ = 0;
    i32      selected_glove_skin_idx_ = 0;

    // Currently equipped weapon (set externally from entity system)
    std::string equipped_weapon_;
    i32         equipped_weapon_id_ = 0;

    // Cached weapon/glove lists for combos
    std::vector<WeaponsEnum> weapon_list_;
    std::vector<GlovesEnum>  glove_list_;

    /// Set the currently equipped weapon (called from main loop).
    void SetEquippedWeapon(std::string_view name, i32 def_id) noexcept {
        equipped_weapon_ = name;
        equipped_weapon_id_ = def_id;
    }

    void DrawSkinsTab() noexcept;
    void DrawMusicTab() noexcept;
    void DrawGlovesTab() noexcept;
    void DrawWeaponCombo() noexcept;
    void DrawSkinCombo(WeaponsEnum weapon) noexcept;
    void DrawGloveTypeCombo() noexcept;
    void DrawGloveSkinCombo(GlovesEnum glove) noexcept;
    void DrawFloatPatternControls() noexcept;
    void SaveAndSync() noexcept;
};
