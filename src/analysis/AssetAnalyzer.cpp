#include "AssetAnalyzer.h"
#include <algorithm>

AssetBreakdown AssetAnalyzer::Analyze() noexcept {
    AssetBreakdown breakdown{};

    if (materials_ && *materials_) {
        auto mats = materials_->GetAllMaterials();
        breakdown.total_materials = mats.size();
        for (auto& m : mats) {
            breakdown.material_memory += m.memory_size;
        }
    }

    if (resources_ && *resources_) {
        auto textures = resources_->GetTextures();
        breakdown.total_textures = textures.size();
        for (auto& t : textures) {
            breakdown.texture_memory += t.memory_size;
        }

        breakdown.total_memory = resources_->GetTotalMemoryUsage();
    }

    breakdown.total_memory = breakdown.material_memory + breakdown.texture_memory;
    return breakdown;
}

std::vector<AssetEntry> AssetAnalyzer::EnumerateAssets() noexcept {
    std::vector<AssetEntry> assets;

    if (materials_ && *materials_) {
        for (auto& m : materials_->GetAllMaterials()) {
            AssetEntry entry;
            entry.name = m.name;
            entry.type = "material";
            entry.memory_bytes = m.memory_size;
            assets.push_back(std::move(entry));
        }
    }

    if (resources_ && *resources_) {
        for (auto& t : resources_->GetTextures()) {
            AssetEntry entry;
            entry.name = t.name;
            entry.type = "texture";
            entry.memory_bytes = t.memory_size;
            assets.push_back(std::move(entry));
        }
    }

    return assets;
}

std::vector<AssetEntry>
AssetAnalyzer::SearchAssets(std::string_view query) noexcept {
    auto all = EnumerateAssets();
    std::vector<AssetEntry> result;

    auto query_lower = std::string(query);
    std::ranges::transform(query_lower, query_lower.begin(),
                           [](char c) { return char(std::tolower(c)); });

    for (auto& entry : all) {
        auto name_lower = entry.name;
        std::ranges::transform(name_lower, name_lower.begin(),
                               [](char c) { return char(std::tolower(c)); });
        if (name_lower.find(query_lower) != std::string_view::npos) {
            result.push_back(std::move(entry));
        }
    }

    return result;
}

std::vector<AssetEntry>
AssetAnalyzer::GetLargestAssets(usize count) noexcept {
    auto all = EnumerateAssets();

    std::ranges::sort(all, std::greater<>(), &AssetEntry::memory_bytes);

    if (all.size() > count) all.resize(count);
    return all;
}

u64 AssetAnalyzer::EstimateTotalMemory() noexcept {
    if (resources_ && *resources_) {
        return resources_->GetTotalMemoryUsage();
    }
    return 0;
}
