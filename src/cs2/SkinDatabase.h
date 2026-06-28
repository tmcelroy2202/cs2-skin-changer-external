#pragma once

#include "core/Core.h"
#include "WeaponTypes.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

/* ── CS2 Skin Database ──────────────────────────────────────────────────────
 *
 * Port of the reference's CSkinDB.  Contains the top 5 most desirable skins
 * per weapon (by market popularity/value), plus a full glove paint kit DB.
 * ──────────────────────────────────────────────────────────────────────────── */

// ── PaintKit entry ────────────────────────────────────────────────────────────
struct PaintKitEntry {
    i32         paint_id;
    std::string name;
    bool        legacy_model;
};

// ── Weapon-to-skins mapping ───────────────────────────────────────────────────
using WeaponSkinMap = std::unordered_map<i32, std::vector<PaintKitEntry>>;

class SkinDatabase {
public:
    SkinDatabase() { BuildDefaultDB(); }

    std::vector<SkinInfo_t> GetWeaponSkins(
        WeaponsEnum weapon, bool include_vanilla = true) const noexcept
    {
        std::vector<SkinInfo_t> results;
        if (include_vanilla)
            results.push_back({0, 0.01f, 0, false, "Vanilla", WeaponsEnum::None});

        i32 def = static_cast<i32>(weapon);
        auto it = db_.find(def);
        if (it == db_.end()) return results;

        // Deduplicate by name: keep first occurrence of each skin name
        std::unordered_set<std::string_view> seen;
        for (auto& entry : it->second) {
            if (seen.count(entry.name)) continue;
            seen.insert(entry.name);
            results.push_back({entry.paint_id, 0.01f, 0, entry.legacy_model, entry.name, weapon});
        }
        return results;
    }

    SkinInfo_t FindSkin(WeaponsEnum weapon,
                        std::string_view name) const noexcept {
        i32 def = static_cast<i32>(weapon);
        auto it = db_.find(def);
        if (it == db_.end()) return {};

        for (auto& entry : it->second) {
            if (entry.name.find(name) != std::string_view::npos) {
                return {entry.paint_id, 0.01f, 0, entry.legacy_model, entry.name, weapon};
            }
        }
        return {};
    }

    // ── Glove helpers ──────────────────────────────────────────────────────
    std::vector<SkinInfo_t> GetGloveSkins(GlovesEnum glove,
                                          bool include_default = true) const noexcept
    {
        std::vector<SkinInfo_t> results;
        if (include_default)
            results.push_back({0, 0.01f, 0, false, "Default", WeaponsEnum::None});

        auto it = gloves_.find(static_cast<i32>(glove));
        if (it == gloves_.end()) return results;

        for (auto& entry : it->second) {
            results.push_back({entry.paint_id, 0.01f, 0, false, entry.name, WeaponsEnum::None});
        }
        return results;
    }

    std::vector<PaintKitEntry> GetAllGlovePaints(GlovesEnum glove) const noexcept {
        auto it = gloves_.find(static_cast<i32>(glove));
        if (it == gloves_.end()) return {};
        return it->second;
    }

private:
    WeaponSkinMap db_;
    WeaponSkinMap gloves_;

    void Add(i32 weapon_id, i32 paint_id, std::string_view name,
             bool legacy = false) noexcept {
        db_[weapon_id].push_back({paint_id, std::string(name), legacy});
    }

