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
#include <vector>

#include "core/Logging.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

/* ── CS2 Offsets ────────────────────────────────────────────────────────────
 *
 * Runtime-resolvable offsets for CS2.  All values are mutable `inline`
 * variables, loaded from an external data file at startup so they survive
 * game updates without recompiling.  Ship cs2-dumper defaults embedded.
 *
 * When CS2 patches, the offsets can be auto-updated from the cs2-dumper
 * project's GitHub output at startup (see UpdateFromRemote).
 *
 * Regenerate defaults: https://github.com/a2x/cs2-dumper
 * ──────────────────────────────────────────────────────────────────────────── */

namespace Offsets {

// ── External addresses (client.dll) ──────────────────────────────────────────
inline std::ptrdiff_t dwEntityList             = 0x24E7680;
inline std::ptrdiff_t dwLocalPlayerController  = 0x2320570;
inline std::ptrdiff_t dwLocalPlayerPawn        = 0x2341528;
inline std::ptrdiff_t dwGlobalVars             = 0x20616D0;
inline std::ptrdiff_t dwViewMatrix             = 0x23469C0;
inline std::ptrdiff_t dwGameRules              = 0x2340FE8;

// ── External addresses (engine2.dll) ─────────────────────────────────────────
inline std::ptrdiff_t dwNetworkGameClient      = 0x90A1A0;

// ── Schema offsets: C_BaseEntity ─────────────────────────────────────────────
inline std::ptrdiff_t m_pGameSceneNode         = 0x330;
inline std::ptrdiff_t m_nSubclassID            = 0x380;
inline std::ptrdiff_t m_hOwnerEntity           = 0x520;

// ── CEntityInstance ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pEntity                = 0x10;

// ── CEntityIdentity ───────────────────────────────────────────────────────────
inline std::ptrdiff_t m_flags                  = 0x30;

// ── C_CSPlayerPawn ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_iTeamNum               = 0x3EB;
inline std::ptrdiff_t m_pClippingWeapon        = 0x15D0;
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
inline std::ptrdiff_t m_OriginalOwnerXuidHigh  = 0x1654;

// ── C_AttributeContainer ─────────────────────────────────────────────────────
inline std::ptrdiff_t m_Item                   = 0x50;

// ── C_EconItemView ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_AttributeList           = 0x208;
inline std::ptrdiff_t m_NetworkedDynamicAttributes = 0x280;
inline std::ptrdiff_t m_iItemDefinitionIndex   = 0x1BA;
inline std::ptrdiff_t m_iAccountID             = 0x1D8;
inline std::ptrdiff_t m_iItemID                = 0x1C8; // uint64 — combined item ID (write -1 for custom)
inline std::ptrdiff_t m_iItemIDHigh            = 0x1D0;
inline std::ptrdiff_t m_iItemIDLow             = 0x1D4;
inline std::ptrdiff_t m_szCustomNameOverride   = 0x399;
inline std::ptrdiff_t m_bInitialized           = 0x1E8;
inline std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x1B8;
inline std::ptrdiff_t m_iEntityQuality         = 0x1BC;

// ── CAttributeList ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_Attributes             = 0x8;

// ── CSkeletonInstance / CModelState ───────────────────────────────────────────
inline std::ptrdiff_t m_modelState             = 0x150;
inline std::ptrdiff_t m_MeshGroupMask          = 0x1C8;

// ── CGameSceneNode ────────────────────────────────────────────────────────────
inline std::ptrdiff_t m_pParent                = 0x38;
inline std::ptrdiff_t m_pChild                 = 0x40;
inline std::ptrdiff_t m_pNextSibling           = 0x48;
inline std::ptrdiff_t m_pOwner                 = 0x30;

// ── CCSWeaponBaseVData ────────────────────────────────────────────────────────
inline std::ptrdiff_t m_bMeleeWeapon           = 0x71C;
inline std::ptrdiff_t m_WeaponType             = 0x520;

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
    return std::filesystem::path(appdata) / "veneer" / "offsets.txt";
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
        {"m_iTeamNum",               &m_iTeamNum},
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
        {"m_iItemID",                &m_iItemID},
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
        {"m_iItemIDLow",             &m_iItemIDLow},
        {"m_OriginalOwnerXuidHigh",  &m_OriginalOwnerXuidHigh},
        {"m_pParent",                &m_pParent},
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
    write("m_iTeamNum",               m_iTeamNum);
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
    write("m_OriginalOwnerXuidHigh",  m_OriginalOwnerXuidHigh);
    write("m_Item",                   m_Item);
    write("m_AttributeList",          m_AttributeList);
    write("m_NetworkedDynamicAttributes", m_NetworkedDynamicAttributes);
    write("m_iItemDefinitionIndex",   m_iItemDefinitionIndex);
    write("m_iAccountID",             m_iAccountID);
    write("m_iItemID",                m_iItemID);
    write("m_iItemIDHigh",            m_iItemIDHigh);
    write("m_iItemIDLow",             m_iItemIDLow);
    write("m_szCustomNameOverride",   m_szCustomNameOverride);
    write("m_bInitialized",           m_bInitialized);
    write("m_bRestoreCustomMaterialAfterPrecache", m_bRestoreCustomMaterialAfterPrecache);
    write("m_iEntityQuality",         m_iEntityQuality);
    write("m_Attributes",             m_Attributes);
    f << "\n";
    write("m_modelState",             m_modelState);
    write("m_MeshGroupMask",          m_MeshGroupMask);
    write("m_pParent",                m_pParent);
    write("m_pChild",                 m_pChild);
    write("m_pNextSibling",           m_pNextSibling);
    write("m_pOwner",                 m_pOwner);
    write("m_bMeleeWeapon",           m_bMeleeWeapon);
    write("m_WeaponType",             m_WeaponType);
    write("m_pDirtyModelData",        m_pDirtyModelData);
    write("m_DirtyMeshGroupMask",     m_DirtyMeshGroupMask);

