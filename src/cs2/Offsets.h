#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <charconv>
#include <cstdlib>

/* ── CS2 Offsets ────────────────────────────────────────────────────────────
 *
 * Runtime-resolvable offsets for CS2.  All values are mutable `inline`
 * variables, loaded from an external data file at startup so they survive
 * game updates without recompiling.  Ship cs2-dumper defaults embedded.
 *
 * When CS2 patches, the user replaces the offset file (or the embedded
 * defaults apply).  Keys match cs2-dumper output names.
 *
 * Regenerate defaults: https://github.com/a2x/cs2-dumper
 * ──────────────────────────────────────────────────────────────────────────── */

namespace Offsets {

// ── External addresses (client.dll) ──────────────────────────────────────────
inline std::ptrdiff_t dwEntityList             = 0x24A7B48;
inline std::ptrdiff_t dwLocalPlayerController  = 0x22ECA18;
inline std::ptrdiff_t dwLocalPlayerPawn        = 0x2062850;
inline std::ptrdiff_t dwGlobalVars             = 0x20572A8;
inline std::ptrdiff_t dwViewMatrix             = 0x2308850;
inline std::ptrdiff_t dwGameRules              = 0x23035F0;

// ── External addresses (engine2.dll) ─────────────────────────────────────────
inline std::ptrdiff_t dwNetworkGameClient      = 0x900FB0;

// ── Schema offsets: C_BaseEntity ─────────────────────────────────────────────
inline std::ptrdiff_t m_pGameSceneNode         = 0x308;
inline std::ptrdiff_t m_nSubclassID            = 0x2C8;
inline std::ptrdiff_t m_hOwnerEntity           = 0x2C0;

// ── CEntityInstance ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pEntity                = 0x10;

// ── CEntityIdentity ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_flags                  = 0x28;

// ── C_CSPlayerPawn ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pClippingWeapon        = 0x15D0;
inline std::ptrdiff_t m_hHudModelArms          = 0x1600;
inline std::ptrdiff_t m_bNeedToReApplyGloves   = 0x15F7;
inline std::ptrdiff_t m_EconGloves             = 0x15D8;

// ── C_BasePlayerPawn ──────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pWeaponServices        = 0x10C0;

// ── CCSPlayerController ───────────────────────────────────────────────────────
inline std::ptrdiff_t m_pInventoryServices      = 0x6B0;

// ── CCSPlayerController_InventoryServices ─────────────────────────────────────
inline std::ptrdiff_t m_unMusicID              = 0x39;

// ── CPlayer_WeaponServices ────────────────────────────────────────────────────
inline std::ptrdiff_t m_hMyWeapons             = 0x30;
inline std::ptrdiff_t m_hActiveWeapon          = 0x38;

// ── C_EconEntity ──────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_AttributeManager       = 0x940;
inline std::ptrdiff_t m_nFallbackPaintKit      = 0x11D0;
inline std::ptrdiff_t m_nFallbackStatTrak      = 0x11D4;
inline std::ptrdiff_t m_flFallbackWear         = 0x11D8;
inline std::ptrdiff_t m_nFallbackSeed          = 0x11DC;
inline std::ptrdiff_t m_OriginalOwnerXuidLow   = 0x11E0;

// ── C_AttributeContainer ─────────────────────────────────────────────────────
inline std::ptrdiff_t m_Item                   = 0x30;

// ── C_EconItemView ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_AttributeList           = 0x70;
inline std::ptrdiff_t m_NetworkedDynamicAttributes = 0x80;
inline std::ptrdiff_t m_iItemDefinitionIndex   = 0xF0;
inline std::ptrdiff_t m_iAccountID             = 0xF4;
inline std::ptrdiff_t m_iItemIDHigh            = 0xF8;
inline std::ptrdiff_t m_szCustomNameOverride   = 0x100;
inline std::ptrdiff_t m_bInitialized           = 0x9C;
inline std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x9D;
inline std::ptrdiff_t m_iEntityQuality         = 0xA0;

// ── CAttributeList ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_Attributes             = 0x18;

// ── CSkeletonInstance / CModelState ───────────────────────────────────────────
inline std::ptrdiff_t m_modelState             = 0x160;
inline std::ptrdiff_t m_MeshGroupMask          = 0x104;

// ── CGameSceneNode ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pChild                 = 0x38;
inline std::ptrdiff_t m_pNextSibling           = 0x40;
inline std::ptrdiff_t m_pOwner                 = 0x30;

// ── CCSWeaponBaseVData ────────────────────────────────────────────────────────
inline std::ptrdiff_t m_bMeleeWeapon           = 0x908;
inline std::ptrdiff_t m_WeaponType             = 0x91C;

// ── Extra (not in cs2-dumper output) ──────────────────────────────────────────
inline std::ptrdiff_t m_pDirtyModelData        = 0xD8;
inline std::ptrdiff_t m_DirtyMeshGroupMask     = 0x10;

// ── Signature patterns (change less often than offsets) ───────────────────────
namespace Sigs {
    inline constexpr const char* RegenerateWeaponSkins =
        "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10";
}

// ── Load/save helpers ─────────────────────────────────────────────────────────

/// Path to the offset data file (stored alongside the skin config).
inline std::filesystem::path DataFilePath() {
    const char* appdata = getenv("APPDATA");
    if (!appdata) appdata = getenv("HOME");
    if (!appdata) return "cs2_offsets.txt";
    return std::filesystem::path(appdata) / "cs2-skin-changer" / "offsets.txt";
}

/// Load all offsets from a data file.  Keys not present keep embedded defaults.
inline bool LoadFromFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return false;

