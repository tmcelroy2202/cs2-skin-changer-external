#pragma once

#include <cstddef>
#include <cstdint>

/* ── CS2 Offsets ────────────────────────────────────────────────────────────
 *
 * Auto-generated offset dumps from cs2-dumper (a2x).
 * These correspond to client.dll schemas and external offsets.
 *
 * Regenerate with: https://github.com/a2x/cs2-dumper
 * Last updated: 2026-01-27
 * ──────────────────────────────────────────────────────────────────────────── */

namespace cs2_dumper {
    namespace schemas {
        namespace client_dll {
            // ── C_BaseEntity ─────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_pGameSceneNode = 0x308;
            constexpr std::ptrdiff_t m_nSubclassID = 0x2C8;
            constexpr std::ptrdiff_t m_hOwnerEntity = 0x2C0;

            // ── CEntityInstance ───────────────────────────────────────────────
            constexpr std::ptrdiff_t m_pEntity = 0x10;

            // ── CEntityIdentity ───────────────────────────────────────────────
            constexpr std::ptrdiff_t m_flags = 0x28;

            // ── C_CSPlayerPawn ────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_pClippingWeapon = 0x15D0;
            constexpr std::ptrdiff_t m_hHudModelArms = 0x1600;
            constexpr std::ptrdiff_t m_bNeedToReApplyGloves = 0x15F7;
            constexpr std::ptrdiff_t m_EconGloves = 0x15D8;

            // ── C_BasePlayerPawn ──────────────────────────────────────────────
            constexpr std::ptrdiff_t m_pWeaponServices = 0x10C0;

            // ── CCSPlayerController ───────────────────────────────────────────
            constexpr std::ptrdiff_t m_pInventoryServices = 0x6B0;

            // ── CCSPlayerController_InventoryServices ─────────────────────────
            constexpr std::ptrdiff_t m_unMusicID = 0x39;

            // ── CPlayer_WeaponServices ────────────────────────────────────────
            constexpr std::ptrdiff_t m_hMyWeapons = 0x30;
            constexpr std::ptrdiff_t m_hActiveWeapon = 0x38;

            // ── C_EconEntity ──────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_AttributeManager = 0x940;
            constexpr std::ptrdiff_t m_nFallbackPaintKit = 0x11D0;
            constexpr std::ptrdiff_t m_nFallbackStatTrak = 0x11D4;
            constexpr std::ptrdiff_t m_flFallbackWear = 0x11D8;
            constexpr std::ptrdiff_t m_nFallbackSeed = 0x11DC;
            constexpr std::ptrdiff_t m_OriginalOwnerXuidLow = 0x11E0;

            // ── C_AttributeContainer ─────────────────────────────────────────
            constexpr std::ptrdiff_t m_Item = 0x30;

            // ── C_EconItemView ───────────────────────────────────────────────
            constexpr std::ptrdiff_t m_AttributeList = 0x70;
            constexpr std::ptrdiff_t m_NetworkedDynamicAttributes = 0x80;
            constexpr std::ptrdiff_t m_iItemDefinitionIndex = 0xF0;
            constexpr std::ptrdiff_t m_iAccountID = 0xF4;
            constexpr std::ptrdiff_t m_iItemIDHigh = 0xF8;
            constexpr std::ptrdiff_t m_szCustomNameOverride = 0x100;
            constexpr std::ptrdiff_t m_bInitialized = 0x9C;
            constexpr std::ptrdiff_t m_bRestoreCustomMaterialAfterPrecache = 0x9D;
            constexpr std::ptrdiff_t m_iEntityQuality = 0xA0;

            // ── CAttributeList ────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_Attributes = 0x18;

            // ── CSkeletonInstance ─────────────────────────────────────────────
            constexpr std::ptrdiff_t m_modelState = 0x160;

            // ── CModelState ───────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_MeshGroupMask = 0x104;

            // ── CGameSceneNode ────────────────────────────────────────────────
            constexpr std::ptrdiff_t m_pChild = 0x38;
            constexpr std::ptrdiff_t m_pNextSibling = 0x40;
            constexpr std::ptrdiff_t m_pOwner = 0x30;

            // ── CCSWeaponBaseVData ────────────────────────────────────────────
            constexpr std::ptrdiff_t m_bMeleeWeapon = 0x908;
            constexpr std::ptrdiff_t m_WeaponType = 0x91C;
        }
    }

    namespace offsets {
        namespace client_dll {
            constexpr std::ptrdiff_t dwEntityList = 0x24A7B48;
            constexpr std::ptrdiff_t dwLocalPlayerController = 0x22ECA18;
            constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x2062850;
            constexpr std::ptrdiff_t dwGlobalVars = 0x20572A8;
            constexpr std::ptrdiff_t dwViewMatrix = 0x2308850;
            constexpr std::ptrdiff_t dwGameRules = 0x23035F0;
        }
        namespace engine2_dll {
            constexpr std::ptrdiff_t dwNetworkGameClient = 0x900FB0;
        }
    }
}

// ── Convenience alias ─────────────────────────────────────────────────────────
namespace Offsets {
    using namespace cs2_dumper::schemas::client_dll;
    using namespace cs2_dumper::offsets::client_dll;

    // Extra internal offsets not in the dumper
    constexpr std::ptrdiff_t m_pDirtyModelData = 0xD8;
    constexpr std::ptrdiff_t m_DirtyMeshGroupMask = 0x10;

    // Sigs
    namespace Sigs {
        constexpr const char* RegenerateWeaponSkins =
            "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10";
    }
}
