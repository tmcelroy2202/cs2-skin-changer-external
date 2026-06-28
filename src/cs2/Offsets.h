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
#include <optional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

/* ── CS2 Offsets ────────────────────────────────────────────────────────────
 *
 * Runtime-resolvable offsets for CS2.  Embedded defaults match cs2-dumper
 * (June 2026).  On startup we try to refresh from GitHub, then merge any
 * user overrides from offsets.txt.
 *
 * Regenerate defaults: https://github.com/a2x/cs2-dumper
 * ──────────────────────────────────────────────────────────────────────────── */

namespace Offsets {

// ── External addresses (client.dll) ──────────────────────────────────────────
inline std::ptrdiff_t dwEntityList             = 0x24E2A20;
inline std::ptrdiff_t dwLocalPlayerController  = 0x2323F20;
inline std::ptrdiff_t dwLocalPlayerPawn        = 0x234C598;
inline std::ptrdiff_t dwGlobalVars             = 0x2067B50;
inline std::ptrdiff_t dwViewMatrix             = 0x235E470;
inline std::ptrdiff_t dwGameRules              = 0x2358A38;

// ── External addresses (engine2.dll) ─────────────────────────────────────────
inline std::ptrdiff_t dwNetworkGameClient      = 0x908820;

// ── Schema offsets: C_BaseEntity ─────────────────────────────────────────────
inline std::ptrdiff_t m_pGameSceneNode         = 0x330;
inline std::ptrdiff_t m_nSubclassID            = 0x2C8;
inline std::ptrdiff_t m_hOwnerEntity           = 0x520;

// ── CEntityInstance ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pEntity                = 0x10;

// ── CEntityIdentity ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_flags                  = 0x28;

// ── C_CSPlayerPawn ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_hHudModelArms          = 0x1B58;
inline std::ptrdiff_t m_bNeedToReApplyGloves   = 0x1655;
inline std::ptrdiff_t m_EconGloves             = 0x1658;

// ── C_BasePlayerPawn ──────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pWeaponServices        = 0x11E0;

// ── CCSPlayerController ───────────────────────────────────────────────────────
inline std::ptrdiff_t m_pInventoryServices      = 0x810;

// ── CCSPlayerController_InventoryServices ─────────────────────────────────────
inline std::ptrdiff_t m_unMusicID              = 0x58;

// ── CPlayer_WeaponServices ────────────────────────────────────────────────────
inline std::ptrdiff_t m_hMyWeapons             = 0x48;
inline std::ptrdiff_t m_hActiveWeapon          = 0x60;

// ── C_EconEntity ──────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_AttributeManager       = 0x1180;
inline std::ptrdiff_t m_nFallbackPaintKit      = 0x1658;
inline std::ptrdiff_t m_nFallbackStatTrak      = 0x1664;
inline std::ptrdiff_t m_flFallbackWear         = 0x1660;
inline std::ptrdiff_t m_nFallbackSeed          = 0x165C;
inline std::ptrdiff_t m_OriginalOwnerXuidLow   = 0x1650;

// ── C_AttributeContainer ─────────────────────────────────────────────────────
inline std::ptrdiff_t m_Item                   = 0x50;

// ── C_EconItemView ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_AttributeList           = 0x208;
inline std::ptrdiff_t m_NetworkedDynamicAttributes = 0x280;
inline std::ptrdiff_t m_iItemDefinitionIndex   = 0x1BA;
inline std::ptrdiff_t m_iAccountID             = 0x1D8;
inline std::ptrdiff_t m_iItemIDHigh            = 0x1D0;
inline std::ptrdiff_t m_iItemIDLow             = 0x1D4;
inline std::ptrdiff_t m_szCustomNameOverride   = 0x100;
inline std::ptrdiff_t m_bInitialized           = 0x1E8;
inline std::ptrdiff_t m_bDisallowSOC           = 0x1E9;
inline std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x1B8;
inline std::ptrdiff_t m_iEntityQuality         = 0x1BC;

// ── CAttributeList ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_Attributes             = 0x8;

// ── CSkeletonInstance / CModelState ───────────────────────────────────────────
inline std::ptrdiff_t m_modelState             = 0x150;
inline std::ptrdiff_t m_MeshGroupMask          = 0x1C8;

// ── CGameSceneNode ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pChild                 = 0x40;
inline std::ptrdiff_t m_pNextSibling           = 0x48;
inline std::ptrdiff_t m_pOwner                 = 0x30;

// ── CCSWeaponBaseVData ────────────────────────────────────────────────────────
inline std::ptrdiff_t m_bMeleeWeapon           = 0x71C;
inline std::ptrdiff_t m_WeaponType             = 0x730;

// ── CModelState dirty-data pointer (within model state) ───────────────────────
inline std::ptrdiff_t m_pDirtyModelData        = 0xD8;
inline std::ptrdiff_t m_DirtyMeshGroupMask     = 0x10;

// ── Legacy alias (active weapon resolved via m_hActiveWeapon) ─────────────────
inline std::ptrdiff_t m_pClippingWeapon        = 0;

// ── Signature patterns ────────────────────────────────────────────────────────
namespace Sigs {
    inline constexpr const char* RegenerateWeaponSkins =
        "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10";
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Full offset from weapon entity base to the attribute vector.
inline std::uint16_t WeaponAttributeListOffset() noexcept {
    auto total = m_AttributeManager + m_Item + m_AttributeList + m_Attributes;
    return static_cast<std::uint16_t>(total & 0xFFFF);
}

inline std::filesystem::path DataFilePath() {
    const char* appdata = getenv("APPDATA");
    if (!appdata) appdata = getenv("HOME");
    if (!appdata) return "cs2_offsets.txt";
    return std::filesystem::path(appdata) / "cs2-skin-changer" / "offsets.txt";
}

namespace detail {

inline std::optional<std::ptrdiff_t> ParseJsonInt(std::string_view json,
                                                   std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":";
    auto pos = json.find(needle);
    if (pos == std::string_view::npos) return std::nullopt;

    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return std::nullopt;

    std::ptrdiff_t val = 0;
    auto start = json.data() + pos;
    auto end   = json.data() + json.size();
    if (json[pos] == '0' && pos + 1 < json.size() &&
        (json[pos + 1] == 'x' || json[pos + 1] == 'X')) {
        std::from_chars(start + 2, end, val, 16);
    } else {
        std::from_chars(start, end, val, 10);
    }
    return val;
}

inline std::optional<std::ptrdiff_t> ParseClientField(std::string_view json,
                                                       std::string_view cls,
                                                       std::string_view field) {
    const std::string cls_needle = std::string("\"") + std::string(cls) + "\":";
    auto cls_pos = json.find(cls_needle);
    if (cls_pos == std::string_view::npos) return std::nullopt;

    auto fields_pos = json.find("\"fields\"", cls_pos);
    if (fields_pos == std::string_view::npos) return std::nullopt;

    const std::string field_needle = std::string("\"") + std::string(field) + "\":";
    auto field_pos = json.find(field_needle, fields_pos);
    if (field_pos == std::string_view::npos) return std::nullopt;

    field_pos += field_needle.size();
    while (field_pos < json.size() && (json[field_pos] == ' ' || json[field_pos] == '\t'))
        ++field_pos;
    if (field_pos >= json.size()) return std::nullopt;

    std::ptrdiff_t val = 0;
    auto start = json.data() + field_pos;
    auto end   = json.data() + json.size();
    std::from_chars(start, end, val, 10);
    return val;
}

inline std::string DownloadUrl(std::string_view url) {
    char temp_path[MAX_PATH]{};
    char temp_file[MAX_PATH]{};
    if (!GetTempPathA(MAX_PATH, temp_path)) return {};
    if (!GetTempFileNameA(temp_path, "csc", 0, temp_file)) return {};

    std::string result;
    if (SUCCEEDED(URLDownloadToFileA(nullptr, std::string(url).c_str(),
                                     temp_file, 0, nullptr))) {
        std::ifstream f(temp_file, std::ios::binary);
        if (f) {
            std::ostringstream ss;
            ss << f.rdbuf();
            result = ss.str();
        }
    }
    DeleteFileA(temp_file);
    return result;
}

} // namespace detail

/// Pull latest offsets from cs2-dumper on GitHub (best-effort).
inline bool TryUpdateFromWeb() {
    const auto offsets_json = detail::DownloadUrl(
        "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.json");
    const auto client_json = detail::DownloadUrl(
        "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.json");

    if (offsets_json.empty() || client_json.empty()) return false;

    auto apply = [&](std::string_view key, std::ptrdiff_t* dst) {
        if (auto v = detail::ParseJsonInt(offsets_json, key)) *dst = *v;
    };

    apply("dwEntityList",            &dwEntityList);
    apply("dwLocalPlayerController", &dwLocalPlayerController);
    apply("dwLocalPlayerPawn",       &dwLocalPlayerPawn);
    apply("dwGlobalVars",            &dwGlobalVars);
    apply("dwViewMatrix",            &dwViewMatrix);
    apply("dwGameRules",             &dwGameRules);
    apply("dwNetworkGameClient",     &dwNetworkGameClient);

    auto apply_field = [&](std::string_view cls, std::string_view field,
                           std::ptrdiff_t* dst) {
        if (auto v = detail::ParseClientField(client_json, cls, field)) *dst = *v;
    };

    apply_field("C_BaseEntity", "m_pGameSceneNode", &m_pGameSceneNode);
    apply_field("C_BaseEntity", "m_hOwnerEntity", &m_hOwnerEntity);
    apply_field("CGameSceneNode", "m_pChild", &m_pChild);
    apply_field("CGameSceneNode", "m_pNextSibling", &m_pNextSibling);
    apply_field("CGameSceneNode", "m_pOwner", &m_pOwner);

    apply_field("C_CSPlayerPawn", "m_hHudModelArms", &m_hHudModelArms);
    apply_field("C_CSPlayerPawn", "m_bNeedToReApplyGloves", &m_bNeedToReApplyGloves);
    apply_field("C_CSPlayerPawn", "m_EconGloves", &m_EconGloves);

    apply_field("C_BasePlayerPawn", "m_pWeaponServices", &m_pWeaponServices);
    apply_field("CCSPlayerController", "m_pInventoryServices", &m_pInventoryServices);
    apply_field("CCSPlayerController_InventoryServices", "m_unMusicID", &m_unMusicID);

    apply_field("CPlayer_WeaponServices", "m_hMyWeapons", &m_hMyWeapons);
    apply_field("CPlayer_WeaponServices", "m_hActiveWeapon", &m_hActiveWeapon);

    apply_field("C_EconEntity", "m_AttributeManager", &m_AttributeManager);
    apply_field("C_EconEntity", "m_nFallbackPaintKit", &m_nFallbackPaintKit);
    apply_field("C_EconEntity", "m_nFallbackStatTrak", &m_nFallbackStatTrak);
    apply_field("C_EconEntity", "m_flFallbackWear", &m_flFallbackWear);
    apply_field("C_EconEntity", "m_nFallbackSeed", &m_nFallbackSeed);
    apply_field("C_EconEntity", "m_OriginalOwnerXuidLow", &m_OriginalOwnerXuidLow);

    apply_field("C_AttributeContainer", "m_Item", &m_Item);

    apply_field("C_EconItemView", "m_AttributeList", &m_AttributeList);
    apply_field("C_EconItemView", "m_NetworkedDynamicAttributes",
                &m_NetworkedDynamicAttributes);
    apply_field("C_EconItemView", "m_iItemDefinitionIndex", &m_iItemDefinitionIndex);
    apply_field("C_EconItemView", "m_iAccountID", &m_iAccountID);
    apply_field("C_EconItemView", "m_iItemIDHigh", &m_iItemIDHigh);
    apply_field("C_EconItemView", "m_iItemIDLow", &m_iItemIDLow);
    apply_field("C_EconItemView", "m_bInitialized", &m_bInitialized);
    apply_field("C_EconItemView", "m_bDisallowSOC", &m_bDisallowSOC);
    apply_field("C_EconItemView", "m_bRestoreCustomMaterialAfterPrecache",
                &m_bRestoreCustomMaterialAfterPrecache);
    apply_field("C_EconItemView", "m_iEntityQuality", &m_iEntityQuality);

    apply_field("CAttributeList", "m_Attributes", &m_Attributes);
    apply_field("CSkeletonInstance", "m_modelState", &m_modelState);
    apply_field("CModelState", "m_MeshGroupMask", &m_MeshGroupMask);

    return true;
}

inline bool LoadFromFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return false;

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
        {"m_iItemIDLow",             &m_iItemIDLow},
        {"m_szCustomNameOverride",   &m_szCustomNameOverride},
        {"m_bInitialized",           &m_bInitialized},
        {"m_bDisallowSOC",           &m_bDisallowSOC},
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
            std::from_chars(val_str.data(), val_str.data() + val_str.size(), val, 10);

