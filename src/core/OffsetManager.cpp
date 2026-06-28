#include "OffsetManager.h"
#include "Logging.h"

#include <mutex>

void OffsetManager::Add(std::string_view key, OffsetEntry entry) {
    Entries().emplace(key, std::move(entry));
}

std::optional<u64> OffsetManager::Resolve(std::string_view key) {
    // Check cache first
    if (auto cached = Get(key)) return cached;

    auto& entries = Entries();
    auto it = entries.find(key);
    if (it == entries.end()) {
        LOG_WARN("Offset '{}' not registered", key);
        return std::nullopt;
    }

    // Determine which module to scan
    Module mod;
    if (!it->second.module_name.empty()) {
        mod = Module::Get(it->second.module_name);
    } else if (ModuleSet()) {
        mod = DefaultModule();
    } else {
        LOG_ERROR("No module set for offset '{}'", key);
        return std::nullopt;
    }

    if (!mod) {
        LOG_ERROR("Module not found for offset '{}'", key);
        return std::nullopt;
    }

    auto result = PatternScanner::Find(mod, it->second.signature);
    if (result) {
        Cache().emplace(key, *result);
        LOG_DEBUG("Resolved offset '{}' -> 0x{:X}", key, *result);
    } else {
        LOG_WARN("Failed to resolve offset '{}'", key);
    }
    return result;
}

std::optional<u64> OffsetManager::Get(std::string_view key) {
    auto& cache = Cache();
    auto it = cache.find(std::string(key));
    if (it != cache.end())
        return it->second;
    return std::nullopt;
}

void OffsetManager::SetDefaultModule(const Module& mod) noexcept {
    DefaultModule() = mod;
    ModuleSet() = true;
}

void OffsetManager::ClearCache() noexcept {
    Cache().clear();
}

// ── Internal state ────────────────────────────────────────────────────────────

OffsetManager::Map& OffsetManager::Entries() noexcept {
    static Map entries;
    return entries;
}

std::unordered_map<std::string, u64, std::hash<std::string>,
                   std::equal_to<>>& OffsetManager::Cache() noexcept {
    static std::unordered_map<std::string, u64, std::hash<std::string>,
                              std::equal_to<>> cache;
    return cache;
}

Module& OffsetManager::DefaultModule() noexcept {
    static Module mod;
    return mod;
}

bool& OffsetManager::ModuleSet() noexcept {
    static bool set = false;
    return set;
}
