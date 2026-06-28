#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "EntitySystem.h"
#include "EconSystem.h"
#include "SkinDatabase.h"
#include <random>

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
        u64 local_player = GetLocalPlayer(mem_);
        if (!local_player) return false;

        u64 local_ctrl = GetLocalController(mem_);
        if (!local_ctrl) return false;

        // ── Music kit ─────────────────────────────────────────────────────
        u64 inv = mem_.Read<u64>(local_ctrl + Offsets::m_pInventoryServices);
        if (inv && music_kit_id_) {
            mem_.Write<u16>(inv + Offsets::m_unMusicID,
                            static_cast<u16>(music_kit_id_));
        }

        // ── Weapons ───────────────────────────────────────────────────────
        auto weapons = GetWeapons(mem_, local_player);
        bool applied = false;

        // Force update prep: reset all weapons so they get re-processed
        if (mgr_.NeedsUpdate()) {
            for (u64 weapon : weapons) {
                u64 it = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
                mem_.Write<u32>(it + Offsets::m_iItemIDHigh, 0);
            }
        }

        for (u64 weapon : weapons) {
            u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;

            // Check if already applied
            if (mem_.Read<u32>(item + Offsets::m_iItemIDHigh) == 0xFFFFFFFF)
                continue;

            i32 cur_def = mem_.Read<u16>(item + Offsets::m_iItemDefinitionIndex);
            WeaponsEnum def = static_cast<WeaponsEnum>(cur_def);

            // ── Knife handling ─────────────────────────────────────────────
            bool is_knife = mgr_.IsKnifeDef(cur_def);
            i32  knife_def  = static_cast<i32>(mgr_.GetKnifeDef());
            i32  knife_pnt  = mgr_.GetKnifePaint();

            SkinInfo_t skin = is_knife && knife_pnt
                ? SkinInfo_t{knife_pnt, 0.01f, 0, false, "Knife", WeaponsEnum::None}
                : mgr_.GetSkin(def);

            if (!skin.Paint) continue;

            // Mark as applied
            mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);

            // Change knife def index if requested
            if (is_knife && knife_def > 0 && knife_def != cur_def) {
                mem_.Write<u16>(item + Offsets::m_iItemDefinitionIndex,
                                static_cast<u16>(knife_def));
            }

            // Write paint kit
            mem_.Write<u32>(weapon + Offsets::m_nFallbackPaintKit,
                            static_cast<u32>(skin.Paint));

            // Set mesh mask for model display
            u64 node = mem_.Read<u64>(weapon + Offsets::m_pGameSceneNode);
            if (node) {
                u64 model = node + Offsets::m_modelState;
                u64 dirty = mem_.Read<u64>(model + Offsets::m_pDirtyModelData);
                if (dirty) {
                    u64 mask = skin.bUsesOldModel ? 2 : 1;
                    mem_.Write<u64>(dirty + Offsets::m_DirtyMeshGroupMask, mask);
                    mem_.Write<u64>(model + Offsets::m_MeshGroupMask, mask);
                }

                // Also update HUD weapon
                u64 hud = GetHudWeapon(mem_, weapon);
                if (hud) {
                    u64 hud_node = mem_.Read<u64>(hud + Offsets::m_pGameSceneNode);
                    if (hud_node) {
                        u64 hud_model = hud_node + Offsets::m_modelState;
                        mem_.Write<u64>(hud_model + Offsets::m_MeshGroupMask, mask);
                    }
                }
            }

            // Write econ attributes
            attr_mgr_.Create(item, skin);

            applied = true;
        }

        // ── Gloves ────────────────────────────────────────────────────────
        if (ApplyGloves()) applied = true;

        if (applied || mgr_.NeedsUpdate()) {
            // Call RegenerateWeaponSkins
            u64 regen_addr = mem_.SigScan(L"client.dll",
                                           Offsets::Sigs::RegenerateWeaponSkins);
            if (regen_addr) {
                // Fire-and-forget — no blocking, no lag spike
                mem_.CallRemoteThreadAsync(regen_addr);
            }

            // Timing jitter: 1–15ms random sleep to avoid fixed patterns
            {
                std::uniform_int_distribution<int> dist(1, 15);
                ::Sleep(dist(rng_));
            }

            // Cleanup fallback values
            for (u64 weapon : weapons) {
                u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
                i32 paint = mem_.Read<i32>(weapon + Offsets::m_nFallbackPaintKit);
                if (paint == -1) continue;
                mem_.Write<u32>(weapon + Offsets::m_nFallbackPaintKit, 0xFFFFFFFF);
                // Remove our econ items to let the game use them
                attr_mgr_.Remove(item);
            }

            mgr_.ClearUpdate();
        }

        return applied;
    }

    /// Apply glove skin from the manager.
    bool ApplyGloves() noexcept {
        u64 local_player = GetLocalPlayer(mem_);
        if (!local_player) return false;

        i32 glove_paint = mgr_.GetGlovePaint();
        if (!glove_paint) return false;

        // Read the glove econ item — m_EconGloves is an embedded C_EconGloves
        // that contains at least a C_EconItemView (or a handle to one).
        u64 glove_data = local_player + Offsets::m_EconGloves;

        // Try to resolve as entity handle first (common approach)
        u32 handle = mem_.Read<u32>(glove_data);
        u64 glove_entity = 0;
        if (handle > 0 && handle < 0x10000)
            glove_entity = GetEntityByHandle(mem_, handle);

        // Fallback: direct pointer read
        if (!glove_entity) {
            glove_entity = mem_.Read<u64>(glove_data);
            if (glove_entity < 0x10000) // still looks like a handle, skip
                glove_entity = 0;
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
            u64 node = mem_.Read<u64>(hud_arms + Offsets::m_pGameSceneNode);
            if (node) {
                u64 model = node + Offsets::m_modelState;
                mem_.Write<u64>(model + Offsets::m_MeshGroupMask, 1);
            }
        }

        // Force re-apply
        mem_.Write<bool>(local_player + Offsets::m_bNeedToReApplyGloves, true);

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
    mutable std::mt19937 rng_{std::random_device{}()};

    // Override mesh mask with retry (mirrors reference's goto approach)
    void OverrideMeshMask(u64 model, u64 mask) noexcept {
        for (int attempt = 0; attempt < 5; ++attempt) {
            for (int i = 0; i < 700; ++i) {
                mem_.Write<u64>(model + Offsets::m_MeshGroupMask, mask);
            }
            ::Sleep(5);
            if (mem_.Read<u64>(model + Offsets::m_MeshGroupMask) == mask)
                break;
        }
    }
};
