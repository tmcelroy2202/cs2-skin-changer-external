#pragma once

#include "imgui.h"
#include "ui/Panels.h"
#include "cs2/SkinDatabase.h"
#include "cs2/SkinChanger.h"
#include "cs2/ConfigManager.h"
#include "cs2/SkinTextureCache.h"

class SkinChangerPanel final : public Panel {
public:
    SkinChangerPanel(SkinDatabase& db, SkinManager& mgr, SkinChanger& changer) noexcept
        : Panel("VeneeR"), db_(db), mgr_(mgr), changer_(changer) {}

    std::string_view GetId() const noexcept override { return "SkinChanger"; }
    void Draw() noexcept override;

    void SetEquippedWeapon(std::string_view name, i32 def_id) noexcept {
        equipped_weapon_ = name;
        equipped_weapon_id_ = def_id;
    }

    void SetTextureCache(SkinTextureCache* cache) noexcept { tex_cache_ = cache; }

private:
    SkinDatabase&  db_;
    SkinManager&   mgr_;
    SkinChanger&   changer_;
    SkinTextureCache* tex_cache_ = nullptr;

    enum class Category { Pistols, Rifles, Smgs, Snipers, Heavy, Knives, Gloves, Music };
    Category active_cat_ = Category::Rifles;

    bool ct_selected_ = true;  // true = CT config, false = T config

    i32 selected_weapon_idx_ = 0;
    i32 selected_skin_idx_  = 0;
    i32 selected_glove_type_idx_ = 0;
    i32 selected_glove_skin_idx_ = 0;
    i32 selected_music_idx_ = 0;

    std::string weapon_filter_;
    std::string skin_filter_;
    std::string glove_skin_search_;
    std::string music_filter_;

    std::string equipped_weapon_;
    i32 equipped_weapon_id_ = 0;

    std::vector<WeaponsEnum> cat_weapons_;

    void DrawTitleBar() noexcept;
    void DrawEquippedBar() noexcept;
    void DrawCategories() noexcept;
    void DrawSkinsContent() noexcept;
    void DrawGlovesContent() noexcept;
    void DrawMusicContent() noexcept;
    void DrawSkinCards(std::span<const SkinInfo_t> skins, WeaponsEnum weapon, bool is_knife) noexcept;
    void DrawSkinCard(i32 idx, const SkinInfo_t& skin, bool selected, WeaponsEnum weapon, bool is_knife) noexcept;
    void DrawWearControls(SkinInfo_t& skin, bool& changed) noexcept;
    void SaveAndSync() noexcept;

    static ImColor PreviewColor(i32 paint_id) noexcept;
    static const char* CategoryLabel(Category c) noexcept;
    static std::vector<WeaponsEnum> WeaponsForCategory(Category c) noexcept;
};
