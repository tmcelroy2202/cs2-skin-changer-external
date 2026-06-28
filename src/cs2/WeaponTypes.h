#pragma once

#include "core/Core.h"

/* ── CS2 Weapon & Skin Types ─────────────────────────────────────────────────
 *
 * Ported from the reference: weapons enums, skin info, econ item definitions.
 * ──────────────────────────────────────────────────────────────────────────── */

#define HideHudFlag 128

enum class WeaponsEnum : u16 {
    None           = 0,
    Deagle         = 1,
    Elite          = 2,
    FiveSeven      = 3,
    Glock          = 4,
    Ak47           = 7,
    Aug            = 8,
    Awp            = 9,
    Famas          = 10,
    G3Sg1          = 11,
    Galil          = 13,
    M249           = 14,
    M4A4           = 16,
    Mac10          = 17,
    P90            = 19,
    Mp5SD          = 23,
    Ump45          = 24,
    Xm1014         = 25,
    Bizon          = 26,
    Mag7           = 27,
    Negev          = 28,
    Sawedoof       = 29,
    Tec9           = 30,
    Zeus           = 31,
    P200           = 32,
    Mp7            = 33,
    Mp9            = 34,
    Nova           = 35,
    p250           = 36,
    Scar20         = 38,
    Sg556          = 39,
    Ssg08          = 40,
    CtKnife        = 42,
    FlashBang      = 43,
    HeGrenade      = 44,
    SmokeGrenade   = 45,
    Molotov        = 46,
    Decoy          = 47,
    IncGrenade     = 48,
    C4             = 49,
    Tknife         = 59,
    M4A1Silencer   = 60,
    UspS           = 61,
    Cz65A          = 63,
    Revolver       = 64,

    // ── Cosmetic knives (from m_hMyWeapons, def indices 500+) ──────────
    Bayonet        = 500,
    FlipKnife      = 505,
    GutKnife       = 506,
    Karambit       = 507,
    M9Bayonet      = 508,
    Huntsman       = 509,
    Falchion       = 510,
    Bowie          = 514,
    Butterfly      = 515,
    ShadowDaggers  = 516,
    Survival       = 517,
    Ursus          = 519,
    Navaja         = 520,
    Stiletto       = 522,
    Talon          = 523,
    ClassicKnife   = 524,
    Nomad          = 525,
    Skeleton       = 526,
};

// ── Glove Definition Indices ──────────────────────────────────────────────────
enum class GlovesEnum : u16 {
    DefaultCT        = 5028,
    DefaultT         = 5029,
    Bloodhound       = 5027,
    Driver           = 5030,
    HandWraps        = 5031,
    Moto             = 5032,
    Specialist       = 5033,
    Sport            = 5034,
    Hydra            = 5035,
    BrokenFang       = 5036,
};

// ── All glove types for iteration ─────────────────────────────────────────────
inline constexpr GlovesEnum kAllGloveDefs[] = {
    GlovesEnum::Bloodhound,
    GlovesEnum::Driver,
    GlovesEnum::HandWraps,
    GlovesEnum::Moto,
    GlovesEnum::Specialist,
    GlovesEnum::Sport,
    GlovesEnum::Hydra,
    GlovesEnum::BrokenFang,
};

inline std::string GloveIdToString(i32 id) {
    switch (id) {
    case 5027: return "Bloodhound Gloves";
    case 5028: return "Default CT Gloves";
    case 5029: return "Default T Gloves";
    case 5030: return "Driver Gloves";
    case 5031: return "Hand Wraps";
    case 5032: return "Moto Gloves";
    case 5033: return "Specialist Gloves";
    case 5034: return "Sport Gloves";
    case 5035: return "Hydra Gloves";
    case 5036: return "Broken Fang Gloves";
    default:   return "Unknown Gloves";
    }
}

enum class CSWeaponType : u32 {
    Knife           = 0x0,
    Pistol          = 0x1,
    SubmachineGun   = 0x2,
    Rifle           = 0x3,
    Shotgun         = 0x4,
    SniperRifle     = 0x5,
    MachineGun      = 0x6,
    C4              = 0x7,
    Taser           = 0x8,
    Grenade         = 0x9,
    Equipment       = 0xA,
    StackableItem   = 0xB,
    Unknown         = 0xC,
};

