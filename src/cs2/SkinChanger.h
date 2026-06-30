#pragma once

#include "core/ProcessMemory.h"
#include "Offsets.h"
#include "EntitySystem.h"
#include "EconSystem.h"
#include "SkinDatabase.h"
#include <random>
#include <chrono>

/* ── VeneeR Skin Changer ────────────────────────────────────────────────────
 *
 * Core skin-changing logic executed once per frame.
 *
 * On each tick:
 *   1. Read local player, inventory, and weapons.
 *   2. For each weapon, check if a skin is selected.
 *   3. Apply the skin by writing paint kit, item ID, mesh mask, and econ attrs.
 *   4. Set m_iItemIDHigh to -1 so the game uses fallback values.
 *   5. Call RegenerateWeaponSkins on the econ item to force a visual refresh.
 * ──────────────────────────────────────────────────────────────────────────── */

class SkinChanger {
public:
    SkinChanger(ProcessMemory& mem, SkinDatabase& db, SkinManager& mgr)
        : mem_(mem), db_(db), mgr_(mgr), attr_mgr_(mem) {}

    /// Called every frame.  Returns true if a skin was applied.
    bool Tick() noexcept;

    /// Apply glove skin from the manager.
    bool ApplyGloves() noexcept;

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
    u32                 account_id_ = 0;
    std::chrono::steady_clock::time_point last_tick_{};
    mutable std::mt19937 rng_{std::random_device{}()};

    /// Read the player's Steam account ID from any existing weapon's item view.
    u32 ResolveAccountId() noexcept;

    /// Write mesh group mask to the weapon's scene node and its HUD counterpart.
    void DoMeshMask(u64 weapon, u64 node, bool old_model) noexcept;
};
