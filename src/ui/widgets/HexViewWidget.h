#pragma once

#include "core/Core.h"

/* ── HexViewWidget ──────────────────────────────────────────────────────────
 *
 * Reusable hex dump widget.  Renders a memory region as a traditional
 * hex + ASCII view with configurable base address and row width.
 *
 * This widget is stateless; all configuration is passed through the
 * Draw parameters.
 * ──────────────────────────────────────────────────────────────────────────── */

struct HexViewWidgetConfig {
    u64     base_address    = 0;
    usize   data_size       = 0x200;
    u8      bytes_per_row   = 16;
    u64     highlight_byte  = ~0ull;   // address to highlight (or ~0 for none)
    bool    show_address    = true;
    bool    show_ascii      = true;
    const char* id          = "##hexview";
};

namespace ui {

/// Draw a hex dump of memory at the given address.
void DrawHexView(const HexViewWidgetConfig& cfg) noexcept;

} // namespace ui
