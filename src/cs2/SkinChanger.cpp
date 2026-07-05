#include "SkinChanger.h"
#include "ui/panels/DebugPanel.h"
#include "core/Logging.h"

// ── Team helpers ────────────────────────────────────────────────────────────────
enum Team : u8 { Team_None = 0, Team_Spec = 1, Team_T = 2, Team_CT = 3 };

static Team ReadTeam(ProcessMemory& mem, u64 local_player) {
    auto raw = mem.ReadSafe<u8>(local_player + Offsets::m_iTeamNum);
    if (!raw) return Team_None;
    return static_cast<Team>(*raw);
}

// ── Write helper with verification ──────────────────────────────────────────
template <typename T>
static bool WriteVerify(ProcessMemory& mem, u64 addr, const T& val,
                        const char* name, u64 weapon_addr) {
    if (!mem.Write<T>(addr, val)) {
        LOG_ERROR("WriteVerify FAILED: {} at 0x{:X} (weapon=0x{:X})", name, addr, weapon_addr);
        return false;
    }
    auto readback = mem.ReadSafe<T>(addr);
    if (!readback || *readback != val) {
        // Use static_cast to int for hex format — handles u8/u16/u32/u64/f32 safely
        LOG_WARN("WriteVerify MISMATCH: {} at 0x{:X}", name, addr);
        return false;
    }
    return true;
}