    // Build a name→pointer map for all inline offset variables
    std::unordered_map<std::string_view, std::ptrdiff_t*> table = {
        {"dwEntityList",             &dwEntityList},
        {"dwLocalPlayerController",  &dwLocalPlayerController},
        {"dwLocalPlayerPawn",        &dwLocalPlayerPawn},
        {"dwGlobalVars",             &dwGlobalVars},
        {"dwViewMatrix",             &dwViewMatrix},
        {"dwGameRules",              &dwGameRules},
        {"dwNetworkGameClient",      &dwNetworkGameClient},
        {"m_pGameSceneNode",         &m_pGameSceneNode},
        {"m_nSubclassID",            &m_nSubclassID},
        {"m_hOwnerEntity",           &m_hOwnerEntity},
        {"m_pEntity",                &m_pEntity},
        {"m_flags",                  &m_flags},
        {"m_pClippingWeapon",        &m_pClippingWeapon},
        {"m_hHudModelArms",          &m_hHudModelArms},
        {"m_bNeedToReApplyGloves",   &m_bNeedToReApplyGloves},
        {"m_EconGloves",             &m_EconGloves},
        {"m_pWeaponServices",        &m_pWeaponServices},
        {"m_pInventoryServices",     &m_pInventoryServices},
        {"m_unMusicID",              &m_unMusicID},
        {"m_hMyWeapons",             &m_hMyWeapons},
        {"m_hActiveWeapon",          &m_hActiveWeapon},
        {"m_AttributeManager",       &m_AttributeManager},
        {"m_nFallbackPaintKit",      &m_nFallbackPaintKit},
        {"m_nFallbackStatTrak",      &m_nFallbackStatTrak},
        {"m_flFallbackWear",         &m_flFallbackWear},
        {"m_nFallbackSeed",          &m_nFallbackSeed},
        {"m_OriginalOwnerXuidLow",   &m_OriginalOwnerXuidLow},
        {"m_Item",                   &m_Item},
        {"m_AttributeList",          &m_AttributeList},
        {"m_NetworkedDynamicAttributes", &m_NetworkedDynamicAttributes},
        {"m_iItemDefinitionIndex",   &m_iItemDefinitionIndex},
        {"m_iAccountID",             &m_iAccountID},
        {"m_iItemIDHigh",            &m_iItemIDHigh},
        {"m_szCustomNameOverride",   &m_szCustomNameOverride},
        {"m_bInitialized",           &m_bInitialized},
        {"m_bRestoreCustomMaterialAfterPrecache", &m_bRestoreCustomMaterialAfterPrecache},
        {"m_iEntityQuality",         &m_iEntityQuality},
        {"m_Attributes",             &m_Attributes},
        {"m_modelState",             &m_modelState},
        {"m_MeshGroupMask",          &m_MeshGroupMask},
        {"m_pChild",                 &m_pChild},
        {"m_pNextSibling",           &m_pNextSibling},
        {"m_pOwner",                 &m_pOwner},
        {"m_bMeleeWeapon",           &m_bMeleeWeapon},
        {"m_WeaponType",             &m_WeaponType},
        {"m_pDirtyModelData",        &m_pDirtyModelData},
        {"m_DirtyMeshGroupMask",     &m_DirtyMeshGroupMask},
    };

