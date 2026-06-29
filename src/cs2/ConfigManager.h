#pragma once

#include "core/Core.h"
#include "SkinDatabase.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <charconv>

/* ── ConfigManager ──────────────────────────────────────────────────────────
 *
 * Saves/loads the skin-changer loadout to a text file in %APPDATA%.
 * File is cloud-sync ready (Dropbox, OneDrive, etc).
 * ──────────────────────────────────────────────────────────────────────────── */

inline std::filesystem::path ConfigPath() {
    const char* appdata = getenv("APPDATA");
    if (!appdata) appdata = getenv("HOME");
    if (!appdata) return "loadout.txt";
    auto path = std::filesystem::path(appdata) / "veneer";
    std::filesystem::create_directories(path);
    return path / "loadout.txt";
}

inline void SaveConfig(SkinManager& mgr, u16 music_kit_id) {
    auto path = ConfigPath();
    std::ofstream f(path);
    if (!f) return;

    f << "# VeneeR loadout\n";
    f << "# Format: weapon_def_id = paint_id, wear, pattern, name, old_model(0/1)\n\n";

    // Per-weapon skins
    for (auto& s : mgr.active_skins) {
        i32 def = static_cast<i32>(s.weaponType);
        if (def > 0 && s.Paint > 0) {
            f << def << " = " << s.Paint << ", " << s.Wear << ", " << s.Pattern
              << ", " << s.name << ", " << (s.bUsesOldModel ? 1 : 0) << "\n";
        }
    }

    f << "\n";

    // Knife
    i32 kdef = static_cast<i32>(mgr.GetKnifeDef());
    i32 kpnt = mgr.GetKnifePaint();
    if (kpnt > 0) {
        f << "knife = " << kdef << ", " << kpnt << "\n";
    }

    // Gloves
    i32 gpnt = mgr.GetGlovePaint();
    i32 gdef = static_cast<i32>(mgr.GetGloveDef());
    if (gpnt > 0) {
        f << "glove = " << gdef << ", " << gpnt << "\n";
    }

    // Music kit
    if (music_kit_id > 0) {
        f << "music_kit = " << static_cast<i32>(music_kit_id) << "\n";
    }
}

inline void LoadConfig(SkinManager& mgr, u16& music_kit_id) {
    auto path = ConfigPath();
    std::ifstream f(path);
    if (!f) return;

    std::string line;
    while (std::getline(f, line)) {
        // Strip comments and whitespace
        auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string key;
        char eq;
        ss >> key >> eq;
        if (eq != '=' || key.empty()) continue;

        if (key == "knife") {
            int kdef, kpnt;
            char comma;
            if (ss >> kdef >> comma >> kpnt) {
                mgr.SetKnife(static_cast<WeaponsEnum>(kdef), kpnt);
            }
        } else if (key == "glove") {
            int gdef, gpnt;
            char comma;
            if (ss >> gdef >> comma >> gpnt) {
                SkinInfo_t glove_skin{gpnt, 0.01f, 0, false, "Glove", WeaponsEnum::None};
                mgr.SetGloveSkin(glove_skin, static_cast<GlovesEnum>(gdef));
            }
        } else if (key == "music_kit") {
            int mid;
            if (ss >> mid) {
                music_kit_id = static_cast<u16>(mid);
            }
        } else {
            // Per-weapon: weapon_def = paint, wear, pattern, name, old_model
            int def = 0;
            auto [p, ec] = std::from_chars(key.data(), key.data() + key.size(), def);
            if (ec != std::errc{} || def <= 0) continue;

            int paint;
            float wear;
            int pattern;
            char c1, c2, c3;
            std::string name;
            int old_model = 0;
            if (ss >> paint >> c1 >> wear >> c2 >> pattern) {
                if (ss >> c3) {
                    std::getline(ss, name, ',');
                    if (!name.empty() && name.front() == ' ') name.erase(0, 1);
                    ss >> old_model;
                }
                SkinInfo_t skin{paint, wear, pattern, old_model != 0, name, static_cast<WeaponsEnum>(def)};
                mgr.SetSkin(skin);
            }
        }
    }
}