bool SkinChanger::Tick() noexcept {
    auto now = std::chrono::steady_clock::now();
    if (!mgr_.NeedsUpdate() &&
        now - last_tick_ < std::chrono::milliseconds(1))
        return false;

    auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_player || !*local_player) {
        LOG_WARN("SkinChanger::Tick — no local player pawn");
        if (debug_) debug_->ReportError("No local player pawn (dwLocalPlayerPawn offset wrong?)");
        return false;
    }
    auto local_ctrl = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerController);
    if (!local_ctrl || !*local_ctrl) {
        LOG_WARN("SkinChanger::Tick — no local player controller");
        if (debug_) debug_->ReportError("No local player controller (dwLocalPlayerController offset wrong?)");
        return false;
    }

    u64 local_player_val = *local_player;
    u64 local_ctrl_val = *local_ctrl;

    Team team = ReadTeam(mem_, local_player_val);
    bool is_ct = (team == Team_CT);
    if (team == Team_None && !logged_team_warn_) {
        LOG_WARN("SkinChanger::Tick — m_iTeamNum (off 0x{:X}) returned 0 — team detection broken",
                 Offsets::m_iTeamNum);
        if (debug_) debug_->ReportError("m_iTeamNum=0 — team offset wrong?");
        logged_team_warn_ = true;
    }

    // Retry account ID resolution every tick until we get one
    if (!account_id_) {
        account_id_ = ResolveAccountId();
        if (!account_id_ && mgr_.NeedsUpdate())
            LOG_WARN("SkinChanger::Tick — account ID not yet resolved (no valid weapon items)");
    }

    // Music kit — write every tick to survive round restarts
    auto inv = mem_.ReadSafe<u64>(local_ctrl_val + Offsets::m_pInventoryServices);
    if (inv && *inv && music_kit_id_ != 0) {
        WriteVerify(mem_, *inv + Offsets::m_unMusicID,
                    static_cast<u16>(music_kit_id_), "m_unMusicID", *inv);
    }

    // Don't reset cached_glove_entity_ every tick — re-finding is expensive.
    // Force re-find only on NeedsUpdate to handle stale entities from round restarts.

    // Weapons
    auto weapons = GetWeapons(mem_, local_player_val);
    // Fallback: if the weapon vector is empty, try the active weapon directly
    if (weapons.empty()) {
        u64 active = GetActiveWeapon(mem_, local_player_val);
        if (active) weapons.push_back(active);
    }
    bool applied = false;

    for (u64 weapon : weapons) {
        // m_AttributeManager and m_Item are EMBEDDED structs (not pointers).
        // Compute the item view address as a flat offset chain.
        u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;

        auto cur_def_raw = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
        if (!cur_def_raw) continue;
        i32 cur_def = *cur_def_raw;
        WeaponsEnum def = static_cast<WeaponsEnum>(cur_def);

        bool is_knife = mgr_.IsKnifeDef(cur_def);
        i32  knife_def  = static_cast<i32>(mgr_.GetKnifeDef(is_ct));
        i32  knife_pnt  = mgr_.GetKnifePaint(is_ct);

        // Apply knife model change BEFORE the paint check — model and paint are
        // independent.  The item-definition-index write works even when no paint
        // is configured (so the model swap still takes effect).
        if (is_knife && knife_def > 0 && knife_def != cur_def) {
            if (!WriteVerify(mem_, item + Offsets::m_iItemDefinitionIndex,
                             static_cast<u16>(knife_def), "m_iItemDefinitionIndex (knife)", weapon))
                LOG_WARN("Tick: knife model write FAILED for weapon=0x{:X}", weapon);
        }

        SkinInfo_t skin = is_knife && knife_pnt
            ? SkinInfo_t{knife_pnt, mgr_.GetKnifeWear(is_ct), mgr_.GetKnifePattern(is_ct),
                         -1, "", false, "Knife", WeaponsEnum::None}
            : mgr_.GetSkin(def);

        if (!skin.Paint) continue;

        // Log once per weapon on first write to diagnose offset issues
        if (!logged_write_fail_) {
            LOG_INFO("Tick: weapon=0x{:X} item=0x{:X} paint={} def={}",
                     weapon, item, skin.Paint, cur_def);
            logged_write_fail_ = true;
        }

        // ── CRITICAL: Write m_bInitialized = false FIRST ──────────────────
        // CS2 checks this flag BEFORE reading fallback values. If it's true,
        // CS2 skips all fallback fields and uses real inventory data instead.
        // Must be written BEFORE fallback values so CS2 sees it in the same pass.
        WriteVerify(mem_, item + Offsets::m_bInitialized,
                    false, "m_bInitialized", weapon);
        WriteVerify(mem_, item + Offsets::m_bRestoreCustomMaterialAfterPrecache,
                    false, "m_bRestoreCustomMaterialAfterPrecache", weapon);

        // ── Fallback values ───────────────────────────────────────────────
        WriteVerify(mem_, weapon + Offsets::m_nFallbackPaintKit,
                    static_cast<u32>(skin.Paint), "m_nFallbackPaintKit", weapon);
        WriteVerify(mem_, weapon + Offsets::m_flFallbackWear,
                    skin.Wear, "m_flFallbackWear", weapon);
        WriteVerify(mem_, weapon + Offsets::m_nFallbackSeed,
                    static_cast<u32>(skin.Pattern), "m_nFallbackSeed", weapon);
        {
            u32 st = (skin.StatTrak >= 0) ? static_cast<u32>(skin.StatTrak) : 0xFFFFFFFFu;
            WriteVerify(mem_, weapon + Offsets::m_nFallbackStatTrak,
                        st, "m_nFallbackStatTrak", weapon);
        }
        WriteVerify(mem_, weapon + Offsets::m_OriginalOwnerXuidLow,
                    static_cast<u32>(0), "m_OriginalOwnerXuidLow", weapon);
        WriteVerify(mem_, weapon + Offsets::m_OriginalOwnerXuidHigh,
                    static_cast<u32>(0), "m_OriginalOwnerXuidHigh", weapon);

        // Set item quality to Unique (5) or Strange (10) so the game knows
        // this is a custom-skin item, not a vanilla/base weapon.
        WriteVerify(mem_, item + Offsets::m_iEntityQuality,
                    (skin.StatTrak >= 0) ? 10 : 5, "m_iEntityQuality", weapon);

        // Force the item view to use our fallback values
        // m_iItemIDHigh = -1 signals "use fallback values" to CS2
        // m_iItemIDLow = 0 (neutral — not -1, which some code paths treat as invalid)
        WriteVerify(mem_, item + Offsets::m_iItemID,
                    0xFFFFFFFFFFFFFFFFull, "m_iItemID", weapon);
        WriteVerify(mem_, item + Offsets::m_iItemIDHigh,
                    0xFFFFFFFFu, "m_iItemIDHigh", weapon);
        WriteVerify(mem_, item + Offsets::m_iItemIDLow,
                    0u, "m_iItemIDLow", weapon);
        if (account_id_)
            WriteVerify(mem_, item + Offsets::m_iAccountID,
                        account_id_, "m_iAccountID", weapon);

        // Create econ attributes (paint, pattern, wear) EVERY TICK.
        // CS2 clears the attribute list on round restarts, model precache,
        // and inventory updates. Without attributes, some rendering paths
        // fall back to real inventory data instead of our fallback values.
        if (!attr_mgr_.Create(item, skin))
            LOG_WARN("SkinChanger::Tick — attr_mgr_.Create failed for item 0x{:X} paint={}", item, skin.Paint);

        // Set mesh mask on the weapon entity and its HUD view model
        {
            auto n = mem_.ReadSafe<u64>(weapon + Offsets::m_pGameSceneNode);
            if (n && *n) DoMeshMask(weapon, *n, skin.bUsesOldModel);
        }

        applied = true;
    }

    // Gloves — per-tick fallback writes (same as weapons) so gloves survive
    // round restarts without requiring the user to re-apply.
    {
        i32 glove_paint = mgr_.GetGlovePaint(is_ct);
        if (glove_paint) {
            // Validate cached glove entity every tick (entity may be stale after round restart)
            if (cached_glove_entity_) {
                auto verify_def = mem_.ReadSafe<u16>(cached_glove_entity_ + Offsets::m_AttributeManager + Offsets::m_Item + Offsets::m_iItemDefinitionIndex);
                if (!verify_def || *verify_def < 5027 || *verify_def > 5036)
                    cached_glove_entity_ = 0;
            }
            if (!cached_glove_entity_ || mgr_.NeedsUpdate())
                cached_glove_entity_ = FindGloveEntity(mem_, local_player_val);

            if (cached_glove_entity_) {
                // CRITICAL: Write ALL glove fields to the GLOVE ENTITY's item view,
                // not the player pawn's embedded m_EconGloves. CS2 reads glove data
                // from the entity's item view for rendering.
                u64 glove_item = cached_glove_entity_ + Offsets::m_AttributeManager + Offsets::m_Item;

                // Write m_bInitialized = false FIRST (same order as weapons)
                WriteVerify(mem_, glove_item + Offsets::m_bInitialized,
                            false, "glove m_bInitialized", cached_glove_entity_);

                // Fallback values on the glove entity
                WriteVerify(mem_, cached_glove_entity_ + Offsets::m_nFallbackPaintKit,
                            static_cast<u32>(glove_paint), "glove m_nFallbackPaintKit", cached_glove_entity_);
                WriteVerify(mem_, cached_glove_entity_ + Offsets::m_flFallbackWear,
                            0.01f, "glove m_flFallbackWear", cached_glove_entity_);
                WriteVerify(mem_, cached_glove_entity_ + Offsets::m_nFallbackStatTrak,
                            0xFFFFFFFFu, "glove m_nFallbackStatTrak", cached_glove_entity_);
                WriteVerify(mem_, cached_glove_entity_ + Offsets::m_OriginalOwnerXuidLow,
                            0u, "glove m_OriginalOwnerXuidLow", cached_glove_entity_);
                WriteVerify(mem_, cached_glove_entity_ + Offsets::m_OriginalOwnerXuidHigh,
                            0u, "glove m_OriginalOwnerXuidHigh", cached_glove_entity_);

                // Item view fields on the glove entity
                WriteVerify(mem_, glove_item + Offsets::m_iItemID,
                            0xFFFFFFFFFFFFFFFFull, "glove entity m_iItemID", cached_glove_entity_);
                WriteVerify(mem_, glove_item + Offsets::m_iItemIDHigh,
                            0xFFFFFFFFu, "glove entity m_iItemIDHigh", cached_glove_entity_);
                WriteVerify(mem_, glove_item + Offsets::m_iItemIDLow,
                            0u, "glove entity m_iItemIDLow", cached_glove_entity_);
                if (account_id_) {
                    WriteVerify(mem_, glove_item + Offsets::m_iAccountID,
                                account_id_, "glove entity m_iAccountID", cached_glove_entity_);
                    WriteVerify(mem_, glove_item + Offsets::m_iEntityQuality,
                                static_cast<i32>(5), "glove entity m_iEntityQuality", cached_glove_entity_);
                }

                // Create attribute list on the GLOVE ENTITY (not player pawn)
                SkinInfo_t glove_skin{glove_paint, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum::None};
                attr_mgr_.Create(glove_item, glove_skin);
            }

            applied = true;
        }
    }

    // Attribute creation + mesh mask — only when user explicitly changes a setting
    if (mgr_.NeedsUpdate()) {
        if (ApplyGloves()) applied = true;
    }

    if (applied) {
        mgr_.ClearUpdate();
        last_tick_ = now;
    } else if (mgr_.NeedsUpdate()) {
        // No weapons matched but user changed a setting — clear the flag
        // to prevent running at full speed every tick forever.
        mgr_.ClearUpdate();
    }

    return applied;
}