    // Add all popular knife finishes for a given knife def index
    void AddKnifePaints(i32 def) noexcept {
        // Ultra-rare / Covert
        Add(def, 415, "Doppler Phase 1");
        Add(def, 416, "Doppler Phase 2");
        Add(def, 417, "Doppler Phase 3");
        Add(def, 418, "Doppler Phase 4");
        Add(def, 419, "Doppler Ruby");
        Add(def, 420, "Doppler Sapphire");
        Add(def, 421, "Doppler Black Pearl");
        Add(def, 568, "Gamma Doppler Emerald");
        Add(def, 569, "Gamma Doppler Phase 1");
        Add(def, 570, "Gamma Doppler Phase 2");
        Add(def, 571, "Gamma Doppler Phase 3");
        Add(def, 572, "Gamma Doppler Phase 4");
        // Popular finishes
        Add(def, 38,  "Fade");
        Add(def, 59,  "Slaughter");
        Add(def, 413, "Marble Fade");
        Add(def, 409, "Tiger Tooth");
        Add(def, 410, "Damascus Steel");
        Add(def, 44,  "Case Hardened");
        Add(def, 12,  "Crimson Web");
        Add(def, 98,  "Ultraviolet");
        Add(def, 42,  "Blue Steel");
        Add(def, 43,  "Stained");
        Add(def, 40,  "Night");
        Add(def, 247, "Damascus Steel");
        // Lore
        Add(def, 558, "Lore");
        // Autotronic
        Add(def, 573, "Autotronic");
        // Freehand
        Add(def, 580, "Freehand");
        // Black Laminate
        Add(def, 563, "Black Laminate");
    }

    void AddGlove(i32 glove_id, i32 paint_id, std::string_view name) noexcept {
        gloves_[glove_id].push_back({paint_id, std::string(name), false});
    }

