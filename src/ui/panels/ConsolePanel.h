#pragma once

#include "ui/Panels.h"
#include "core/Logging.h"
#include <deque>

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
    ConsolePanel() noexcept : Panel("Console") {}

    std::string_view GetId() const noexcept override { return "Console"; }

    void Draw() noexcept override;

    /// Add a log message from the logging system.
    void AddMessage(std::string_view text, LogLevel level) noexcept;

    /// Clear all messages.
    void Clear() noexcept;

private:
    std::deque<LogMessage> messages_;
    std::string            filter_;
    bool                   auto_scroll_{true};
    bool                   scroll_to_bottom_{};
    usize                  max_messages_{1000};

    static ImVec4 ColorForLevel(LogLevel lv) noexcept;
};
