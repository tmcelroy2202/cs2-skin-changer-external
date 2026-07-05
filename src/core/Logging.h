#pragma once

#include "Core.h"
#include <source_location>
#include <string_view>
#include <atomic>

/* ── Logging ────────────────────────────────────────────────────────────────
 *
 * Tiered logging for diagnostic output.  Compiled to a complete no-op when
 * NDEBUG is defined — no format strings, no fprintf, no strings in binary.
 * Only active in Debug/Development builds.
 * ──────────────────────────────────────────────────────────────────────────── */

enum class LogLevel : u8 {
    Trace, Debug, Info, Warn, Error, Fatal
};

using LogCallback = void(*)(LogLevel lv, std::string_view msg) noexcept;

#ifndef NDEBUG

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
    void SetCallback(LogCallback cb) noexcept { callback_.store(cb, std::memory_order_release); }

private:
    void Dispatch(LogLevel lv, std::string_view msg, std::source_location const& loc);
    LogLevel min_level_{LogLevel::Info};
    std::atomic<LogCallback> callback_{nullptr};
};

#define LOG_TRACE(...)  Logger::Instance().Trace(std::format(__VA_ARGS__))
#define LOG_DEBUG(...)  Logger::Instance().Debug(std::format(__VA_ARGS__))
#define LOG_INFO(...)   Logger::Instance().Info (std::format(__VA_ARGS__))
#define LOG_WARN(...)   Logger::Instance().Warn (std::format(__VA_ARGS__))
#define LOG_ERROR(...)  Logger::Instance().Error(std::format(__VA_ARGS__))
#define LOG_FATAL(...)  do { \
    Logger::Instance().Fatal(std::format(__VA_ARGS__)); \
    std::terminate(); \
} while (0)

#else

// Release builds: all logging compiled to absolute no-ops.
// No format strings, no function calls, no strings in binary.
class Logger {
public:
    static Logger& Instance() { static Logger inst; return inst; }
    void SetMinLevel(LogLevel) noexcept {}
    void SetCallback(LogCallback) noexcept {}
};

#define LOG_TRACE(...)  ((void)0)
#define LOG_DEBUG(...)  ((void)0)
#define LOG_INFO(...)   ((void)0)
#define LOG_WARN(...)   ((void)0)
#define LOG_ERROR(...)  ((void)0)
#define LOG_FATAL(...)  std::terminate()

#endif