    void BuildDefaultDB() noexcept {
        // ═════════════════════════════════════════════════════════════════════
        //  PISTOLS
        // ═════════════════════════════════════════════════════════════════════

        // DEAGLE (1)
        Add(1, 962, "Printstream");
        Add(1, 37,  "Blaze");
        Add(1, 328, "Hand Cannon");
        Add(1, 711, "Code Red");
        Add(1, 527, "Kumicho Dragon");
        Add(1, 351, "Conspiracy");
        Add(1, 476, "Hand Cannon");
        Add(1, 444, "Hypnotic");
        Add(1, 631, "Printstream");

        // DUAL BERETTAS (2)
        Add(2, 978, "Dezastre");
        Add(2, 747, "Twin Turbo");
        Add(2, 625, "Royal Consorts");
        Add(2, 491, "Dualing Dragons");
        Add(2, 903, "Elite 1.6");
        Add(2, 596, "Limelight");
        Add(2, 112, "Cobalt Quartz");

        // FIVE-SEVEN (3)
        Add(3, 979, "Fairy Tale");
        Add(3, 427, "Monkey Business");
        Add(3, 430, "Hyper Beast");
        Add(3, 837, "Angry Mob");
        Add(3, 44,  "Case Hardened");
        Add(3, 400, "Case Hardened");

        // GLOCK-18 (4)
        Add(4, 38,  "Fade");
        Add(4, 353, "Water Elemental");
        Add(4, 380, "Wasteland Rebel");
        Add(4, 732, "Synth Leaf");
        Add(4, 437, "Twilight Galaxy");
        Add(4, 46,  "Neo-Noir");
        Add(4, 48,  "Sacrifice");
        Add(4, 57,  "Off World");
        Add(4, 52,  "Umbral Rabbit");
        Add(4, 58,  "Moonrise");

        // ═════════════════════════════════════════════════════════════════════
        //  RIFLES
        // ═════════════════════════════════════════════════════════════════════

        // AK-47 (7)
        Add(7, 180, "Fire Serpent");
        Add(7, 44,  "Case Hardened");
        Add(7, 675, "The Empress");
        Add(7, 551, "Asiimov");
        Add(7, 259, "Redline");
        Add(7, 8,   "Vulcan");
        Add(7, 11,  "Fire Serpent");
        Add(7, 524, "Fuel Injector");
        Add(7, 7,   "Redline");
        Add(7, 597, "Bloodsport");
        Add(7, 20,  "Bloodsport");
        Add(7, 22,  "Asiimov");
        Add(7, 19,  "Fuel Injector");
        Add(7, 490, "Frontside Misty");
        Add(7, 586, "Wasteland Rebel");
        Add(7, 506, "Point Disarray");
        Add(7, 13,  "Jaguar");
        Add(7, 16,  "Aquamarine Revenge");

        // AUG (8)
        Add(8, 33,  "Hot Rod");
        Add(8, 601, "Syd Mead");
        Add(8, 9,   "Bengal Tiger");
        Add(8, 541, "Fleet Flock");
        Add(8, 583, "Aristocrat");
        Add(8, 179, "Chameleon Plasti");

        // AWP (9)
        Add(9, 344, "Dragon Lore", true);
        Add(9, 756, "Gungnir", true);
        Add(9, 446, "Medusa", true);
        Add(9, 736, "The Prince");
        Add(9, 551, "Asiimov");
        Add(9, 316, "Dragon Lore", true);
        Add(9, 319, "Asiimov");
        Add(9, 662, "Oni Taiji");
        Add(9, 324, "Hyper Beast");
        Add(9, 653, "Neo-Noir");
        Add(9, 579, "Neo-Noir");
        Add(9, 580, "Wildfire");
        Add(9, 325, "The Prince");
        Add(9, 311, "Corticera");
        Add(9, 312, "Redline");
        Add(9, 1058, "POP AWP");

        // FAMAS (10)
        Add(10, 919, "Commemoration");
        Add(10, 923, "Eye of the Tiger");
        Add(10, 1092, "ZX Spectron");
        Add(10, 925, "Meow");
        Add(10, 924, "Cryo");
        Add(10, 920, "Drip");
        Add(10, 927, "Quickscope");
        Add(10, 902, "Aztec");

        // G3SG1 (11)
        Add(11, 511, "The Executioner");
        Add(11, 628, "Stinger");
        Add(11, 83,  "Orange DDPAT");
        Add(11, 606, "Ventilator");
        Add(11, 806, "Scavenger");

        // GALIL AR (13)
        Add(13, 428, "Eco");
        Add(13, 661, "Sugar Rush");
        Add(13, 1038, "Chromatic Aberration");
        Add(13, 647, "Crimson Tsunami");
        Add(13, 1071, "CAUTION!");
        Add(13, 433, "Chromatic Aberration");
        Add(13, 431, "Sugar Rush");
        Add(13, 435, "Phoenix Blacklight");
        Add(13, 447, "Crimson Tsunami");

        // M249 (14)
        Add(14, 401, "System Lock");
        Add(14, 547, "Spectre");
        Add(14, 452, "Shipping Forecast");
        Add(14, 496, "Nebula Crusader");
        Add(14, 266, "Magma");

        // M4A4 (16)
        Add(16, 309, "Howl", true);
        Add(16, 844, "The Emperor");
        Add(16, 653, "Neo-Noir");
        Add(16, 359, "Asiimov");
        Add(16, 588, "Desolate Space");
        Add(16, 155, "Bullet Rain");
        Add(16, 156, "Death by Kitty");
        Add(16, 166, "Global Offensive");
        Add(16, 160, "Desolate Space");
        Add(16, 161, "Hellfire");
        Add(16, 162, "Neo-Noir");
        Add(16, 1063, "The Coalition");
        Add(16, 189, "Bright Water");

        // MAC-10 (17)
        Add(17, 898, "Stalker");
        Add(17, 840, "Whitefish");
        Add(17, 707, "Neon Rider");
        Add(17, 947, "Disco Tech");
        Add(17, 665, "Aloha");
        Add(17, 433, "Neon Rider");
        Add(17, 434, "Stalker");

        // P90 (19)
        Add(19, 156, "Death by Kitty");
        Add(19, 182, "Emerald Dragon");
        Add(19, 551, "Asiimov");
        Add(19, 925, "Desert DDPAT");
        Add(19, 1233, "Neoqueen");
        Add(19, 906, "Buddy");
        Add(19, 80,  "Blind Spot");

        // MP5-SD (23)
        Add(23, 846, "Gauss");
        Add(23, 915, "Agent");
        Add(23, 810, "Phosphor");
        Add(23, 781, "Co-Processor");
        Add(23, 949, "Desert Strike");
        Add(23, 1018, "Panthera onca");

        // UMP-45 (24)
        Add(24, 556, "Primal Saber");
        Add(24, 752, "Fade");
        Add(24, 636, "Shallow Grave");
        Add(24, 990, "Gold Bismuth");
        Add(24, 703, "SWAG-7");
        Add(24, 80,  "Blind Spot");

        // XM1014 (25)
        Add(25, 654, "Seasons");
        Add(25, 689, "Ziggy");
        Add(25, 1103, "Watchdog");
        Add(25, 850, "Incinegator");
        Add(25, 760, "Frost Borre");
        Add(25, 857, "Neon");
        Add(25, 38,  "Blaze");

        // PP-Bizon (26)
        Add(26, 676, "High Roller");
        Add(26, 542, "Judgement of Anubis");
        Add(26, 508, "Fuel Rod");
        Add(26, 777, "Facility Draft");

        // MAG-7 (27)
        Add(27, 948, "Justice");
        Add(27, 703, "SWAG-7");
        Add(27, 754, "Rust Coat");
        Add(27, 961, "Monster Call");
        Add(27, 787, "Core Breach");
        Add(27, 785, "Mandrel");

        // Negev (28)
        Add(28, 483, "Loudmouth");
        Add(28, 783, "Bulkhead");
        Add(28, 950, "Prototype");
        Add(28, 1012, "Phoenix Stencil");
        Add(28, 610, "Dazzle");

        // Sawed-Off (29)
        Add(29, 720, "Devourer");
        Add(29, 638, "Wasteland Princess");
        Add(29, 1155, "Kiss♥Love");
        Add(29, 596, "Limelight");
        Add(29, 517, "Yorick");
        Add(29, 880, "Parched");

        // Tec-9 (30)
        Add(30, 524, "Fuel Injector");
        Add(30, 555, "Re-Entry");
        Add(30, 599, "Ice Cap");
        Add(30, 552, "Fubar");
        Add(30, 791, "Remote Control");
        Add(30, 816, "Fubar");
        Add(30, 837, "Angry Mob");

        // P2000 (32)
        Add(32, 591, "Imperial Dragon");
        Add(32, 389, "Fire Elemental");
        Add(32, 550, "Oceanic");
        Add(32, 391, "Wicked");
        Add(32, 71,  "Scorpion");
        Add(32, 327, "Chainmail");
        Add(32, 84,  "Ivory");

        // MP7 (33)
        Add(33, 597, "Bloodsport");
        Add(33, 1073, "Imminent Danger");
        Add(33, 612, "Powercore");
        Add(33, 250, "Full Stop");
        Add(33, 500, "Special Delivery");
        Add(33, 893, "Neon Ply");

        // MP9 (34)
        Add(34, 262, "Rose Iron");
        Add(34, 1037, "Food Chain");
        Add(34, 1094, "Mount Fuji");
        Add(34, 630, "Sand Scale");
        Add(34, 1134, "Starlight Protector");

        // Nova (35)
        Add(35, 537, "Hyper Beast");
        Add(35, 809, "Wood Fired");
        Add(35, 785, "Mandrel");
        Add(35, 716, "Toy Soldier");
        Add(35, 864, "Candy Apple");
        Add(35, 3,   "Candy Apple");

        // P250 (36)
        Add(36, 678, "See Ya Later");
        Add(36, 801, "Asiimov");
        Add(36, 749, "Vino Primo");
        Add(36, 102, "Whiteout");
        Add(36, 981, "Contaminant");

        // SCAR-20 (38)
        Add(38, 639, "Bloodsport");
        Add(38, 12,  "Crimson Web");
        Add(38, 719, "Powercore");
        Add(38, 954, "Enforcer");
        Add(38, 637, "Cyrex");
        Add(38, 313, "Bloodsport");
        Add(38, 315, "Enforcer");

        // SG 553 (39)
        Add(39, 750, "Integrale");
        Add(39, 1234, "Cyberforce");
        Add(39, 613, "Triarch");
        Add(39, 897, "Colony IV");
        Add(39, 815, "Danger Close");
        Add(39, 179, "Chameleon Plasti");

        // SSG 08 (40)
        Add(40, 222, "Blood in the Water");
        Add(40, 624, "Dragonfire", true);
        Add(40, 503, "Big Iron");
        Add(40, 670, "Death's Head");
        Add(40, 989, "Parallax");
        Add(40, 538, "Necropos");
        Add(40, 321, "Turbo Peek");
        Add(40, 310, "Blood in the Water");

        // ═════════════════════════════════════════════════════════════════════
        //  RIFLES (M4A1-S, USP-S, CZ75, Revolver)
        // ═════════════════════════════════════════════════════════════════════

        // M4A1-S (60)
        Add(60, 962, "Printstream");
        Add(60, 1017, "Blue Phosphor");
        Add(60, 1001, "Welcome to the Jungle");
        Add(60, 946, "Player Two");
        Add(60, 587, "Mecha Industries");
        Add(60, 921, "Gold Arabesque");
        Add(60, 53,  "Printstream");
        Add(60, 54,  "Guardian");
        Add(60, 41,  "Hyper Beast");
        Add(60, 44,  "Mecha Industries");
        Add(60, 49,  "Player Two");
        Add(60, 50,  "Welcome to the Jungle");
        Add(60, 48,  "Control");
        Add(60, 40,  "Icarus Fell");
        Add(60, 43,  "Golden Coil");
        Add(60, 42,  "Chantico's Fire");
        Add(60, 52,  "Emphorosaur-S");

        // USP-S (61)
        Add(61, 504, "Kill Confirmed");
        Add(61, 984, "Printstream");
        Add(61, 653, "Neo-Noir");
        Add(61, 1136, "Ticket to Hell");
        Add(61, 312, "Cyrex");
        Add(61, 53,  "Neo-Noir");
        Add(61, 51,  "Cyrex");
        Add(61, 45,  "Guardian");
        Add(61, 50,  "Lead Conduit");
        Add(61, 47,  "Overgrowth");

        // CZ75-Auto (63)
        Add(63, 643, "Xiangliu");
        Add(63, 976, "Vendetta");
        Add(63, 543, "Red Astor");
        Add(63, 453, "Emerald");
        Add(63, 297, "Tuxedo");

        // R8 Revolver (64)
        Add(64, 752, "Fade");
        Add(64, 1145, "Crazy 8");
        Add(64, 843, "Skull Crusher");
        Add(64, 892, "Memento");
        Add(64, 714, "Nightmare");
        Add(64, 924, "Desert Brush");

        // ═════════════════════════════════════════════════════════════════════
        //  KNIVES — popular finishes for all cosmetic knife types
        // ═════════════════════════════════════════════════════════════════════
        for (auto k : kAllKnifeDefs) {
            AddKnifePaints(static_cast<i32>(k));
        }

        // ═════════════════════════════════════════════════════════════════════
        //  GLOVES — paint kits per glove type
        // ═════════════════════════════════════════════════════════════════════

        // Bloodhound Gloves (5027)
        AddGlove(5027, 10009, "Leather");
        AddGlove(5027, 10010, "Spruce DDPAT");
        AddGlove(5027, 10066, "Lt. Commander");
        AddGlove(5027, 10070, "Snow Leopard");
        AddGlove(5027, 10071, "Queen Jaguar");
        AddGlove(5027, 10073, "Slingshot");
        AddGlove(5027, 10074, "Big Game");
        AddGlove(5027, 10077, "Finish Line");
        AddGlove(5027, 10078, "Smoke Out");

        // Driver Gloves (5030)
        AddGlove(5030, 10016, "Crimson Weave");
        AddGlove(5030, 10028, "Cool Mint");
        AddGlove(5030, 10035, "Foundation");
        AddGlove(5030, 10040, "Diamondback");
        AddGlove(5030, 10041, "King Snake");
        AddGlove(5030, 10042, "Imperial Plaid");
        AddGlove(5030, 10043, "Overtake");
        AddGlove(5030, 10044, "Racing Green");
        AddGlove(5030, 10045, "Amphibious");
        AddGlove(5030, 10046, "Bronze Morph");
        AddGlove(5030, 10051, "Transport");
        AddGlove(5030, 10053, "Cobalt Skulls");
        AddGlove(5030, 10088, "Unhinged");

        // Hand Wraps (5031)
        AddGlove(5031, 10035, "Foundation");
        AddGlove(5031, 10037, "Pandora's Box");
        AddGlove(5031, 10038, "Hedge Maze");
        AddGlove(5031, 10070, "Snow Leopard");
        AddGlove(5031, 10071, "Queen Jaguar");
        AddGlove(5031, 10074, "Big Game");
        AddGlove(5031, 10040, "Diamondback");
        AddGlove(5031, 10046, "Bronze Morph");
        AddGlove(5031, 10077, "Finish Line");
        AddGlove(5031, 10078, "Smoke Out");

        // Moto Gloves (5032)
        AddGlove(5032, 10009, "Leather");
        AddGlove(5032, 10010, "Spruce DDPAT");
        AddGlove(5032, 10016, "Crimson Weave");
        AddGlove(5032, 10028, "Cool Mint");
        AddGlove(5032, 10035, "Foundation");
        AddGlove(5032, 10041, "King Snake");
        AddGlove(5032, 10042, "Imperial Plaid");
        AddGlove(5032, 10043, "Overtake");
        AddGlove(5032, 10044, "Racing Green");
        AddGlove(5032, 10045, "Amphibious");
        AddGlove(5032, 10051, "Transport");
        AddGlove(5032, 10053, "Cobalt Skulls");
        AddGlove(5032, 10066, "Lt. Commander");
        AddGlove(5032, 10073, "Slingshot");
        AddGlove(5032, 10088, "Unhinged");

        // Specialist Gloves (5033)
        AddGlove(5033, 10009, "Leather");
        AddGlove(5033, 10037, "Pandora's Box");
        AddGlove(5033, 10038, "Hedge Maze");
        AddGlove(5033, 10066, "Lt. Commander");
        AddGlove(5033, 10070, "Snow Leopard");
        AddGlove(5033, 10071, "Queen Jaguar");
        AddGlove(5033, 10073, "Slingshot");
        AddGlove(5033, 10074, "Big Game");
        AddGlove(5033, 10077, "Finish Line");
        AddGlove(5033, 10078, "Smoke Out");

        // Sport Gloves (5034)
        AddGlove(5034, 10016, "Crimson Weave");
        AddGlove(5034, 10028, "Cool Mint");
        AddGlove(5034, 10035, "Foundation");
        AddGlove(5034, 10037, "Pandora's Box");
        AddGlove(5034, 10038, "Hedge Maze");
        AddGlove(5034, 10040, "Diamondback");
        AddGlove(5034, 10041, "King Snake");
        AddGlove(5034, 10042, "Imperial Plaid");
        AddGlove(5034, 10043, "Overtake");
        AddGlove(5034, 10044, "Racing Green");
        AddGlove(5034, 10045, "Amphibious");
        AddGlove(5034, 10046, "Bronze Morph");
        AddGlove(5034, 10051, "Transport");
        AddGlove(5034, 10053, "Cobalt Skulls");
        AddGlove(5034, 10088, "Unhinged");

        // Hydra Gloves (5035)
        AddGlove(5035, 10009, "Leather");
        AddGlove(5035, 10010, "Spruce DDPAT");
        AddGlove(5035, 10066, "Lt. Commander");
        AddGlove(5035, 10070, "Snow Leopard");
        AddGlove(5035, 10071, "Queen Jaguar");
        AddGlove(5035, 10073, "Slingshot");
        AddGlove(5035, 10074, "Big Game");
        AddGlove(5035, 10077, "Finish Line");
        AddGlove(5035, 10078, "Smoke Out");

        // Broken Fang Gloves (5036)
        AddGlove(5036, 10009, "Leather");
        AddGlove(5036, 10010, "Spruce DDPAT");
        AddGlove(5036, 10016, "Crimson Weave");
        AddGlove(5036, 10028, "Cool Mint");
        AddGlove(5036, 10037, "Pandora's Box");
        AddGlove(5036, 10038, "Hedge Maze");
        AddGlove(5036, 10066, "Lt. Commander");
        AddGlove(5036, 10071, "Queen Jaguar");
        AddGlove(5036, 10073, "Slingshot");
        AddGlove(5036, 10074, "Big Game");
        AddGlove(5036, 10077, "Finish Line");
        AddGlove(5036, 10078, "Smoke Out");
        AddGlove(5036, 10088, "Unhinged");
    }
};