        if (val != 0 || val_str == "0") {
            *it->second = val;
            any = true;
        }
    }
    return any;
}

inline bool SaveToFile(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if (!f) return false;

    f << "# CS2 Offsets — auto-synced from cs2-dumper\n";
    f << "# Format: key = hex_value\n\n";

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
    write("m_hOwnerEntity",           m_hOwnerEntity);
    write("m_pChild",                 m_pChild);
    write("m_pNextSibling",           m_pNextSibling);
    write("m_pOwner",                 m_pOwner);
    f << "\n";
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
    write("m_iItemIDLow",             m_iItemIDLow);
    write("m_bInitialized",           m_bInitialized);
    write("m_bDisallowSOC",           m_bDisallowSOC);
    write("m_bRestoreCustomMaterialAfterPrecache", m_bRestoreCustomMaterialAfterPrecache);
    write("m_iEntityQuality",         m_iEntityQuality);
    write("m_Attributes",             m_Attributes);
    f << "\n";
    write("m_modelState",             m_modelState);
    write("m_MeshGroupMask",          m_MeshGroupMask);
    write("m_pDirtyModelData",        m_pDirtyModelData);
    write("m_DirtyMeshGroupMask",     m_DirtyMeshGroupMask);

    return true;
}

} // namespace Offsets
