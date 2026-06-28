#pragma once

#include "core/ProcessMemory.h"
#include "core/Logging.h"
#include "Offsets.h"
#include "EntitySystem.h"
#include "EconSystem.h"
#include "SkinDatabase.h"
#include <random>
#include <chrono>
#include <thread>

/* ── CS2 Skin Changer ──────────────────────────────────────────────────────
 *
 * External skin changer loop (lunaR-style flow):
 *   1. Enumerate local player weapons.
 *   2. For configured weapons, force fallback mode (item IDs = -1).
 *   3. Write paint kit / wear / seed + econ attributes + mesh mask.
 *   4. Call RegenerateWeaponSkins (patched for external use).
 *   5. Clean up fallback writes so the game keeps attributes only.
 * ──────────────────────────────────────────────────────────────────────────── */

class SkinChanger {
public:
    SkinChanger(ProcessMemory& mem, SkinDatabase& db, SkinManager& mgr)
        : mem_(mem), db_(db), mgr_(mgr), attr_mgr_(mem) {}

    bool Tick() noexcept {
        auto now = std::chrono::steady_clock::now();
        if (!mgr_.NeedsUpdate() &&
            now - last_tick_ < std::chrono::milliseconds(50))
            return false;

        auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
        if (!local_player || !*local_player) return false;
        auto local_ctrl = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerController);
        if (!local_ctrl || !*local_ctrl) return false;

        u64 local_player_val = *local_player;
        u64 local_ctrl_val = *local_ctrl;

        // ── Music kit ────────────────────────────────────────────────────
        auto inv = mem_.ReadSafe<u64>(local_ctrl_val + Offsets::m_pInventoryServices);
        if (inv && *inv && music_kit_id_ != last_music_kit_id_) {
            mem_.Write<u16>(*inv + Offsets::m_unMusicID,
                            static_cast<u16>(music_kit_id_));
            last_music_kit_id_ = music_kit_id_;
        }

        EnsureRegenReady();

        auto weapons = GetWeapons(mem_, local_player_val);
        bool applied = false;
        std::vector<std::pair<u64, u64>> processed_pairs;

        if (mgr_.NeedsUpdate()) {
            for (u64 weapon : weapons) {
                u64 it = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
                mem_.Write<u32>(it + Offsets::m_iItemIDHigh, 0);
            }
        }

