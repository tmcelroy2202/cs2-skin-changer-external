#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <set>
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
            // Try local CS2 installation first (much faster, always available)
            char cs2_path[MAX_PATH]{};
            HMODULE cs2 = GetModuleHandleA("client.dll");
            if (cs2) {
                GetModuleFileNameA(cs2, cs2_path, MAX_PATH);
                auto p = std::filesystem::path(cs2_path).parent_path();
                // Navigate up to CS2 root: game/bin/win64 -> game/csgo
                auto items_game = p.parent_path().parent_path().parent_path()
                                / "csgo" / "scripts" / "items" / "items_game.txt";
                if (std::filesystem::exists(items_game)) {
                    std::filesystem::copy_file(items_game, path,
                        std::filesystem::copy_options::overwrite_existing);
                }
            }
            // Fallback: download from GitHub with retry
            if (!std::filesystem::exists(path)) {
                for (int attempt = 0; attempt < 3; ++attempt) {
                    Offsets::DownloadUrl(
                        "https://raw.githubusercontent.com/SteamDatabase/"
                        "GameTracking-CS2/master/game/csgo/pak01_dir/scripts/items/items_game.txt",
                        path);
                    if (std::filesystem::exists(path) &&
                        std::filesystem::file_size(path) > 1000)
                        break;
                    if (std::filesystem::exists(path))
                        std::filesystem::remove(path);
                    Sleep(2000 * (1 << attempt));
                }
            }
        }
        if (std::filesystem::exists(path))
            ParseItemsGame(path);
    }

    /// Get cached texture (nullptr if not loaded yet).
    ImTextureID Get(i32 paint_id) const {
        auto it = map_.find(paint_id);
        if (it != map_.end() && it->second.srv)
            return (ImTextureID)it->second.srv;
        return ImTextureID(0);
    }

    /// Request async texture load (queued for next ProcessPending call).
    /// If not cached on disk, spawns a background download thread.
    /// Retries failed loads on subsequent Request calls.
    void Request(i32 paint_id, WeaponsEnum weapon) {
        auto it = map_.find(paint_id);
        if (it != map_.end()) {
            if (it->second.srv) return;         // already loaded
            if (it->second.pending) return;     // already queued for load
            if (downloading_.count(paint_id)) return; // already downloading
            // Not cached, not pending, not downloading — start download
            if (!std::filesystem::exists(ImagePath(paint_id))) {
                downloading_.insert(paint_id);
                StartDownload(paint_id, weapon);
                return;
            }
            // File is now cached — queue for SRV creation on render thread
            it->second.pending = true;
            it->second.weapon = weapon;
            pending_.push_back(paint_id);
            return;
        }
        // First request
        if (std::filesystem::exists(ImagePath(paint_id))) {
            map_[paint_id] = Entry{nullptr, weapon, true};
            pending_.push_back(paint_id);
        } else {
            map_[paint_id] = Entry{nullptr, weapon, false};
            if (!downloading_.count(paint_id)) {
                downloading_.insert(paint_id);
                StartDownload(paint_id, weapon);
            }
        }
    }

    /// Load pending textures that have been downloaded to disk.
    /// Call each frame until queue empty.
    void ProcessPending() {
        // Pick up newly completed downloads
        std::vector<i32> batch;
        {
            std::lock_guard lock(download_state_->mutex);
            batch.swap(download_state_->completed);
        }
        for (i32 pid : batch) {
            downloading_.erase(pid);
            auto it = map_.find(pid);
            if (it != map_.end() && !it->second.srv) {
                it->second.pending = true;
                pending_.push_back(pid);
            }
        }

        // Process up to 4 pending items per frame to load textures faster
        int processed = 0;
        while (!pending_.empty() && processed < 4) {
            i32 pid = pending_.back();
            pending_.pop_back();

            auto it = map_.find(pid);
            if (it == map_.end()) continue;
            auto& e = it->second;
            e.pending = false;

            if (std::filesystem::exists(ImagePath(pid))) {
                e.srv = LoadTexture(pid, e.weapon);
            }
            ++processed;
        }
    }

    void Clear() {
        for (auto& [id, e] : map_) {
            if (e.srv) { e.srv->Release(); e.srv = nullptr; }
        }
        map_.clear();
        names_.clear();
        pending_.clear();
        downloading_.clear();
        download_state_->stopped = true;
        download_state_ = std::make_shared<DownloadState>();
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
    std::unordered_map<i32, std::string> names_;            // paint_id → internal name (from items_game)
    std::vector<i32> pending_;
    std::set<i32> downloading_;          // paint_ids currently being downloaded (render thread only)

    struct DownloadState {
        std::mutex mutex;
        std::vector<i32> completed;
        std::atomic<bool> stopped{false};
    };
    std::shared_ptr<DownloadState> download_state_ = std::make_shared<DownloadState>();

    static std::filesystem::path CacheDir() {
        auto path = BasePath() / "cache";
        std::filesystem::create_directories(path);
        return path;
    }

    static std::filesystem::path ItemsGamePath() {
        return BasePath() / "items_game.txt";
    }

    static std::filesystem::path ImagePath(i32 paint_id) {
        return CacheDir() / (std::to_string(paint_id) + ".png");
    }

    /// Build a display name hint from the internal paint name.
    /// E.g. "cu_fireserpent_ak47_bravo" → "Fire Serpent"
    static std::string PaintInternalToDisplay(std::string_view in) {
        // Strip known prefixes
        std::string s(in);
        static const char* prefixes[] = {
            "cu_", "am_", "sp_", "hy_", "gs_", "aq_", "so_", "tt_", "ua_"
        };
        for (auto pfx : prefixes) {
            if (s.find(pfx) == 0) {
                s = s.substr(3);
                break;
            }
        }
        // Strip weapon suffix after last underscore (e.g. "_ak47", "_m4a1")
        auto last_us = s.rfind('_');
        if (last_us != std::string::npos) {
            auto suffix = s.substr(last_us + 1);
            static const char* weapon_suffixes[] = {
                "ak47", "awp", "m4a1", "m4a4", "deagle", "glock", "usp",
                "p250", "famas", "galil", "aug", "sg556", "p90", "mp7",
                "mp9", "mac10", "mp5sd", "bizon", "ump45", "nova",
                "xm1014", "mag7", "sawedoff", "m249", "negev", "tec9",
                "fiveseven", "elite", "cz75a", "revolver", "hkp2000",
                "scar20", "g3sg1", "ssg08", "usp_silencer", "bravo"
            };
            for (auto ws : weapon_suffixes) {
                if (suffix == ws) {
                    s = s.substr(0, last_us);
                    break;
                }
            }
        }
        // Replace underscores with spaces and title-case
        for (auto& c : s) {
            if (c == '_') c = ' ';
        }
        if (!s.empty()) s[0] = (char)std::toupper((unsigned char)s[0]);
        for (size_t i = 1; i < s.size(); ++i) {
            if (s[i-1] == ' ' && s[i] != ' ')
                s[i] = (char)std::toupper((unsigned char)s[i]);
        }
        return s;
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
        case WeaponsEnum::M4A4:          return "weapon_m4a4";
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
        if (wname.empty()) {
            // For gloves/knives with no weapon mapping, try CS2 economy image API
            // using just the paint internal name (Steam market search)
            return {};
        }

        // Primary: CS:GO CDN format (still works for most CS2 skins)
        return "https://steamcdn-a.akamaihd.net/apps/730/icons/econ/default_generated/"
             + std::string(wname) + "_" + name_it->second + "_light_large.256x256.png";
    }

    // Build a Steam market search URL as fallback (doesn't need weapon type)
    std::string MakeSearchURL(i32 paint_id) {
        auto name_it = names_.find(paint_id);
        if (name_it == names_.end()) return {};

        std::string display = PaintInternalToDisplay(name_it->second);
        std::string encoded;
        for (char c : display) {
            if (c == ' ') encoded += '+';
            else encoded += c;
        }
        return "https://steamcommunity.com/market/search/render/?query=+"
               + encoded + "+&appid=730&start=0&count=1&norender=1";
    }

    // ── Async download ─────────────────────────────────────────────────────

    void StartDownload(i32 paint_id, WeaponsEnum weapon) {
        auto path = ImagePath(paint_id);
        auto state = download_state_;

        // Look up data on the caller thread (safe) for the async worker
        std::string wname_str;
        std::string paint_internal;
        {
            auto wname = WeaponToInternal(weapon);
            if (!wname.empty()) wname_str = std::string(wname);
            auto name_it = names_.find(paint_id);
            if (name_it != names_.end()) paint_internal = name_it->second;
        }

        std::thread t([paint_id, path, state, wname_str, paint_internal]() {
            // 1) Try CS:GO CDN URL first (works for most CS2 skins too)
            if (!wname_str.empty() && !paint_internal.empty()) {
                std::string url = "https://steamcdn-a.akamaihd.net/apps/730/icons/econ/default_generated/"
                                + wname_str + "_" + paint_internal + "_light_large.256x256.png";
                Offsets::DownloadUrl(url.c_str(), path);
                if (std::filesystem::exists(path) && std::filesystem::file_size(path) > 1000) {
                    if (!state->stopped) {
                        std::lock_guard lock(state->mutex);
                        state->completed.push_back(paint_id);
                    }
                    return;
                }
                std::filesystem::remove(path);
            }

            // 2) Try CS2 economy image — direct paint_id URL
            if (!paint_internal.empty()) {
                // Try the CS2-format economy image URL
                std::string cs2_url = "https://community.cloudflare.steamstatic.com/economy/image/"
                                    + paint_internal + "/256x196.jpg";
                Offsets::DownloadUrl(cs2_url.c_str(), path);
                if (std::filesystem::exists(path) && std::filesystem::file_size(path) > 1000) {
                    if (!state->stopped) {
                        std::lock_guard lock(state->mutex);
                        state->completed.push_back(paint_id);
                    }
                    return;
                }
                std::filesystem::remove(path);
            }

            // 3) Try economy image API — fetch hash per-item from Steam market
            if (!paint_internal.empty()) {
                std::string display = PaintInternalToDisplay(paint_internal);
                std::string encoded;
                for (char c : display) {
                    if (c == ' ') encoded += '+';
                    else encoded += c;
                }
                std::string search_url = "https://steamcommunity.com/market/search/render/?query=+"
                                       + encoded + "+&appid=730&start=0&count=1&norender=1";

                auto tmp = std::filesystem::path(path).parent_path() / "_search.json";
                Offsets::DownloadUrl(search_url.c_str(), tmp);
                if (std::filesystem::exists(tmp)) {
                    std::ifstream f(tmp);
                    std::string json((std::istreambuf_iterator<char>(f)),
                                      std::istreambuf_iterator<char>());
                    f.close();
                    std::filesystem::remove(tmp);

                    auto pos = json.find("\"icon_url\":\"");
                    if (pos != std::string::npos) {
                        pos += 12;
                        auto end = json.find('"', pos);
                        if (end != std::string::npos) {
                            std::string hash = json.substr(pos, end - pos);
                            if (!hash.empty()) {
                                std::string url = "https://community.cloudflare.steamstatic.com/economy/image/"
                                                + hash + "/256x196.jpg";
                                Offsets::DownloadUrl(url.c_str(), path);
                                if (std::filesystem::exists(path) && std::filesystem::file_size(path) > 1000) {
                                    if (!state->stopped) {
                                        std::lock_guard lock(state->mutex);
                                        state->completed.push_back(paint_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        });
        t.detach();
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
        // WIC requires COM on this thread. Safe to call even if already init'd
        // (returns S_FALSE, needs matching CoUninitialize).
        HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        auto comCleanup = [coInit] { if (coInit == S_OK || coInit == S_FALSE) CoUninitialize(); };

        IWICImagingFactory* factory = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) { comCleanup(); return nullptr; }

        IWICBitmapDecoder* decoder = nullptr;
        hr = factory->CreateDecoderFromFilename(path.wstring().c_str(), nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) { factory->Release(); comCleanup(); return nullptr; }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) { decoder->Release(); factory->Release(); comCleanup(); return nullptr; }

        IWICFormatConverter* converter = nullptr;
        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr)) { frame->Release(); decoder->Release(); factory->Release(); comCleanup(); return nullptr; }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); factory->Release(); comCleanup(); return nullptr; }

        UINT width = 0, height = 0;
        converter->GetSize(&width, &height);

        std::vector<u8> pixels(static_cast<size_t>(width) * height * 4);
        hr = converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());

        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        comCleanup();

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
