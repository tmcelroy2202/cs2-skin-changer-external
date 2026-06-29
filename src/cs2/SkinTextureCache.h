#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <objbase.h>
#include <d3d11.h>
#include <wincodec.h>
#include "imgui.h"
#include "core/Core.h"
#include "WeaponTypes.h"
#include "Offsets.h"

class SkinTextureCache {
public:
    explicit SkinTextureCache(ID3D11Device* device) : device_(device) {
        std::filesystem::create_directories(CacheDir());
    }

    ~SkinTextureCache() { Clear(); }

    /// Called once after construction to load paint kit -> icon path mapping.
    void InitMapping() {
        auto path = ItemsGamePath();
        if (!std::filesystem::exists(path)) {
            Offsets::DownloadUrl(
                "https://raw.githubusercontent.com/SteamDatabase/"
                "GameTracking-CS2/master/game/csgo/pak01_dir/scripts/items/items_game.txt",
                path);
        }
        if (std::filesystem::exists(path))
            ParseItemsGame(path);
    }

    /// Get cached texture (nullptr if not loaded yet).
    ImTextureID Get(i32 paint_id) const {
        auto it = map_.find(paint_id);
        if (it != map_.end() && it->second.srv)
            return (ImTextureID)it->second.srv;
        return nullptr;
    }

    /// Request async texture load (queued for next ProcessPending call).
    void Request(i32 paint_id, WeaponsEnum weapon) {
        if (map_.contains(paint_id)) return;
        auto& e = map_[paint_id];
        if (e.pending) return;
        e.pending = true;
        e.weapon = weapon;
        pending_.push_back(paint_id);
    }

    /// Load one pending texture (call each frame until queue empty).
    void ProcessPending() {
        if (pending_.empty()) return;
        i32 pid = pending_.back();
        pending_.pop_back();

        auto& e = map_[pid];
        e.pending = false;
        e.srv = LoadTexture(pid, e.weapon);
    }

    void Clear() {
        for (auto& [id, e] : map_) {
            if (e.srv) { e.srv->Release(); e.srv = nullptr; }
        }
        map_.clear();
        names_.clear();
        pending_.clear();
    }

    bool IsLoaded(i32 paint_id) const {
        auto it = map_.find(paint_id);
        return it != map_.end() && it->second.srv != nullptr;
    }

private:
    struct Entry {
        ID3D11ShaderResourceView* srv = nullptr;
        WeaponsEnum weapon = WeaponsEnum{};
        bool pending = false;
    };

    ID3D11Device* device_;
    std::unordered_map<i32, Entry> map_;
    std::unordered_map<i32, std::string> names_;
    std::vector<i32> pending_;

    static std::filesystem::path CacheDir() {
        const char* appdata = getenv("APPDATA");
        if (!appdata) appdata = getenv("HOME");
        if (!appdata) appdata = ".";
        return std::filesystem::path(appdata) / "veneer" / "cache";
    }

    static std::filesystem::path ItemsGamePath() {
        const char* appdata = getenv("APPDATA");
        if (!appdata) appdata = getenv("HOME");
        if (!appdata) appdata = ".";
        return std::filesystem::path(appdata) / "veneer" / "items_game.txt";
    }

    static std::filesystem::path ImagePath(i32 paint_id) {
        return CacheDir() / (std::to_string(paint_id) + ".png");
    }

    // ── items_game.txt parser ──────────────────────────────────────────────

    void ParseItemsGame(const std::filesystem::path& path) {
        std::ifstream f(path);
        if (!f) return;

        std::string line;
        int brace = 0;
        bool in_paint_kits = false;
        i32 current_id = 0;

        auto trim = [](std::string_view s) -> std::string_view {
            auto start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return {};
            s.remove_prefix(start);
            auto end = s.find_last_not_of(" \t\r\n");
            if (end != std::string::npos) s = s.substr(0, end + 1);
            return s;
        };

        auto unquote = [](std::string_view s) -> std::string_view {
            auto q1 = s.find('"');
            if (q1 == std::string::npos) return s;
            auto q2 = s.find('"', q1 + 1);
            if (q2 == std::string::npos) return s.substr(q1 + 1);
            return s.substr(q1 + 1, q2 - q1 - 1);
        };

        while (std::getline(f, line)) {
            auto sv = trim(std::string_view(line));
            if (sv.empty()) continue;

            if (sv.front() == '{') { ++brace; continue; }
            if (sv.front() == '}') { --brace; if (brace < 0) brace = 0; continue; }

            if (!in_paint_kits) {
                if (sv.find("\"paint_kits\"") == 0) in_paint_kits = true;
                continue;
            }

            if (brace == 1) {
                auto key = unquote(sv);
                if (key.empty()) continue;
                auto [p, ec] = std::from_chars(key.data(), key.data() + key.size(), current_id);
                if (ec != std::errc{}) current_id = 0;
                continue;
            }

            if (brace == 2 && current_id > 0) {
                auto q1 = sv.find('"');
                if (q1 == std::string::npos) continue;
                auto q2 = sv.find('"', q1 + 1);
                if (q2 == std::string::npos) continue;
                auto key = sv.substr(q1 + 1, q2 - q1 - 1);

                if (key == "name") {
                    auto after = q2 + 1;
                    auto v1 = sv.find('"', after);
                    if (v1 == std::string::npos) continue;
                    auto v2 = sv.find('"', v1 + 1);
                    if (v2 == std::string::npos) continue;
                    names_[current_id] = std::string(sv.substr(v1 + 1, v2 - v1 - 1));
                }
            }
        }
    }