bool SkinChanger::ApplyGloves() noexcept {
    auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_player || !*local_player) {
        LOG_WARN("ApplyGloves — no local player");
        return false;
    }

    // Sanity-check the m_EconGloves offset before using it
    if (Offsets::m_EconGloves <= 0 || Offsets::m_EconGloves > 0x10000) {
        LOG_WARN("ApplyGloves — m_EconGloves offset (0x{:X}) looks invalid (CS2 update?)",
                 Offsets::m_EconGloves);
        return false;
    }

    Team team = ReadTeam(mem_, *local_player);
    bool is_ct = (team == Team_CT);
    i32 glove_paint = mgr_.GetGlovePaint(is_ct);
    if (!glove_paint) {
        LOG_WARN("ApplyGloves — no glove paint selected");
        return false;
    }

    // Find and cache the glove entity (C_EconGloves) for per-tick fallback writes
    cached_glove_entity_ = FindGloveEntity(mem_, *local_player);

    // Per-tick fallback writes + attribute creation are now handled in Tick().
    // Here we only do one-shot operations: mesh mask + reapply flag.

    // Hide hud arms model mask
    u64 hud_arms = GetHudArms(mem_);
    if (hud_arms) {
        auto node = mem_.ReadSafe<u64>(hud_arms + Offsets::m_pGameSceneNode);
        if (node && *node) {
            u64 model = *node + Offsets::m_modelState;
            WriteVerify(mem_, model + Offsets::m_MeshGroupMask,
                        static_cast<u64>(1), "arms m_MeshGroupMask", hud_arms);
        }
    }

    WriteVerify(mem_, *local_player + Offsets::m_bNeedToReApplyGloves,
                true, "m_bNeedToReApplyGloves", *local_player);

    return true;
}


