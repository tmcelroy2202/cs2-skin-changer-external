#pragma once

#include "core/Core.h"
#include "SkinDatabase.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <charconv>
#include <cstdlib>
#include <vector>

/* ── ConfigManager ──────────────────────────────────────────────────────────
 *
 * Saves/loads the skin-changer loadout to a text file alongside the DLL.
 * ──────────────────────────────────────────────────────────────────────────── */

inline std::filesystem::path ConfigPath() {
    auto path = BasePath();
    std::filesystem::create_directories(path);
    return path / "loadout.txt";
}

inline void SaveConfig(SkinManager& mgr, u16 music_kit_id) {
    auto path = ConfigPath();
    std::ofstream f(path);
    if (!f) return;

    f << "# loadout\n";
    f << "# Format: weapon_def_id = paint_id, wear, pattern, name, old_model(0/1)\n\n";

    // Per-weapon skins (format: def = paint, wear, pattern, name, old_model[, statTrak])
    for (auto& s : mgr.active_skins) {
        i32 def = static_cast<i32>(s.weaponType);
        if (def > 0 && s.Paint > 0) {
            std::string esc_name = s.name;
            for (size_t p = 0; (p = esc_name.find('\\', p)) != std::string::npos; p += 2)
                esc_name.replace(p, 1, "\\\\");
            for (size_t p = 0; (p = esc_name.find(',', p)) != std::string::npos; p += 2)
                esc_name.replace(p, 1, "\\,");
            f << def << " = " << s.Paint << ", " << s.Wear << ", " << s.Pattern
              << ", " << esc_name << ", " << (s.bUsesOldModel ? 1 : 0)
              << ", " << s.StatTrak << "\n";
        }
    }

    f << "\n";

    // Knife — CT and T
    {
        i32 kdef = static_cast<i32>(mgr.GetKnifeDef(true));
        i32 kpnt = mgr.GetKnifePaint(true);
        f32 kwear = mgr.GetKnifeWear(true);
        i32 kpattern = mgr.GetKnifePattern(true);
        if (kpnt > 0)
            f << "knife_ct = " << kdef << ", " << kpnt << ", " << kwear << ", " << kpattern << "\n";
    }
    {
        i32 kdef = static_cast<i32>(mgr.GetKnifeDef(false));
        i32 kpnt = mgr.GetKnifePaint(false);
        f32 kwear = mgr.GetKnifeWear(false);
        i32 kpattern = mgr.GetKnifePattern(false);
        if (kpnt > 0)
            f << "knife = " << kdef << ", " << kpnt << ", " << kwear << ", " << kpattern << "\n";
    }

    // Gloves — CT and T
    {
        i32 gpnt = mgr.GetGlovePaint(true);
        i32 gdef = static_cast<i32>(mgr.GetGloveDef(true));
        if (gpnt > 0)
            f << "glove_ct = " << gdef << ", " << gpnt << "\n";
    }
    {
        i32 gpnt = mgr.GetGlovePaint(false);
        i32 gdef = static_cast<i32>(mgr.GetGloveDef(false));
        if (gpnt > 0)
            f << "glove = " << gdef << ", " << gpnt << "\n";
    }

    // Music kit
    if (music_kit_id > 0) {
        f << "music_kit = " << static_cast<i32>(music_kit_id) << "\n";
    }
}

inline void LoadConfig(SkinManager& mgr, u16& music_kit_id) {
    mgr.ResetAll();
    music_kit_id = 0;

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
            float kwear = 0.01f;
            int kpattern = 0;
            if (ss >> kdef >> comma >> kpnt) {
                // Optional wear and pattern (backward-compatible)
                char comma2;
                if (ss >> comma2 >> kwear) {
                    char comma3;
                    if (ss >> comma3 >> kpattern) { /* ok */ }
                }
                mgr.SetTeamKnife(static_cast<WeaponsEnum>(kdef), kpnt, false, kwear, kpattern);
            }
        } else if (key == "knife_ct") {
            int kdef, kpnt;
            char comma;
            float kwear = 0.01f;
            int kpattern = 0;
            if (ss >> kdef >> comma >> kpnt) {
                char comma2;
                if (ss >> comma2 >> kwear) {
                    char comma3;
                    if (ss >> comma3 >> kpattern) { /* ok */ }
                }
                mgr.SetTeamKnife(static_cast<WeaponsEnum>(kdef), kpnt, true, kwear, kpattern);
            }
        } else if (key == "glove") {
            int gdef, gpnt;
            char comma;
            if (ss >> gdef >> comma >> gpnt) {
                SkinInfo_t glove_skin{gpnt, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum::None};
                mgr.SetTeamGloveSkin(glove_skin, static_cast<GlovesEnum>(gdef), false);
            }
        } else if (key == "glove_ct") {
            int gdef, gpnt;
            char comma;
            if (ss >> gdef >> comma >> gpnt) {
                SkinInfo_t glove_skin{gpnt, 0.01f, 0, -1, "", false, "Glove", WeaponsEnum::None};
                mgr.SetTeamGloveSkin(glove_skin, static_cast<GlovesEnum>(gdef), true);
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
            char c1, c2;
            std::string rest;
            if (ss >> paint >> c1 >> wear >> c2 >> pattern) {
                // Parse trailing name, old_model, statTrak (if present)
                std::getline(ss, rest);
                auto trim_front = [](std::string& s) {
                    while (!s.empty() && s.front() == ' ') s.erase(0, 1);
                };
                trim_front(rest);

                std::string name;
                i32 old_model = 0;
                i32 stat_trak = -1;

                // Split rest by commas (respecting escaped commas)
                std::vector<std::string> parts;
                {
                    std::string cur;
                    bool esc = false;
                    for (char c : rest) {
                        if (esc) {
                            if (c == ',') cur += ',';
                            else if (c == '\\') cur += '\\';
                            else { cur += '\\'; cur += c; }
                            esc = false;
                        } else if (c == '\\') {
                            esc = true;
                        } else if (c == ',') {
                            trim_front(cur);
                            if (!cur.empty()) parts.push_back(cur);
                            cur.clear();
                        } else {
                            cur += c;
                        }
                    }
                    trim_front(cur);
                    if (!cur.empty()) parts.push_back(cur);
                }

                if (parts.size() >= 1) name = parts[0];
                if (parts.size() >= 2) old_model = std::atoi(parts[1].c_str());
                if (parts.size() >= 3) stat_trak = std::atoi(parts[2].c_str());

                SkinInfo_t skin{paint, wear, pattern, stat_trak, "",
                                old_model != 0, name, static_cast<WeaponsEnum>(def)};
                mgr.SetSkin(skin);
            }
        }
    }
}