    return true;
}

/// Validate loaded offsets against known-bad values.
/// After auto-update or LoadFromFile, some offsets may have been set to the
/// wrong class's value (e.g. m_AttributeManager from C_PlantedC4 instead of
/// C_EconEntity).  Call this after loading to fix any such stale values.
inline void ValidateLoadedOffsets() noexcept {
    // m_AttributeManager must be from C_EconEntity (0x1180).
    // Known wrong values: C_Chicken (0x13B8), C_PlantedC4 (0x11C0).
    if (m_AttributeManager == 0x13B8 || m_AttributeManager == 0x11C0) {
        m_AttributeManager = 0x1180;
    }
}

/// Reset all offsets to cs2-dumper embedded defaults (already set at init).
inline void ResetToDefaults() {
    // The inline variable initialisers are already the defaults.
    // To force a reload from defaults, the simplest path is to delete the
    // data file — LoadFromFile will fail and the built-in values stay.
}

// ── Auto-update from cs2-dumper GitHub ─────────────────────────────────────-
// These constants control which URLs are fetched at startup.
inline constexpr const char* kCs2DumperOffsetsUrl =
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.hpp";
inline constexpr const char* kCs2DumperClientDllUrl =
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp";

/// Download a URL to a local file using URLDownloadToFileA (resolved dynamically).
inline bool DownloadUrl(const char* url, const std::filesystem::path& out) noexcept {
    HMODULE urlmon = ::LoadLibraryW(L"urlmon.dll");
    if (!urlmon) return false;

    // "URLDownloadToFileA" XOR 0x5C
    static constexpr unsigned char kName[] = {
        0x09, 0x0E, 0x10, 0x18, 0x33, 0x2B, 0x32, 0x30,
        0x33, 0x3D, 0x38, 0x08, 0x33, 0x1A, 0x35, 0x30,
        0x39, 0x1D };
    char name[19]{};
    for (int i = 0; i < 18; ++i) name[i] = static_cast<char>(kName[i] ^ 0x5C);

    using Fn = HRESULT(STDAPICALLTYPE*)(void*, const char*, const char*, DWORD, void*);
    auto fn = reinterpret_cast<Fn>(::GetProcAddress(urlmon, name));

    if (!fn) { ::FreeLibrary(urlmon); return false; }

    std::string out_str = out.string();
    HRESULT hr = fn(nullptr, url, out_str.c_str(), 0, nullptr);
    ::FreeLibrary(urlmon);
    return SUCCEEDED(hr);
}

