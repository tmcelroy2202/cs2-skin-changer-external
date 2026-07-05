#ifndef NDEBUG

#include "Logging.h"

#include <cstdio>
#include <ctime>
#include <mutex>

static constexpr const char* LevelLabel(LogLevel lv) noexcept {
    switch (lv) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "?????";
}

Logger& Logger::Instance() {
    static Logger inst;
    return inst;
}

void Logger::Dispatch(LogLevel lv, std::string_view msg,
                      std::source_location const& loc) {
    if (lv < min_level_) return;

    static std::mutex mtx;
    std::lock_guard lock(mtx);

    std::time_t t = std::time(nullptr);
    char ts[32]{};
    std::strftime(ts, sizeof ts, "%H:%M:%S", std::localtime(&t));

    std::fprintf(stderr, "[%s] %s %s:%u | %.*s\n",
                 ts, LevelLabel(lv),
                 loc.file_name(), loc.line(),
                 static_cast<int>(msg.size()), msg.data());
    std::fflush(stderr);

    auto cb = callback_.load(std::memory_order_acquire);
    if (cb)
        cb(lv, msg);
}

void Logger::Trace(std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Trace, msg, loc); }
void Logger::Debug(std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Debug, msg, loc); }
void Logger::Info (std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Info,  msg, loc); }
void Logger::Warn (std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Warn,  msg, loc); }
void Logger::Error(std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Error, msg, loc); }
void Logger::Fatal(std::string_view msg, std::source_location loc) { Dispatch(LogLevel::Fatal, msg, loc); }

#endif // NDEBUG