struct SkinInfo_t {
    i32         Paint           = 0;
    f32         Wear            = 0.01f;
    i32         Pattern         = 0;
    bool        bUsesOldModel   = false;
    std::string name;
    WeaponsEnum weaponType      = WeaponsEnum::None;
};

inline std::string WeaponIdToString(i32 id) {
    switch (id) {
    case 0:  return "None";
    case 1:  return "Deagle";
    case 2:  return "Dual Berettas";
    case 3:  return "Five-SeveN";
    case 4:  return "Glock-18";
    case 7:  return "AK-47";
    case 8:  return "AUG";
    case 9:  return "AWP";
    case 10: return "FAMAS";
    case 11: return "G3SG1";
    case 13: return "Galil AR";
    case 14: return "M249";
    case 16: return "M4A4";
    case 17: return "MAC-10";
    case 19: return "P90";
    case 23: return "MP5-SD";
    case 24: return "UMP-45";
    case 25: return "XM1014";
    case 26: return "PP-Bizon";
    case 27: return "MAG-7";
    case 28: return "Negev";
    case 29: return "Sawed-Off";
    case 30: return "Tec-9";
    case 31: return "Zeus x27";
    case 32: return "P2000";
    case 33: return "MP7";
    case 34: return "MP9";
    case 35: return "Nova";
    case 36: return "P250";
    case 38: return "SCAR-20";
    case 39: return "SG 553";
    case 40: return "SSG 08";
    case 42: return "Knife";
    case 43: return "Flashbang";
    case 44: return "HE Grenade";
    case 45: return "Smoke Grenade";
    case 46: return "Molotov";
    case 47: return "Decoy";
    case 48: return "Incendiary";
    case 49: return "C4";
    case 59: return "Knife (T)";
    case 60: return "M4A1-S";
    case 61: return "USP-S";
    case 63: return "CZ75-Auto";
    case 64: return "R8 Revolver";
    case 500: return "★ Bayonet";
    case 505: return "★ Flip Knife";
    case 506: return "★ Gut Knife";
    case 507: return "★ Karambit";
    case 508: return "★ M9 Bayonet";
    case 509: return "★ Huntsman Knife";
    case 510: return "★ Falchion Knife";
    case 514: return "★ Bowie Knife";
    case 515: return "★ Butterfly Knife";
    case 516: return "★ Shadow Daggers";
    case 517: return "★ Survival Knife";
    case 519: return "★ Ursus Knife";
    case 520: return "★ Navaja Knife";
    case 522: return "★ Stiletto Knife";
    case 523: return "★ Talon Knife";
    case 524: return "★ Classic Knife";
    case 525: return "★ Nomad Knife";
    case 526: return "★ Skeleton Knife";
    default: return "Unknown";
    }
}

inline CSWeaponType WeaponEnumToType(WeaponsEnum weapon) {
    switch (weapon) {
    case WeaponsEnum::CtKnife:
    case WeaponsEnum::Tknife:
        return CSWeaponType::Knife;
    case WeaponsEnum::Deagle:
    case WeaponsEnum::Elite:
    case WeaponsEnum::FiveSeven:
    case WeaponsEnum::Glock:
    case WeaponsEnum::Tec9:
    case WeaponsEnum::P200:
    case WeaponsEnum::p250:
    case WeaponsEnum::UspS:
    case WeaponsEnum::Cz65A:
    case WeaponsEnum::Revolver:
        return CSWeaponType::Pistol;
    case WeaponsEnum::Mac10:
    case WeaponsEnum::Mp7:
    case WeaponsEnum::Mp9:
    case WeaponsEnum::P90:
    case WeaponsEnum::Bizon:
    case WeaponsEnum::Ump45:
    case WeaponsEnum::Mp5SD:
        return CSWeaponType::SubmachineGun;
    case WeaponsEnum::Ak47:
    case WeaponsEnum::Famas:
    case WeaponsEnum::Galil:
    case WeaponsEnum::Aug:
    case WeaponsEnum::Sg556:
    case WeaponsEnum::M4A4:
    case WeaponsEnum::M4A1Silencer:
        return CSWeaponType::Rifle;
    case WeaponsEnum::Awp:
    case WeaponsEnum::Ssg08:
    case WeaponsEnum::Scar20:
    case WeaponsEnum::G3Sg1:
        return CSWeaponType::SniperRifle;
    case WeaponsEnum::Nova:
    case WeaponsEnum::Xm1014:
    case WeaponsEnum::Mag7:
    case WeaponsEnum::Sawedoof:
        return CSWeaponType::Shotgun;
    case WeaponsEnum::M249:
    case WeaponsEnum::Negev:
        return CSWeaponType::MachineGun;
    case WeaponsEnum::Zeus:
        return CSWeaponType::Taser;
    case WeaponsEnum::FlashBang:
    case WeaponsEnum::HeGrenade:
    case WeaponsEnum::SmokeGrenade:
    case WeaponsEnum::Molotov:
    case WeaponsEnum::IncGrenade:
    case WeaponsEnum::Decoy:
        return CSWeaponType::Grenade;
    case WeaponsEnum::C4:
        return CSWeaponType::C4;
    default:
        return CSWeaponType::Unknown;
    }
}

