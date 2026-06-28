#pragma once

#include "Core.h"
#include "Module.h"
#include "PatternScanner.h"
#include <unordered_map>

/* ── OffsetManager ──────────────────────────────────────────────────────────
 *
 * Central registry for offsets and signatures used throughout the framework.
 * All offsets are resolved lazily on first access and cached thereafter.
 *
 * Signatures are identified by a string key and can be loaded from an embedded
 * table or from a configuration file at runtime.  This decouples the engine
 * version-specific data from the analysis logic.
 *
 * Typical use:
 *   OffsetManager::Add("entity_list", { .pattern = "...", .mask = "..." });
 *   u64 entity_list = OffsetManager::Resolve("entity_list");
 * ──────────────────────────────────────────────────────────────────────────── */

struct OffsetEntry {
    Signature   signature;
    std::string module_name;       // empty = use default module
};

class OffsetManager {
public:
    /// Register a new offset/signature entry (thread-safe after init).
    static void             Add(std::string_view key, OffsetEntry entry);

    /// Resolve a registered offset by scanning and caching the result.
    static std::optional<u64> Resolve(std::string_view key);

    /// Direct lookup of a cached offset (no scan).
    static std::optional<u64> Get(std::string_view key);

    /// Set the default module used for resolution.
    static void SetDefaultModule(const Module& mod) noexcept;

    /// Reset all cached resolutions (e.g., after map load).
    static void ClearCache() noexcept;

private:
    using Map = std::unordered_map<std::string, OffsetEntry,
                                   std::hash<std::string>,
                                   std::equal_to<>>;

    static Map&     Entries() noexcept;
    static std::unordered_map<std::string, u64, std::hash<std::string>,
                              std::equal_to<>>& Cache() noexcept;
    static Module&  DefaultModule() noexcept;
    static bool&    ModuleSet() noexcept;
};