/// Parse one cs2-dumper C++ header line.
/// Extracts the identifier (stripping namespace prefixes) and hex value.
inline bool ParseCs2Line(const std::string& line,
                         std::string& out_key,
                         std::ptrdiff_t& out_val) noexcept {
    // Look for: constexpr std::ptrdiff_t  IDENTIFIER  =  0xVALUE;
    constexpr const char prefix[] = "constexpr std::ptrdiff_t ";
    auto pos = line.find(prefix);
    if (pos == std::string::npos) return false;

    auto start = pos + sizeof(prefix) - 1;
    // skip spaces
    while (start < line.size() && line[start] == ' ') ++start;

    // extract identifier (until space or =)
    auto end_id = start;
    while (end_id < line.size() && line[end_id] != ' ' && line[end_id] != '=') ++end_id;
    if (end_id == start) return false;

    std::string ident = line.substr(start, end_id - start);

    // Strip namespace prefixes (take last component after ::)
    auto scope = ident.rfind("::");
    if (scope != std::string::npos) ident = ident.substr(scope + 2);

    // Find hex value after '='
    auto eq = line.find('=', end_id);
    if (eq == std::string::npos) return false;

    auto vpos = eq + 1;
    // Skip spaces then optional 0x/0X prefix
    while (vpos < line.size() && line[vpos] == ' ') ++vpos;
    if (vpos + 1 < line.size() && line[vpos] == '0' &&
        ((line[vpos + 1] | 0x20) == 'x'))
        vpos += 2;

    if (vpos >= line.size()) return false;

    // Read hex digits until ; or space
    std::string hex_str;
    while (vpos < line.size() && line[vpos] != ';' && line[vpos] != ' ' && line[vpos] != '\r' && line[vpos] != '\n') {
        hex_str += line[vpos];
        ++vpos;
    }

    if (hex_str.empty()) return false;

    std::ptrdiff_t val = 0;
    auto [p, ec] = std::from_chars(hex_str.data(), hex_str.data() + hex_str.size(), val, 16);
    if (ec != std::errc{}) return false;

    out_key = ident;
    out_val = val;
    return true;
}

/// Parse a line for namespace opening: "namespace X {" → returns X, else ""
/// cs2-dumper format has a space before '{':  "namespace C_EconEntity {"
inline std::string_view ParseNamespaceOpen(const std::string& line) noexcept {
    constexpr const char ns_prefix[] = "namespace ";
    auto pos = line.find(ns_prefix);
    if (pos == std::string::npos) return {};
    auto start = pos + sizeof(ns_prefix) - 1;
    // skip spaces
    while (start < line.size() && line[start] == ' ') ++start;
    auto end = start;
    while (end < line.size() && line[end] != ' ' && line[end] != '{' && line[end] != '\r' && line[end] != '\n') ++end;
    if (end == start) return {};
    auto ident_end = end;
    // skip whitespace between identifier and '{'
    while (end < line.size() && line[end] == ' ') ++end;
    if (end >= line.size() || line[end] != '{') return {};
    return std::string_view(line).substr(start, ident_end - start);
}

