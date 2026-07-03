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

bool SkinChanger::Tick() noexcept {
    auto now = std::chrono::steady_clock::now();
    if (!mgr_.NeedsUpdate() &&
        now - last_tick_ < std::chrono::milliseconds(50))
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

    // Music kit
    auto inv = mem_.ReadSafe<u64>(local_ctrl_val + Offsets::m_pInventoryServices);
    if (inv && *inv && music_kit_id_ != last_music_kit_id_) {
        mem_.Write<u16>(*inv + Offsets::m_unMusicID,
                        static_cast<u16>(music_kit_id_));
        last_music_kit_id_ = music_kit_id_;
    }

    // Weapons
    auto weapons = GetWeapons(mem_, local_player_val);
    // Fallback: if the weapon vector is empty, try the active weapon directly
    if (weapons.empty()) {
        u64 active = GetActiveWeapon(mem_, local_player_val);
        if (active) weapons.push_back(active);
    }
    bool applied = false;

    for (u64 weapon : weapons) {
        u64 attr_mgr = mem_.Read<u64>(weapon + Offsets::m_AttributeManager);
        if (!attr_mgr || attr_mgr == ~0ull) {
            if (!logged_weapon_warns_.count(weapon)) {
                LOG_WARN("SkinChanger::Tick — weapon 0x{:X}: m_AttributeManager (off 0x{:X}) = 0",
                         weapon, Offsets::m_AttributeManager);
                if (debug_) debug_->ReportError("m_AttributeManager=0 for weapon — offset wrong?");
            }
            logged_weapon_warns_.insert(weapon);
            continue;
        }
        u64 item = mem_.Read<u64>(attr_mgr + Offsets::m_Item);
        if (!item || item == ~0ull) {
            if (!logged_weapon_warns_.count(weapon)) {
                LOG_WARN("SkinChanger::Tick — weapon 0x{:X}: m_Item (off 0x{:X} in C_AttributeContainer) = 0",
                         weapon, Offsets::m_Item);
                if (debug_) debug_->ReportError("m_Item=0 for weapon — m_Item offset wrong?");
            }
            logged_weapon_warns_.insert(weapon);
            continue;
        }

        auto cur_def_raw = mem_.ReadSafe<u16>(item + Offsets::m_iItemDefinitionIndex);
        if (!cur_def_raw) continue;
        i32 cur_def = *cur_def_raw;
        WeaponsEnum def = static_cast<WeaponsEnum>(cur_def);

        bool is_knife = mgr_.IsKnifeDef(cur_def);
        i32  knife_def  = static_cast<i32>(mgr_.GetKnifeDef(is_ct));
        i32  knife_pnt  = mgr_.GetKnifePaint(is_ct);

        SkinInfo_t skin = is_knife && knife_pnt
            ? SkinInfo_t{knife_pnt, 0.01f, 0, -1, "", false, "Knife", WeaponsEnum::None}
            : mgr_.GetSkin(def);

        if (!skin.Paint) continue;

        // Fallback values (written every frame for reliability)
        mem_.Write<u32>(weapon + Offsets::m_nFallbackPaintKit, static_cast<u32>(skin.Paint));
        mem_.Write<f32>(weapon + Offsets::m_flFallbackWear, skin.Wear);
        mem_.Write<u32>(weapon + Offsets::m_nFallbackSeed, static_cast<u32>(skin.Pattern));
        {
            u32 st = (skin.StatTrak >= 0) ? static_cast<u32>(skin.StatTrak) : 0xFFFFFFFFu;
            mem_.Write<u32>(weapon + Offsets::m_nFallbackStatTrak, st);
        }
        mem_.Write<u32>(weapon + Offsets::m_OriginalOwnerXuidLow, 0);
        mem_.Write<u32>(weapon + Offsets::m_OriginalOwnerXuidHigh, 0);
        // Nametag (m_szCustomNameOverride is CUtlString — needs heap alloc, skip for now)
        // TODO: implement nametag via string allocation in target process

        // Set item quality to Unique (5) or Strange (10) so the game knows
        // this is a custom-skin item, not a vanilla/base weapon.
        mem_.Write<i32>(item + Offsets::m_iEntityQuality, (skin.StatTrak >= 0) ? 10 : 5);

        // Force the item view to use our fallback values
        mem_.Write<u64>(item + Offsets::m_iItemID, 0xFFFFFFFFFFFFFFFFull);
        mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
        mem_.Write<u32>(item + Offsets::m_iItemIDLow, 0xFFFFFFFF);
        if (account_id_)
            mem_.Write<u32>(item + Offsets::m_iAccountID, account_id_);
        if (is_knife && knife_def > 0 && knife_def != cur_def)
            mem_.Write<u16>(item + Offsets::m_iItemDefinitionIndex, static_cast<u16>(knife_def));

        // Create econ attributes (paint, pattern, wear) in the item view's attribute list.
        // Only needed when the user changes a skin — fallback values written above
        // are read by CS2's renderer every frame.
        if (mgr_.NeedsUpdate()) {
            if (!attr_mgr_.Create(item, skin))
                LOG_WARN("SkinChanger::Tick — attr_mgr_.Create failed for item 0x{:X} paint={}", item, skin.Paint);
        }

        // Set mesh mask on the weapon entity and its HUD view model
        {
            auto n = mem_.ReadSafe<u64>(weapon + Offsets::m_pGameSceneNode);
            if (n && *n) DoMeshMask(weapon, *n, skin.bUsesOldModel);
        }

        // Mark as uninitialized so the game re-reads our custom data.
        // Also clear the restore-custom-material flag so CS2 re-applies
        // the skin material on the next frame.
        mem_.Write<bool>(item + Offsets::m_bInitialized, false);
        mem_.Write<bool>(item + Offsets::m_bRestoreCustomMaterialAfterPrecache, false);

        applied = true;
    }

    // Gloves — only apply when user explicitly changes a glove setting
    if (mgr_.NeedsUpdate()) {
        if (ApplyGloves()) applied = true;
    }

    if (applied) {
        mgr_.ClearUpdate();
        last_tick_ = now;
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

    // m_EconGloves is C_EconItemView embedded-by-value in the player pawn.
    // Direct field access on the embedded item view:
    u64 glove_iv = *local_player + Offsets::m_EconGloves;

    // Find the glove entity (C_EconGloves) via entity list scan.
    // Its m_Item pointer should match the embedded item view address.
    u64 glove_entity = FindGloveEntity(mem_, *local_player);

    if (glove_entity) {
        // Write fallback values to the glove entity
        mem_.Write<u32>(glove_entity + Offsets::m_nFallbackPaintKit, static_cast<u32>(glove_paint));
        mem_.Write<f32>(glove_entity + Offsets::m_flFallbackWear, 0.01f);
        mem_.Write<u32>(glove_entity + Offsets::m_nFallbackStatTrak, 0xFFFFFFFF);
        mem_.Write<u32>(glove_entity + Offsets::m_OriginalOwnerXuidLow, 0);
        mem_.Write<u32>(glove_entity + Offsets::m_OriginalOwnerXuidHigh, 0);
    } else {
        LOG_WARN("ApplyGloves — glove entity not found; writing fallback to item view only");
    }

    // Write item view fields directly to the embedded C_EconItemView
    mem_.Write<u64>(glove_iv + Offsets::m_iItemID, 0xFFFFFFFFFFFFFFFFull);
    mem_.Write<u32>(glove_iv + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
    mem_.Write<u32>(glove_iv + Offsets::m_iItemIDLow, 0xFFFFFFFF);
    mem_.Write<u32>(glove_iv + Offsets::m_iAccountID, account_id_);
    mem_.Write<i32>(glove_iv + Offsets::m_iEntityQuality, 5);
    mem_.Write<bool>(glove_iv + Offsets::m_bInitialized, false);
    mem_.Write<bool>(glove_iv + Offsets::m_bRestoreCustomMaterialAfterPrecache, false);

    // Attribute creation: use the item view address directly
    SkinInfo_t glove_skin{glove_paint, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum::None};
    if (!attr_mgr_.Create(glove_iv, glove_skin))
        LOG_WARN("ApplyGloves — attr_mgr_.Create failed for glove_iv 0x{:X} paint={}", glove_iv, glove_paint);

    // Hide hud arms model mask
    u64 hud_arms = GetHudArms(mem_);
    if (hud_arms) {
        auto node = mem_.ReadSafe<u64>(hud_arms + Offsets::m_pGameSceneNode);
        if (node && *node) {
            u64 model = *node + Offsets::m_modelState;
            mem_.Write<u64>(model + Offsets::m_MeshGroupMask, 1);
        }
    } else {
        LOG_WARN("ApplyGloves — GetHudArms returned 0");
    }

    mem_.Write<bool>(*local_player + Offsets::m_bNeedToReApplyGloves, true);

    return true;
}


u32 SkinChanger::ResolveAccountId() noexcept {
    auto local_player = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerPawn);
    if (!local_player || !*local_player) return 0;
    auto weapons = GetWeapons(mem_, *local_player);
    for (u64 weapon : weapons) {
        u64 attr_mgr = mem_.Read<u64>(weapon + Offsets::m_AttributeManager);
        if (!attr_mgr || attr_mgr == ~0ull) continue;
        u64 item = mem_.Read<u64>(attr_mgr + Offsets::m_Item);
        if (!item || item == ~0ull) continue;
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
        mem_.Write<u64>(*dirty + Offsets::m_DirtyMeshGroupMask, mask);
    mem_.Write<u64>(model + Offsets::m_MeshGroupMask, mask);
    u64 hud = GetHudWeapon(mem_, weapon);
    if (!hud) {
        LOG_WARN("DoMeshMask — GetHudWeapon returned 0 for weapon 0x{:X}", weapon);
        return;
    }
    auto hud_node = mem_.ReadSafe<u64>(hud + Offsets::m_pGameSceneNode);
    if (!hud_node || !*hud_node) {
        LOG_WARN("DoMeshMask — no scene node on hud weapon 0x{:X}", hud);
        return;
    }
    mem_.Write<u64>(*hud_node + Offsets::m_modelState + Offsets::m_MeshGroupMask, mask);
}