// ── Skin Manager ──────────────────────────────────────────────────────────────
// Tracks which skins the user has selected per weapon, and triggers updates.

class SkinManager {
public:
    std::vector<SkinInfo_t>  active_skins;
    SkinInfo_t               default_skin;

    void SetSkin(const SkinInfo_t& skin) noexcept {
        for (auto& s : active_skins) {
            if (s.weaponType == skin.weaponType) {
                if (s.Paint == skin.Paint) return;
                s = skin;
                force_update_ = true;
                return;
            }
        }
        active_skins.push_back(skin);
        force_update_ = true;
    }

    SkinInfo_t GetSkin(WeaponsEnum weapon) const noexcept {
        for (auto& s : active_skins) {
            if (s.weaponType == weapon) return s;
        }
        return {};
    }

    SkinInfo_t GetGloveSkin(i32 glove_def) const noexcept {
        for (auto& s : active_skins) {
            if (s.Paint > 0 && s.weaponType == WeaponsEnum::None &&
                s.name.find("Glove") != std::string_view::npos) {
                return s;
            }
        }
        return {};
    }

    void SetGloveSkin(const SkinInfo_t& skin) noexcept {
        glove_paint_ = skin.Paint;
        force_update_ = true;
    }

    i32 GetGlovePaint() const noexcept { return glove_paint_; }