/// Fetch the latest offsets from cs2-dumper's GitHub output and save them
/// to the local offsets file.  Best-effort: silently returns false on failure.
///
/// Class-aware: tracks namespace context so that ambiguous offsets like
/// m_AttributeManager (which exists in C_Chicken, C_EconEntity, C_PlantedC4)
/// are resolved from the correct class.
inline bool UpdateFromRemote() noexcept {
    LOG_INFO("UpdateFromRemote: downloading offsets.hpp ...");
    auto tmp_offsets = std::filesystem::temp_directory_path() / "cs2_offsets_dl.tmp";
    if (!DownloadUrl(kCs2DumperOffsetsUrl, tmp_offsets)) {
        LOG_WARN("UpdateFromRemote: failed to download offsets.hpp");
        return false;
    }
    LOG_INFO("UpdateFromRemote: offsets.hpp downloaded OK");

    LOG_INFO("UpdateFromRemote: downloading client_dll.hpp ...");
    auto tmp_client = std::filesystem::temp_directory_path() / "cs2_client_dl.tmp";
    if (!DownloadUrl(kCs2DumperClientDllUrl, tmp_client)) {
        LOG_WARN("UpdateFromRemote: failed to download client_dll.hpp (best-effort, continuing)");
    } else {
        LOG_INFO("UpdateFromRemote: client_dll.hpp downloaded OK");
    }

    // ── Class-aware parse ──────────────────────────────────────────────────
    // Track namespace stack to know which class each field belongs to.
    // Store results as (class_name, field_name) → value.
    struct ParsedField {
        std::string class_name;
        std::string field_name;
        std::ptrdiff_t value;
    };
    std::vector<ParsedField> parsed_fields;
    // Also keep a flat map for dw* offsets (no class context needed)
    std::unordered_map<std::string, std::ptrdiff_t> parsed_flat;

    auto parse_file_classaware = [&](const std::filesystem::path& p, bool track_ns) {
        std::ifstream f(p);
        if (!f) return;
        std::string line;
        std::vector<std::string> ns_stack;
        int brace_depth = 0;
        // Track brace depth per namespace entry so we pop correctly
        std::vector<int> ns_depths;

        while (std::getline(f, line)) {
            // Detect namespace opening BEFORE counting braces so we only
            // call ParseNamespaceOpen once per line (avoids double-push
            // if a line has multiple '{' chars).
            std::string_view pending_ns;
            if (track_ns) {
                pending_ns = ParseNamespaceOpen(line);
            }

            // Count braces (simple: ignore braces inside strings/comments — good enough for cs2-dumper output)
            bool ns_pushed = false;
            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == '{') {
                    ++brace_depth;
                    if (track_ns && !pending_ns.empty() && !ns_pushed) {
                        ns_stack.emplace_back(pending_ns);
                        ns_depths.push_back(brace_depth);
                        ns_pushed = true;
                    }
                } else if (c == '}') {
                    if (track_ns && !ns_stack.empty() && brace_depth == ns_depths.back()) {
                        ns_stack.pop_back();
                        ns_depths.pop_back();
                    }
                    --brace_depth;
                }
            }

            std::string key;
            std::ptrdiff_t val;
            if (ParseCs2Line(line, key, val)) {
                if (track_ns && !ns_stack.empty()) {
                    parsed_fields.push_back({ns_stack.back(), key, val});
                } else {
                    parsed_flat[key] = val;
                }
            }
        }
    };

    // offsets.hpp has no class namespaces (just dw* constants) — flat parse
    parse_file_classaware(tmp_offsets, false);
    LOG_INFO("UpdateFromRemote: parsed {} flat offsets from offsets.hpp", parsed_flat.size());

    // client_dll.hpp has nested namespace classes — class-aware parse
    if (std::filesystem::exists(tmp_client))
        parse_file_classaware(tmp_client, true);
    LOG_INFO("UpdateFromRemote: parsed {} class-aware fields from client_dll.hpp", parsed_fields.size());

    // Clean up temp files
    std::error_code ec;
    std::filesystem::remove(tmp_offsets, ec);
    std::filesystem::remove(tmp_client, ec);

    if (parsed_fields.empty() && parsed_flat.empty()) return false;

    // ── Offset table with class hints ──────────────────────────────────────
    // For ambiguous offsets (multiple classes define the same field name),
    // we specify which class to accept.  Empty string = accept any class.
    struct OffsetEntry {
        std::ptrdiff_t* ptr;
        const char* required_class;  // nullptr or "" = accept any
    };

    std::unordered_map<std::string, OffsetEntry> table;

    auto add = [&](const char* name, std::ptrdiff_t* ptr, const char* required_class = nullptr) {
        table[name] = {ptr, required_class};
    };

    // dw* offsets (no class context, come from flat parse)
    add("dwEntityList",             &dwEntityList);
    add("dwLocalPlayerController",  &dwLocalPlayerController);
    add("dwLocalPlayerPawn",        &dwLocalPlayerPawn);
    add("dwGlobalVars",             &dwGlobalVars);
    add("dwViewMatrix",             &dwViewMatrix);
    add("dwGameRules",              &dwGameRules);
    add("dwNetworkGameClient",      &dwNetworkGameClient);

    // C_BaseEntity
    add("m_pGameSceneNode",         &m_pGameSceneNode);
    add("m_nSubclassID",            &m_nSubclassID);
    add("m_hOwnerEntity",           &m_hOwnerEntity);
    add("m_pEntity",                &m_pEntity);
    add("m_flags",                  &m_flags);
    add("m_iTeamNum",               &m_iTeamNum);

    // C_CSPlayerPawn / C_BasePlayerPawn
    add("m_pClippingWeapon",        &m_pClippingWeapon);
    add("m_hHudModelArms",          &m_hHudModelArms);
    add("m_bNeedToReApplyGloves",   &m_bNeedToReApplyGloves);
    add("m_EconGloves",             &m_EconGloves);
    add("m_pWeaponServices",        &m_pWeaponServices);
    add("m_pInventoryServices",     &m_pInventoryServices);

    // CPlayer_WeaponServices
    add("m_hMyWeapons",             &m_hMyWeapons);
    add("m_hActiveWeapon",          &m_hActiveWeapon);

    // CCSPlayerController_InventoryServices
    add("m_unMusicID",              &m_unMusicID);

    // CRITICAL: m_AttributeManager exists in C_Chicken (0x13B8), C_EconEntity (0x1180),
    // and C_PlantedC4 (0x11C0).  We MUST match C_EconEntity for weapon skins.
    add("m_AttributeManager",       &m_AttributeManager, "C_EconEntity");

    // C_EconEntity / C_EconItemView fallback offsets
    add("m_nFallbackPaintKit",      &m_nFallbackPaintKit);
    add("m_nFallbackStatTrak",      &m_nFallbackStatTrak);
    add("m_flFallbackWear",         &m_flFallbackWear);
    add("m_nFallbackSeed",          &m_nFallbackSeed);
    add("m_OriginalOwnerXuidLow",   &m_OriginalOwnerXuidLow);
    add("m_OriginalOwnerXuidHigh",  &m_OriginalOwnerXuidHigh);

    // C_AttributeContainer
    add("m_Item",                   &m_Item);

    // C_EconItemView
    add("m_AttributeList",          &m_AttributeList);
    add("m_NetworkedDynamicAttributes", &m_NetworkedDynamicAttributes);
    add("m_iItemDefinitionIndex",   &m_iItemDefinitionIndex);
    add("m_iAccountID",             &m_iAccountID);
    add("m_iItemID",                &m_iItemID);
    add("m_iItemIDHigh",            &m_iItemIDHigh);
    add("m_iItemIDLow",             &m_iItemIDLow);
    add("m_szCustomNameOverride",   &m_szCustomNameOverride);
    add("m_bInitialized",           &m_bInitialized);
    add("m_bRestoreCustomMaterialAfterPrecache", &m_bRestoreCustomMaterialAfterPrecache);
    add("m_iEntityQuality",         &m_iEntityQuality);

    // CAttributeList
    add("m_Attributes",             &m_Attributes);

    // CSkeletonInstance / CModelState / CGameSceneNode
    add("m_modelState",             &m_modelState);
    add("m_MeshGroupMask",          &m_MeshGroupMask);
    add("m_pParent",                &m_pParent);
    add("m_pChild",                 &m_pChild);
    add("m_pNextSibling",           &m_pNextSibling);
    add("m_pOwner",                 &m_pOwner);

    // CCSWeaponBaseVData
    add("m_bMeleeWeapon",           &m_bMeleeWeapon);
    add("m_WeaponType",             &m_WeaponType);

    // Extra
    add("m_pDirtyModelData",        &m_pDirtyModelData);
    add("m_DirtyMeshGroupMask",     &m_DirtyMeshGroupMask);

    // ── Apply parsed values ────────────────────────────────────────────────
    int applied = 0;
    int skipped_class = 0;

    // First: apply flat-parsed offsets (dw* from offsets.hpp)
    for (auto& [k, v] : parsed_flat) {
        auto it = table.find(k);
        if (it != table.end() && (!it->second.required_class || it->second.required_class[0] == '\0')) {
            *it->second.ptr = v;
            ++applied;
        }
    }

    // Second: apply class-aware parsed offsets (m_* from client_dll.hpp)
    for (auto& pf : parsed_fields) {
        auto it = table.find(pf.field_name);
        if (it == table.end()) continue;

        // If this offset requires a specific class, check for match
        if (it->second.required_class && it->second.required_class[0] != '\0') {
            if (pf.class_name != it->second.required_class) {
                ++skipped_class;
                continue;
            }
        }

        *it->second.ptr = pf.value;
        ++applied;
    }

    LOG_INFO("UpdateFromRemote: applied {} offsets, skipped {} class-mismatched", applied, skipped_class);

    // Persist to local file for next launch (best-effort)
    SaveToFile(DataFilePath());
    return true;
}

} // namespace Offsets
