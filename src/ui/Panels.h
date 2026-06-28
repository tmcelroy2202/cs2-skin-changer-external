#pragma once

#include "core/Core.h"

/* ── Panels ─────────────────────────────────────────────────────────────────
 *
 * Base class for all ImGui debug panels.  Each panel is a self-contained
 * UI unit that can be shown/hidden, docked, and serialised.
 *
 * The PanelManager owns all registered panels and dispatches Draw() calls
 * during the UI render cycle.
 * ──────────────────────────────────────────────────────────────────────────── */

class Panel {
public:
    explicit Panel(std::string_view title) : title_(title) {}
    virtual ~Panel() = default;

    /// Render the panel's contents via ImGui.
    virtual void Draw() noexcept = 0;

    /// Unique panel identifier.
    [[nodiscard]] virtual std::string_view GetId() const noexcept = 0;

    /// Human-readable title.
    [[nodiscard]] std::string_view GetTitle() const noexcept { return title_; }

    [[nodiscard]] bool* OpenPtr() noexcept { return &open_; }

    void SetOpen(bool open) noexcept { open_ = open; }
    bool IsOpen() const noexcept { return open_; }

    void SetVisible(bool visible) noexcept { visible_ = visible; }
    bool IsVisible() const noexcept { return visible_; }

protected:
    std::string title_;
    bool open_     = true;
    bool visible_  = true;
};

/// Manages a collection of panels.
class PanelManager {
public:
    void Register(Panel* panel) noexcept;
    void Unregister(Panel* panel) noexcept;

    /// Draw all open panels.
    void DrawAll() noexcept;

    /// Find a panel by ID.
    [[nodiscard]] Panel* Find(std::string_view id) const noexcept;

    /// Get all registered panels.
    [[nodiscard]] const std::vector<Panel*>& GetPanels() const noexcept { return panels_; }

    /// Set all panels visibility.
    void SetAllVisible(bool visible) noexcept;

private:
    std::vector<Panel*> panels_;
};