u32 SkinChanger::ResolveAccountId() noexcept {
    auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_player || !*local_player) return 0;
    auto weapons = GetWeapons(mem_, *local_player);
    for (u64 weapon : weapons) {
        // m_AttributeManager and m_Item are EMBEDDED structs (not pointers).
        u64 item = weapon + Offsets::m_AttributeManager + Offsets::m_Item;
        auto aid = mem_.ReadSafe<u32>(item + Offsets::m_iAccountID);
        if (aid && *aid) return *aid;
    }
    return 0;
}

void SkinChanger::DoMeshMask(u64 weapon, u64 node, bool old_model) noexcept {
    u64 model = node + Offsets::m_modelState;
    u64 mask = old_model ? 2 : 1;
    auto dirty = mem_.ReadSafe<u64>(model + Offsets::m_pDirtyModelData);
    if (dirty && *dirty)
        WriteVerify(mem_, *dirty + Offsets::m_DirtyMeshGroupMask,
                    mask, "m_DirtyMeshGroupMask", weapon);
    WriteVerify(mem_, model + Offsets::m_MeshGroupMask,
                mask, "weapon m_MeshGroupMask", weapon);
    u64 hud = GetHudWeapon(mem_, weapon);
    if (!hud) return;
    auto hud_node = mem_.ReadSafe<u64>(hud + Offsets::m_pGameSceneNode);
    if (!hud_node || !*hud_node) return;
    WriteVerify(mem_, *hud_node + Offsets::m_modelState + Offsets::m_MeshGroupMask,
                mask, "hud m_MeshGroupMask", hud);
}


