#include "SkinChanger.h"
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
        return false;
    }
    auto local_ctrl = mem_.ReadSafe<u64>(mem_.Client() + Offsets::dwLocalPlayerController);
    if (!local_ctrl || !*local_ctrl) {
        LOG_WARN("SkinChanger::Tick — no local player controller");
        return false;
    }

    u64 local_player_val = *local_player;
    u64 local_ctrl_val = *local_ctrl;

    Team team = ReadTeam(mem_, local_player_val);
    bool is_ct = (team == Team_CT);

    if (!account_id_) {
        account_id_ = ResolveAccountId();
        if (!account_id_)
            LOG_ERROR("SkinChanger::Tick — failed to resolve account ID");
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
    bool applied = false;

    for (u64 weapon : weapons) {
        u64 item = mem_.Read<u64>(weapon + Offsets::m_AttributeManager + Offsets::m_Item);
        if (!item || item == ~0ull) continue;

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

        // Fallback + item-view force
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

        mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
        mem_.Write<u32>(item + Offsets::m_iAccountID, account_id_);
        mem_.Write<bool>(item + Offsets::m_bInitialized, false);
        mem_.Write<bool>(item + Offsets::m_bRestoreCustomMaterialAfterPrecache, true);

        if (is_knife && knife_def > 0 && knife_def != cur_def)
            mem_.Write<u16>(item + Offsets::m_iItemDefinitionIndex, static_cast<u16>(knife_def));

        // Mesh mask + attrs (randomized order)
        auto do_mesh = [&]() {
            auto n = mem_.ReadSafe<u64>(weapon + Offsets::m_pGameSceneNode);
            if (n && *n) DoMeshMask(weapon, *n, skin.bUsesOldModel);
        };
        auto do_attrs = [&]() {
            if (!attr_mgr_.Create(item, skin))
                LOG_WARN("SkinChanger::Tick — attr_mgr_.Create failed for item 0x{:X} paint={}", item, skin.Paint);
        };

        if (std::uniform_int_distribution<int>(0, 1)(rng_)) {
            do_mesh();  do_attrs();
        } else {
            do_attrs(); do_mesh();
        }

        // Force the game to re-read and apply our attributes immediately
        CallRegenerateWeaponSkins(item);

        applied = true;
    }

    // Gloves
    if (ApplyGloves()) applied = true;

    if (applied || mgr_.NeedsUpdate()) {
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

    Team team = ReadTeam(mem_, *local_player);
    bool is_ct = (team == Team_CT);
    i32 glove_paint = mgr_.GetGlovePaint(is_ct);
    if (!glove_paint) {
        LOG_WARN("ApplyGloves — no glove paint selected");
        return false;
    }

    u64 glove_data = *local_player + Offsets::m_EconGloves;
    auto handle = mem_.ReadSafe<u32>(glove_data);
    if (!handle || !*handle) {
        LOG_WARN("ApplyGloves — no glove handle found at 0x{:X}", glove_data);
        return false;
    }

    u64 glove_entity = GetEntityByHandle(mem_, *handle);
    if (!glove_entity) {
        LOG_WARN("ApplyGloves — GetEntityByHandle failed for handle 0x{:X}", *handle);
        return false;
    }

    u64 item = mem_.Read<u64>(glove_entity + Offsets::m_AttributeManager + Offsets::m_Item);
    if (!item || item == ~0ull) {
        LOG_WARN("ApplyGloves — no econ item at glove entity 0x{:X}", glove_entity);
        return false;
    }

    mem_.Write<u32>(glove_entity + Offsets::m_nFallbackPaintKit, static_cast<u32>(glove_paint));
    mem_.Write<f32>(glove_entity + Offsets::m_flFallbackWear, 0.01f);
    mem_.Write<u32>(glove_entity + Offsets::m_nFallbackStatTrak, 0xFFFFFFFF);
    mem_.Write<u32>(glove_entity + Offsets::m_OriginalOwnerXuidLow, 0);
    mem_.Write<u32>(glove_entity + Offsets::m_OriginalOwnerXuidHigh, 0);

    mem_.Write<u32>(item + Offsets::m_iItemIDHigh, 0xFFFFFFFF);
    mem_.Write<u32>(item + Offsets::m_iAccountID, account_id_);
    mem_.Write<bool>(item + Offsets::m_bInitialized, false);
    mem_.Write<bool>(item + Offsets::m_bRestoreCustomMaterialAfterPrecache, true);

    SkinInfo_t glove_skin{glove_paint, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum::None};
    if (!attr_mgr_.Create(item, glove_skin))
        LOG_WARN("ApplyGloves — attr_mgr_.Create failed for item 0x{:X} paint={}", item, glove_paint);

    // Force visual refresh for glove item too
    CallRegenerateWeaponSkins(item);

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
        u64 item = mem_.Read<u64>(weapon + Offsets::m_AttributeManager + Offsets::m_Item);
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

void SkinChanger::CallRegenerateWeaponSkins(u64 item) noexcept {
    if (mem_.ProcessId() != ::GetCurrentProcessId()) {
        LOG_WARN("CallRegenerateWeaponSkins — external process, skipping");
        return;
    }

    if (!regenerate_addr_) {
        regenerate_addr_ = mem_.SigScan(L"client.dll", Offsets::Sigs::RegenerateWeaponSkins);
        if (!regenerate_addr_) {
            LOG_ERROR("SkinChanger — failed to resolve RegenerateWeaponSkins signature");
            return;
        }
        LOG_INFO("SkinChanger — resolved RegenerateWeaponSkins at 0x{:X}", regenerate_addr_);
    }

    LOG_INFO("SkinChanger — calling RegenerateWeaponSkins(0x{:X})", item);

    using Fn = void (*)(void*);

#if defined(_MSC_VER)
    __try {
        reinterpret_cast<Fn>(regenerate_addr_)(reinterpret_cast<void*>(item));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LOG_ERROR("SkinChanger — access violation in RegenerateWeaponSkins, disabling");
        regenerate_addr_ = 0;
    }
#else
    // MinGW: no SEH, call directly. Signature validation should prevent crashes.
    reinterpret_cast<Fn>(regenerate_addr_)(reinterpret_cast<void*>(item));
#endif
}