// ── All knife definition IDs for iteration ────────────────────────────────────
inline constexpr WeaponsEnum kAllKnifeDefs[] = {
    WeaponsEnum::Bayonet, WeaponsEnum::FlipKnife, WeaponsEnum::GutKnife,
    WeaponsEnum::Karambit, WeaponsEnum::M9Bayonet, WeaponsEnum::Huntsman,
    WeaponsEnum::Falchion, WeaponsEnum::Bowie, WeaponsEnum::Butterfly,
    WeaponsEnum::ShadowDaggers, WeaponsEnum::Survival, WeaponsEnum::Ursus,
    WeaponsEnum::Navaja, WeaponsEnum::Stiletto, WeaponsEnum::Talon,
    WeaponsEnum::ClassicKnife, WeaponsEnum::Nomad, WeaponsEnum::Skeleton,
};

// ── All weapon definition IDs for iteration ──────────────────────────────────
inline constexpr WeaponsEnum kAllWeaponDefs[] = {
    WeaponsEnum::Deagle, WeaponsEnum::Elite, WeaponsEnum::FiveSeven,
    WeaponsEnum::Glock, WeaponsEnum::Ak47, WeaponsEnum::Aug,
    WeaponsEnum::Awp, WeaponsEnum::Famas, WeaponsEnum::G3Sg1,
    WeaponsEnum::Galil, WeaponsEnum::M249, WeaponsEnum::M4A4,
    WeaponsEnum::Mac10, WeaponsEnum::P90, WeaponsEnum::Mp5SD,
    WeaponsEnum::Ump45, WeaponsEnum::Xm1014, WeaponsEnum::Bizon,
    WeaponsEnum::Mag7, WeaponsEnum::Negev, WeaponsEnum::Sawedoof,
    WeaponsEnum::Tec9, WeaponsEnum::P200, WeaponsEnum::Mp7,
    WeaponsEnum::Mp9, WeaponsEnum::Nova, WeaponsEnum::p250,
    WeaponsEnum::Scar20, WeaponsEnum::Sg556, WeaponsEnum::Ssg08,
    WeaponsEnum::M4A1Silencer, WeaponsEnum::UspS, WeaponsEnum::Cz65A,
    WeaponsEnum::Revolver,
    // Knives
    WeaponsEnum::Bayonet, WeaponsEnum::FlipKnife, WeaponsEnum::GutKnife,
    WeaponsEnum::Karambit, WeaponsEnum::M9Bayonet, WeaponsEnum::Huntsman,
    WeaponsEnum::Falchion, WeaponsEnum::Bowie, WeaponsEnum::Butterfly,
    WeaponsEnum::ShadowDaggers, WeaponsEnum::Survival, WeaponsEnum::Ursus,
    WeaponsEnum::Navaja, WeaponsEnum::Stiletto, WeaponsEnum::Talon,
    WeaponsEnum::ClassicKnife, WeaponsEnum::Nomad, WeaponsEnum::Skeleton,
};