    std::string line;
    bool any = false;
    while (std::getline(f, line)) {
        auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string key;
        char eq{};
        ss >> key >> eq;
        if (eq != '=' || key.empty()) continue;

        auto it = table.find(key);
        if (it == table.end()) continue;

        std::string val_str;
        ss >> val_str;
        if (val_str.empty()) continue;

        std::ptrdiff_t val = 0;
        if (val_str.starts_with("0x") || val_str.starts_with("0X"))
            std::from_chars(val_str.data() + 2, val_str.data() + val_str.size(), val, 16);
        else
            std::from_chars(val_str.data(), val_str.data() + val_str.size(), val);

        if (val != 0 || val_str == "0") {
            *it->second = val;
            any = true;
        }
    }
    return any;
}

/// Save current offsets to a data file so the user can inspect/edit them.
inline bool SaveToFile(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if (!f) return false;

    f << "# CS2 Offsets — saved " << __DATE__ << "\n";
    f << "# Format: key = hex_value\n";
    f << "# Replace this file when CS2 updates (get new values from cs2-dumper)\n\n";

    auto write = [&](const char* name, std::ptrdiff_t val) {
        f << name << " = 0x" << std::hex << val << std::dec << "\n";
    };

    write("dwEntityList",             dwEntityList);
    write("dwLocalPlayerController",  dwLocalPlayerController);
    write("dwLocalPlayerPawn",        dwLocalPlayerPawn);
    write("dwGlobalVars",             dwGlobalVars);
    write("dwViewMatrix",             dwViewMatrix);
    write("dwGameRules",              dwGameRules);
    write("dwNetworkGameClient",      dwNetworkGameClient);
    f << "\n";
    write("m_pGameSceneNode",         m_pGameSceneNode);
    write("m_nSubclassID",            m_nSubclassID);
    write("m_hOwnerEntity",           m_hOwnerEntity);
    write("m_pEntity",                m_pEntity);
    write("m_flags",                  m_flags);
    f << "\n";
    write("m_pClippingWeapon",        m_pClippingWeapon);
    write("m_hHudModelArms",          m_hHudModelArms);
    write("m_bNeedToReApplyGloves",   m_bNeedToReApplyGloves);
    write("m_EconGloves",             m_EconGloves);
    write("m_pWeaponServices",        m_pWeaponServices);
    write("m_pInventoryServices",     m_pInventoryServices);
    write("m_unMusicID",              m_unMusicID);
    write("m_hMyWeapons",             m_hMyWeapons);
    write("m_hActiveWeapon",          m_hActiveWeapon);
    f << "\n";
    write("m_AttributeManager",       m_AttributeManager);
    write("m_nFallbackPaintKit",      m_nFallbackPaintKit);
    write("m_nFallbackStatTrak",      m_nFallbackStatTrak);
    write("m_flFallbackWear",         m_flFallbackWear);
    write("m_nFallbackSeed",          m_nFallbackSeed);
    write("m_OriginalOwnerXuidLow",   m_OriginalOwnerXuidLow);
    write("m_Item",                   m_Item);
    write("m_AttributeList",          m_AttributeList);
    write("m_NetworkedDynamicAttributes", m_NetworkedDynamicAttributes);
    write("m_iItemDefinitionIndex",   m_iItemDefinitionIndex);
    write("m_iAccountID",             m_iAccountID);
    write("m_iItemIDHigh",            m_iItemIDHigh);
    write("m_szCustomNameOverride",   m_szCustomNameOverride);
    write("m_bInitialized",           m_bInitialized);
    write("m_bRestoreCustomMaterialAfterPrecache", m_bRestoreCustomMaterialAfterPrecache);
    write("m_iEntityQuality",         m_iEntityQuality);
    write("m_Attributes",             m_Attributes);
    f << "\n";
    write("m_modelState",             m_modelState);
    write("m_MeshGroupMask",          m_MeshGroupMask);
    write("m_pChild",                 m_pChild);
    write("m_pNextSibling",           m_pNextSibling);
    write("m_pOwner",                 m_pOwner);
    write("m_bMeleeWeapon",           m_bMeleeWeapon);
    write("m_WeaponType",             m_WeaponType);
    write("m_pDirtyModelData",        m_pDirtyModelData);
    write("m_DirtyMeshGroupMask",     m_DirtyMeshGroupMask);

    return true;
}

/// Reset all offsets to cs2-dumper embedded defaults (already set at init).
inline void ResetToDefaults() {
    // The inline variable initialisers are already the defaults.
    // To force a reload from defaults, the simplest path is to delete the
    // data file — LoadFromFile will fail and the built-in values stay.
}

} // namespace Offsets
