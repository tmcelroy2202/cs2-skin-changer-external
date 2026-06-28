#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "EntitySystem.h"
#include "EconSystem.h"
#include "SkinDatabase.h"
#include <random>
#include <chrono>

/* ── CS2 Skin Changer ──────────────────────────────────────────────────────
 *
 * Port of the reference's main skin-changing logic.
 *
 * On each tick:
 *   1. Read local player, inventory, and weapons.
 *   2. For each weapon, check if a skin is selected.
 *   3. Apply the skin by writing paint kit, item ID, mesh mask, and econ attrs.
 *   4. Call RegenerateWeaponSkins if needed.
 * ──────────────────────────────────────────────────────────────────────────── */

class SkinChanger {
public:
    SkinChanger(ProcessMemory& mem, SkinDatabase& db, SkinManager& mgr)
        : mem_(mem), db_(db), mgr_(mgr), attr_mgr_(mem) {}

    /// Called every frame.  Returns true if a skin was applied.
    bool Tick() noexcept {
        auto now = std::chrono::steady_clock::now();
        if (!mgr_.NeedsUpdate() &&
            now - last_tick_ < std::chrono::milliseconds(50))
            return false;

        // Read local player and controller atomically to avoid race
        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;
        auto local_ctrl = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerController);
        if (!local_ctrl || !*local_ctrl) return false;

        u64 local_player_val = *local_player;
        u64 local_ctrl_val = *local_ctrl;

        // ── Music kit (only write when changed) ──────────────────────────
        auto inv = mem_.ReadSafe<u64>(local_ctrl_val + Offsets::m_pInventoryServices);
        if (inv && *inv && music_kit_id_ != last_music_kit_id_) {
            mem_.Write<u16>(*inv + Offsets::m_unMusicID,
                            static_cast<u16>(music_kit_id_));
            last_music_kit_id_ = music_kit_id_;
        }

        // ── Weapons ───────────────────────────────────────────────────────
        auto weapons = GetWeapons(mem_, local_player_val);
        bool applied = false;

        // Force update prep: reset all weapons so they get re-processed
        if (mgr_.NeedsUpdate()) {
            for (u64 weapon : weapons) {
                u64 it = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
                mem_.Write<u32>(it + Offsets::m_iItemIDHigh, 0);
            }
        }

        std::vector<std::pair<u64, u64>> processed_pairs;

        for (u64 weapon : weapons) {
            u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;

            // Check if already applied
            auto item_id = mem_.ReadSafe<u32>(item + Offsets::m_iItemIDHigh);
            if (!item_id || *item_id == 0xFFFFFFFF)
                continue;

            auto cur_def_raw = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
            if (!cur_def_raw) continue;
            i32 cur_def = *cur_def_raw;
            WeaponsEnum def = static_cast<WeaponsEnum>(cur_def);

            // ── Knife handling ─────────────────────────────────────────────
            bool is_knife = mgr_.IsKnifeDef(cur_def);
            i32  knife_def  = static_cast<i32>(mgr_.GetKnifeDef());
            i32  knife_pnt  = mgr_.GetKnifePaint();

            SkinInfo_t skin = is_knife && knife_pnt
                ? SkinInfo_t{knife_pnt, 0.01f, 0, false, "Knife", WeaponsEnum::None}
                : mgr_.GetSkin(def);

            // No skin selected for this weapon — mark to prevent re-processing
            if (!skin.Paint) {
                mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
                continue;
            }

            // Mark as applied
            mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);

            // Randomize write order among 3 groups (6 permutations)
            auto do_paint = [&]() {
                mem_.Write<u32>(weapon + Offsets::m_nFallbackPaintKit,
                                static_cast<u32>(skin.Paint));
                if (is_knife && knife_def > 0 && knife_def != cur_def)
                    mem_.Write<u16>(item + Offsets::m_iItemDefinitionIndex,
                                    static_cast<u16>(knife_def));
            };
            auto do_mesh = [&]() {
                auto n = mem_.ReadSafe<u64>(weapon + Offsets::m_pGameSceneNode);
                if (n && *n) DoMeshMask(weapon, *n, skin.bUsesOldModel);
            };
            auto do_attrs = [&]() { attr_mgr_.Create(item, skin); };

            switch (std::uniform_int_distribution<int>(0, 5)(rng_)) {
            case 0: do_paint(); do_mesh();  do_attrs(); break;
            case 1: do_paint(); do_attrs(); do_mesh();  break;
            case 2: do_mesh();  do_paint(); do_attrs(); break;
            case 3: do_mesh();  do_attrs(); do_paint(); break;
            case 4: do_attrs(); do_paint(); do_mesh();  break;
            case 5: do_attrs(); do_mesh();  do_paint(); break;
            }