    // ── Knife selection ──────────────────────────────────────────────────
    void SetKnife(WeaponsEnum knife_def, i32 paint) noexcept {
        knife_def_   = knife_def;
        knife_paint_ = paint;
        force_update_ = true;
    }

    WeaponsEnum GetKnifeDef() const noexcept { return knife_def_; }
    i32 GetKnifePaint() const noexcept { return knife_paint_; }

    bool IsKnifeDef(i32 def) const noexcept {
        return def == static_cast<i32>(WeaponsEnum::CtKnife) ||
               def == static_cast<i32>(WeaponsEnum::Tknife) ||
               (def >= 500 && def <= 530);
    }

    i32 GetSkinIndex(
        const std::vector<SkinInfo_t>& available,
        const SkinInfo_t& selected) const noexcept
    {
        for (i32 i = 0; i < static_cast<i32>(available.size()); ++i) {
            if (available[i].Paint == selected.Paint &&
                available[i].weaponType == selected.weaponType) {
                return i;
            }
        }
        return 0;
    }

    bool NeedsUpdate() const noexcept { return force_update_; }
    void ClearUpdate() noexcept { force_update_ = false; }

private:
    bool force_update_ = false;
    i32  glove_paint_  = 0;
    WeaponsEnum knife_def_   = WeaponsEnum::None;
    i32         knife_paint_ = 0;
};