    // ── Image URL construction ─────────────────────────────────────────────

    static std::string_view WeaponToInternal(WeaponsEnum w) {
        switch (w) {
        case WeaponsEnum::Deagle:        return "weapon_deagle";
        case WeaponsEnum::Elite:         return "weapon_elite";
        case WeaponsEnum::FiveSeven:     return "weapon_fiveseven";
        case WeaponsEnum::Glock:         return "weapon_glock";
        case WeaponsEnum::Ak47:          return "weapon_ak47";
        case WeaponsEnum::Aug:           return "weapon_aug";
        case WeaponsEnum::Awp:           return "weapon_awp";
        case WeaponsEnum::Famas:         return "weapon_famas";
        case WeaponsEnum::G3Sg1:         return "weapon_g3sg1";
        case WeaponsEnum::Galil:         return "weapon_galilar";
        case WeaponsEnum::M249:          return "weapon_m249";
        case WeaponsEnum::M4A4:          return "weapon_m4a1";
        case WeaponsEnum::M4A1Silencer:  return "weapon_m4a1_silencer";
        case WeaponsEnum::Mac10:         return "weapon_mac10";
        case WeaponsEnum::P90:           return "weapon_p90";
        case WeaponsEnum::Mp5SD:         return "weapon_mp5sd";
        case WeaponsEnum::Ump45:         return "weapon_ump45";
        case WeaponsEnum::Xm1014:        return "weapon_xm1014";
        case WeaponsEnum::Bizon:         return "weapon_bizon";
        case WeaponsEnum::Mag7:          return "weapon_mag7";
        case WeaponsEnum::Negev:         return "weapon_negev";
        case WeaponsEnum::Sawedoof:      return "weapon_sawedoff";
        case WeaponsEnum::Tec9:          return "weapon_tec9";
        case WeaponsEnum::P200:          return "weapon_hkp2000";
        case WeaponsEnum::Mp7:           return "weapon_mp7";
        case WeaponsEnum::Mp9:           return "weapon_mp9";
        case WeaponsEnum::Nova:          return "weapon_nova";
        case WeaponsEnum::P250:          return "weapon_p250";
        case WeaponsEnum::Scar20:        return "weapon_scar20";
        case WeaponsEnum::Sg556:         return "weapon_sg556";
        case WeaponsEnum::Ssg08:         return "weapon_ssg08";
        case WeaponsEnum::UspS:          return "weapon_usp_silencer";
        case WeaponsEnum::Cz75A:         return "weapon_cz75a";
        case WeaponsEnum::Revolver:      return "weapon_revolver";
        default:                         return {};
        }
    }

    std::string MakeURL(i32 paint_id, WeaponsEnum weapon) {
        auto name_it = names_.find(paint_id);
        if (name_it == names_.end()) return {};

        auto wname = WeaponToInternal(weapon);
        if (wname.empty()) return {};

        return "https://steamcdn-a.akamaihd.net/apps/730/icons/econ/default_generated/"
             + std::string(wname) + "_" + name_it->second + "_light_large.256x256.png";
    }

    // ── Texture loading ────────────────────────────────────────────────────

    ID3D11ShaderResourceView* LoadTexture(i32 paint_id, WeaponsEnum weapon) {
        auto path = ImagePath(paint_id);

        if (!std::filesystem::exists(path)) {
            auto url = MakeURL(paint_id, weapon);
            if (url.empty()) return nullptr;
            Offsets::DownloadUrl(url.c_str(), path);
            if (!std::filesystem::exists(path)) return nullptr;
        }

        return CreateSRVFromFile(path);
    }

    ID3D11ShaderResourceView* CreateSRVFromFile(const std::filesystem::path& path) {
        IWICImagingFactory* factory = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) return nullptr;

        IWICBitmapDecoder* decoder = nullptr;
        hr = factory->CreateDecoderFromFilename(path.wstring().c_str(), nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) { factory->Release(); return nullptr; }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) { decoder->Release(); factory->Release(); return nullptr; }

        IWICFormatConverter* converter = nullptr;
        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr)) { frame->Release(); decoder->Release(); factory->Release(); return nullptr; }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); factory->Release(); return nullptr; }

        UINT width = 0, height = 0;
        converter->GetSize(&width, &height);

        std::vector<u8> pixels(width * height * 4);
        hr = converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());

        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();

        if (FAILED(hr)) return nullptr;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = pixels.data();
        init.SysMemPitch = width * 4;

        ID3D11Texture2D* tex = nullptr;
        hr = device_->CreateTexture2D(&desc, &init, &tex);
        if (FAILED(hr)) return nullptr;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = device_->CreateShaderResourceView(tex, nullptr, &srv);
        tex->Release();
        if (FAILED(hr)) return nullptr;

        return srv;
    }
};