        for (u64 weapon : weapons) {
            u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;

            auto item_id = mem_.ReadSafe<u32>(item + Offsets::m_iItemIDHigh);
            if (!item_id || *item_id == 0xFFFFFFFF)
                continue;

            auto cur_def_raw = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
            if (!cur_def_raw) continue;
            i32 cur_def = *cur_def_raw;
            WeaponsEnum def = static_cast<WeaponsEnum>(cur_def);

            bool is_knife = mgr_.IsKnifeDef(cur_def);
            i32  knife_def  = static_cast<i32>(mgr_.GetKnifeDef());
            i32  knife_pnt  = mgr_.GetKnifePaint();

            SkinInfo_t skin = is_knife && knife_pnt
                ? SkinInfo_t{knife_pnt, 0.01f, 0, false, "Knife", WeaponsEnum::None}
                : mgr_.GetSkin(def);

            if (!skin.Paint) {
                mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
                continue;
            }

            PrepareItemView(item);

            u64 mask = skin.bUsesOldModel ? 2 : 1;

            auto do_paint = [&]() {
                mem_.Write<u32>(weapon + Offsets::m_nFallbackPaintKit,
                                static_cast<u32>(skin.Paint));
                mem_.Write<f32>(weapon + Offsets::m_flFallbackWear, skin.Wear);
                mem_.Write<u32>(weapon + Offsets::m_nFallbackSeed,
                                static_cast<u32>(skin.Pattern));
                if (is_knife && knife_def > 0 && knife_def != cur_def)
                    mem_.Write<u16>(item + Offsets::m_iItemDefinitionIndex,
                                    static_cast<u16>(knife_def));
            };
            auto do_mesh = [&]() {
                auto n = mem_.ReadSafe<u64>(weapon + Offsets::m_pGameSceneNode);
                if (n && *n) SetMeshGroupMask(weapon, *n, mask);
                u64 hud = GetHudWeapon(mem_, local_player_val, weapon);
                if (hud) {
                    auto hud_node = mem_.ReadSafe<u64>(hud + Offsets::m_pGameSceneNode);
                    if (hud_node && *hud_node)
                        SetMeshGroupMask(hud, *hud_node, mask);
                }
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

        if (ApplyGloves(local_player_val)) applied = true;

        if (applied || mgr_.NeedsUpdate()) {
            if (regen_addr_)
                mem_.CallRemoteThreadAsync(regen_addr_, nullptr);

            {
                std::uniform_int_distribution<int> dist(1, 15);
                ::Sleep(dist(rng_));
            }

            for (auto& [weapon_base, item_addr] : processed_pairs) {
                auto paint = mem_.ReadSafe<u32>(weapon_base + Offsets::m_nFallbackPaintKit);
                if (!paint || *paint == 0xFFFFFFFF) continue;

                mem_.Write<u32>(weapon_base + Offsets::m_nFallbackPaintKit, 0xFFFFFFFF);
                mem_.Write<f32>(weapon_base + Offsets::m_flFallbackWear, 0.0f);
                mem_.Write<u32>(weapon_base + Offsets::m_nFallbackSeed, 0);
                attr_mgr_.Remove(item_addr);
            }

            mgr_.ClearUpdate();
            last_tick_ = now;
        }

        return applied;
    }

    bool ApplyGloves(u64 local_player) noexcept {
        if (!local_player) return false;

        i32 glove_paint = mgr_.GetGlovePaint();
        if (!glove_paint) return false;

        u64 glove_data = local_player + Offsets::m_EconGloves;

        auto handle = mem_.ReadSafe<u32>(glove_data);
        u64 glove_entity = 0;
        if (handle && *handle > 0 && *handle < 0x10000)
            glove_entity = GetEntityByHandle(mem_, *handle);

        if (!glove_entity) {
            auto direct = mem_.ReadSafe<u64>(glove_data);
            if (direct && *direct >= 0x10000)
                glove_entity = *direct;
        }

        if (!glove_entity) return false;

        u64 item = glove_entity + Offsets::m_AttributeManager + Offsets::m_Item;
        PrepareItemView(item);

        SkinInfo_t glove_skin{glove_paint, 0.01f, 0, false, "Glove", WeaponsEnum::None};
        attr_mgr_.Create(item, glove_skin);

        u64 hud_arms = GetHudArms(mem_);
        if (hud_arms) {
            auto node = mem_.ReadSafe<u64>(hud_arms + Offsets::m_pGameSceneNode);
            if (node && *node)
                SetMeshGroupMask(hud_arms, *node, 1);
        }

        mem_.Write<bool>(local_player + Offsets::m_bNeedToReApplyGloves, true);
        return true;
    }

    void SetMusicKit(u16 id) noexcept { music_kit_id_ = id; }
    u16 GetMusicKit() const noexcept { return music_kit_id_; }

private:
    ProcessMemory&      mem_;
    SkinDatabase&       db_;
    SkinManager&        mgr_;
    EconItemAttributeManager attr_mgr_;
    u16                 music_kit_id_ = 0;
    u16                 last_music_kit_id_ = 0;
    u64                 regen_addr_ = 0;
    bool                regen_patched_ = false;
    std::chrono::steady_clock::time_point last_tick_{};
    mutable std::mt19937 rng_{std::random_device{}()};

    void EnsureRegenReady() noexcept {
        if (regen_addr_) return;

        regen_addr_ = mem_.SigScan(L"client.dll", Offsets::Sigs::RegenerateWeaponSkins);
        if (!regen_addr_) {
            LOG_WARN("SigScan for RegenerateWeaponSkins failed");
            return;
        }

        // External changers patch the immediate inside RegenerateWeaponSkins so it
        // reads the attribute list at the correct weapon offset.
        if (!regen_patched_) {
            mem_.Write<u16>(regen_addr_ + 0x52, Offsets::WeaponAttributeListOffset());
            regen_patched_ = true;
            LOG_INFO("RegenerateWeaponSkins patched at 0x{:X}", regen_addr_);
        }
    }

    void PrepareItemView(u64 item) noexcept {
        mem_.Write<bool>(item + Offsets::m_bInitialized, false);
        mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
        mem_.Write<u32>(item + Offsets::m_iItemIDLow, 0xFFFFFFFF);
        mem_.Write<bool>(item + Offsets::m_bDisallowSOC, true);
    }

    void SetMeshGroupMask(u64 entity, u64 scene_node, u64 mask) noexcept {
        u64 model_state = scene_node + Offsets::m_modelState;

        auto dirty = mem_.ReadSafe<u64>(model_state + Offsets::m_pDirtyModelData);
        if (dirty && *dirty)
            mem_.Write<u64>(*dirty + Offsets::m_DirtyMeshGroupMask, mask);

        for (int attempt = 0; attempt < 8; ++attempt) {
            mem_.Write<u64>(model_state + Offsets::m_MeshGroupMask, mask);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            auto current = mem_.ReadSafe<u64>(model_state + Offsets::m_MeshGroupMask);
            if (current && *current == mask)
                break;
        }
    }
};