            processed_pairs.push_back({weapon, item});
            applied = true;
        }

        // ── Gloves ────────────────────────────────────────────────────────
        if (ApplyGloves()) applied = true;

        if (applied || mgr_.NeedsUpdate()) {
            // Cache RegenerateWeaponSkins address after first scan
            if (!regen_addr_) {
                regen_addr_ = mem_.SigScan(L"client.dll",
                                           Offsets::Sigs::RegenerateWeaponSkins);
            }
            if (regen_addr_ && inv && *inv) {
                // Pass inventory services pointer as this-context
                mem_.CallRemoteThreadAsync(regen_addr_,
                    reinterpret_cast<void*>(static_cast<u64>(*inv)));
            }

            // Timing jitter: 1–15ms random sleep to avoid fixed patterns
            {
                std::uniform_int_distribution<int> dist(1, 15);
                ::Sleep(dist(rng_));
            }

            // Cleanup only weapons we actually touched
            for (auto& [weapon_base, item_addr] : processed_pairs) {
                mem_.Write<u32>(weapon_base + Offsets::m_nFallbackPaintKit, 0xFFFFFFFF);
                attr_mgr_.Remove(item_addr);
            }

            mgr_.ClearUpdate();
            last_tick_ = now;
        }

        return applied;
    }

    /// Apply glove skin from the manager.
    bool ApplyGloves() noexcept {
        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;

        i32 glove_paint = mgr_.GetGlovePaint();
        if (!glove_paint) return false;

        u64 glove_data = *local_player + Offsets::m_EconGloves;

        // Try to resolve as entity handle first (common approach)
        auto handle = mem_.ReadSafe<u32>(glove_data);
        u64 glove_entity = 0;
        if (handle && *handle > 0 && *handle < 0x10000)
            glove_entity = GetEntityByHandle(mem_, *handle);

        // Fallback: direct pointer read
        if (!glove_entity) {
            auto direct = mem_.ReadSafe<u64>(glove_data);
            if (direct && *direct >= 0x10000)
                glove_entity = *direct;
        }

        if (!glove_entity) return false;

        // Same treat as weapon econ item
        u64 item = glove_entity + Offsets::m_AttributeManager + Offsets::m_Item;

        // Set item ID high to mark as applied
        mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);

        // Create econ attributes for paint kit
        SkinInfo_t glove_skin{glove_paint, 0.01f, 0, false, "Glove", WeaponsEnum::None};
        attr_mgr_.Create(item, glove_skin);

        // Set mesh mask for HUD arms
        u64 hud_arms = GetHudArms(mem_);
        if (hud_arms) {
            auto node = mem_.ReadSafe<u64>(hud_arms + Offsets::m_pGameSceneNode);
            if (node && *node) {
                u64 model = *node + Offsets::m_modelState;
                mem_.Write<u64>(model + Offsets::m_MeshGroupMask, 1);
            }
        }

        // Force re-apply
        mem_.Write<bool>(*local_player + Offsets::m_bNeedToReApplyGloves, true);

        return true;
    }

    /// Set music kit ID (0 = default).
    void SetMusicKit(u16 id) noexcept { music_kit_id_ = id; }

    /// Get current music kit ID.
    u16 GetMusicKit() const noexcept { return music_kit_id_; }

private:
    ProcessMemory&      mem_;
    SkinDatabase&       db_;
    SkinManager&        mgr_;
    EconItemAttributeManager attr_mgr_;
    u16                 music_kit_id_ = 0;
    u16                 last_music_kit_id_ = 0;
    u64                 regen_addr_ = 0;
    std::chrono::steady_clock::time_point last_tick_{};
    mutable std::mt19937 rng_{std::random_device{}()};

    void DoMeshMask(u64 weapon, u64 node, bool old_model) noexcept {
        u64 model = node + Offsets::m_modelState;
        auto dirty = mem_.ReadSafe<u64>(model + Offsets::m_pDirtyModelData);
        u64 mask = old_model ? 2 : 1;
        if (dirty && *dirty) {
            mem_.Write<u64>(*dirty + Offsets::m_DirtyMeshGroupMask, mask);
            mem_.Write<u64>(model + Offsets::m_MeshGroupMask, mask);
        }
        u64 hud = GetHudWeapon(mem_, weapon);
        if (hud) {
            auto hud_node = mem_.ReadSafe<u64>(hud + Offsets::m_pGameSceneNode);
            if (hud_node && *hud_node)
                mem_.Write<u64>(*hud_node + Offsets::m_modelState + Offsets::m_MeshGroupMask, mask);
        }
    }

    // Override mesh mask with retry (limited writes)
    void OverrideMeshMask(u64 model, u64 mask) noexcept {
        for (int attempt = 0; attempt < 3; ++attempt) {
            for (int i = 0; i < 20; ++i) {
                mem_.Write<u64>(model + Offsets::m_MeshGroupMask, mask);
            }
            ::Sleep(5);
            auto current = mem_.ReadSafe<u64>(model + Offsets::m_MeshGroupMask);
            if (current && *current == mask)
                break;
        }
    }
};
