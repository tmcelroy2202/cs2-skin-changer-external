#pragma once

#include "core/Core.h"
#include "engine/resource/ResourceSystem.h"
#include "engine/render/MaterialSystem.h"

/* ── AssetAnalyzer ──────────────────────────────────────────────────────────
 *
 * Analysis tool for examining game assets — materials, textures, models —
 * and their memory footprints, reference chains, and load patterns.
 *
 * Provides:
 *   - Asset dependency tracking
 *   - Memory usage breakdown by asset type / category
 *   - Duplicate asset detection (same resource loaded multiple times)
 *   - Load order and timing analysis (requires event hooking)
 *   - Asset metadata extraction
 * ──────────────────────────────────────────────────────────────────────────── */

struct AssetBreakdown {
    u64     total_materials{};
    u64     total_textures{};
    u64     total_models{};
    u64     total_sounds{};
    u64     material_memory{};
    u64     texture_memory{};
    u64     model_memory{};
    u64     sound_memory{};
    u64     total_memory{};
};

struct AssetEntry {
    std::string name;
    std::string type;          // "material", "texture", "model", "sound"
    u64         memory_bytes{};
    std::vector<std::string> references; // assets referenced by this one
};

class AssetAnalyzer {
public:
    explicit AssetAnalyzer(ResourceSystem* resources,
                           MaterialSystem* materials) noexcept
        : resources_(resources), materials_(materials) {}

    /// Run a full asset analysis and return breakdown.
    [[nodiscard]] AssetBreakdown Analyze() noexcept;

    /// Enumerate all assets with metadata.
    [[nodiscard]] std::vector<AssetEntry> EnumerateAssets() noexcept;

    /// Find assets whose name contains the given substring.
    [[nodiscard]] std::vector<AssetEntry> SearchAssets(
        std::string_view query) noexcept;

    /// Get the top-N largest assets by memory usage.
    [[nodiscard]] std::vector<AssetEntry> GetLargestAssets(
        usize count = 20) noexcept;

    /// Estimate total resident asset memory.
    [[nodiscard]] u64 EstimateTotalMemory() noexcept;

private:
    ResourceSystem* resources_;
    MaterialSystem* materials_;
};
