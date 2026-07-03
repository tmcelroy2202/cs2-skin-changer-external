#pragma once

#include "ui/Panels.h"
#include "core/Logging.h"
#include "imgui.h"
#include <deque>
#include <mutex>

/* ── ConsolePanel ──────────────────────────────────────────────────────────
 *
 * Log output console that captures LOG_* messages and displays them with
 * colour-coded severity levels.
 *
 * Features:
 *   - Real-time log display with auto-scroll
 *   - Severity-based colouring
 *   - Search/filter
 *   - Copy-to-clipboard
 *   - Clear button
 * ──────────────────────────────────────────────────────────────────────────── */

struct LogMessage {
    std::string text;
    LogLevel    level;
    f32         timestamp;
};

class ConsolePanel final : public Panel {
public:
    ConsolePanel() noexcept : Panel("Console") {
        instance_ = this;
    }

    ~ConsolePanel() noexcept override {
        if (instance_ == this)
            instance_ = nullptr;
    }

    std::string_view GetId() const noexcept override { return "Console"; }

    void Draw() noexcept override;

    /// Add a log message from the logging system.
    void AddMessage(std::string_view text, LogLevel level) noexcept;

    /// Clear all messages.
    void Clear() noexcept;

    /// Static callback to feed Logger output into the panel.
    static void LoggerCallback(LogLevel lv, std::string_view msg) noexcept {
        if (instance_)
            instance_->AddMessage(msg, lv);
    }

private:
    std::deque<LogMessage> messages_;
    mutable std::mutex     messages_mutex_;
    std::string            filter_;
    bool                   auto_scroll_{true};
    bool                   scroll_to_bottom_{};
    usize                  max_messages_{1000};

    static inline ConsolePanel* instance_{nullptr};

    static ImVec4 ColorForLevel(LogLevel lv) noexcept;
};
