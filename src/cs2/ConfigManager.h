#pragma once

#include "core/Core.h"
#include "SkinDatabase.h"
#include <fstream>
#include <sstream>
#include <filesystem>

/* ── ConfigManager ──────────────────────────────────────────────────────────
 *
 * Saves/loads the skin-changer loadout to a text file in %APPDATA%.
 * File is cloud-sync ready (Dropbox, OneDrive, etc).
 * ──────────────────────────────────────────────────────────────────────────── */

inline std::filesystem::path ConfigPath() {
    auto path = std::filesystem::path(getenv("APPDATA")) / "cs2-skin-changer";
    std::filesystem::create_directories(path);
    return path / "loadout.txt";
}

inline void SaveConfig(SkinManager& mgr, u16 music_kit_id) {
    auto path = ConfigPath();
    std::ofstream f(path);
    if (!f) return;

    f << "# CS2 Skin Changer loadout\n";
    f << "# Format: weapon_def_id = paint_id, wear, pattern\n\n";

    // Per-weapon skins
    for (auto& s : mgr.active_skins) {
        i32 def = static_cast<i32>(s.weaponType);
        if (def > 0 && s.Paint > 0) {
            f << def << " = " << s.Paint << ", " << s.Wear << ", " << s.Pattern << "\n";
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
    if (gpnt > 0) {
        f << "glove = " << gpnt << "\n";
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
            int gpnt;
            if (ss >> gpnt) {
                SkinInfo_t glove_skin{gpnt, 0.01f, 0, false, "Glove", WeaponsEnum::None};
                mgr.SetGloveSkin(glove_skin);
            }
        } else if (key == "music_kit") {
            int mid;
            if (ss >> mid) {
                music_kit_id = static_cast<u16>(mid);
            }
        } else {
            // Per-weapon: weapon_def = paint, wear, pattern
            int def = std::atoi(key.c_str());
            if (def <= 0) continue;

            int paint;
            float wear;
            int pattern;
            char c1, c2;
            if (ss >> paint >> c1 >> wear >> c2 >> pattern) {
                SkinInfo_t skin{paint, wear, pattern, false, "", static_cast<WeaponsEnum>(def)};
                mgr.SetSkin(skin);
            }
        }
    }
}
