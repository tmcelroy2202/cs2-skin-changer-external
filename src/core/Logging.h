#pragma once

#include "Core.h"
#include <source_location>
#include <string_view>
#include <atomic>

/* ── Logging ────────────────────────────────────────────────────────────────
 *
 * Tiered logging for diagnostic output.  Compiled to a no-op when NDEBUG is
 * defined (except warnings/errors which always survive).  Falls back to a
 * minimal stdout printer when spdlog is unavailable.
 *
 * Typical use:
 *   LOG_INFO("Schema system initialised with {} types", count);
 *   LOG_WARN("Unrecognised interface '{}'", name);
 *   LOG_ERROR("Pattern scan failed for {}", sig_name);
 * ──────────────────────────────────────────────────────────────────────────── */

enum class LogLevel : u8 {
    Trace, Debug, Info, Warn, Error, Fatal
};

using LogCallback = void(*)(LogLevel lv, std::string_view msg) noexcept;

class Logger {
public:
    static Logger& Instance();

    void Trace(std::string_view msg, std::source_location loc = std::source_location::current());
    void Debug(std::string_view msg, std::source_location loc = std::source_location::current());
    void Info (std::string_view msg, std::source_location loc = std::source_location::current());
    void Warn (std::string_view msg, std::source_location loc = std::source_location::current());
    void Error(std::string_view msg, std::source_location loc = std::source_location::current());
    void Fatal(std::string_view msg, std::source_location loc = std::source_location::current());

    void SetMinLevel(LogLevel lv) noexcept { min_level_ = lv; }

    /// Register a callback to receive all future log messages.
    void SetCallback(LogCallback cb) noexcept { callback_.store(cb, std::memory_order_release); }

private:
    void Dispatch(LogLevel lv, std::string_view msg, std::source_location const& loc);

    LogLevel min_level_{LogLevel::Info};
    std::atomic<LogCallback> callback_{nullptr};
};

// ── Convenience macros ───────────────────────────────────────────────────────

#define LOG_TRACE(...)  Logger::Instance().Trace(std::format(__VA_ARGS__))
#define LOG_DEBUG(...)  Logger::Instance().Debug(std::format(__VA_ARGS__))
#define LOG_INFO(...)   Logger::Instance().Info (std::format(__VA_ARGS__))
#define LOG_WARN(...)   Logger::Instance().Warn (std::format(__VA_ARGS__))
#define LOG_ERROR(...)  Logger::Instance().Error(std::format(__VA_ARGS__))
#define LOG_FATAL(...)  do { \
    Logger::Instance().Fatal(std::format(__VA_ARGS__)); \
    std::terminate(); \
} while (0)
