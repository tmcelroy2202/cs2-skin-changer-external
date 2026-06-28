#pragma once

#include "Core.h"

/* ── Module ─────────────────────────────────────────────────────────────────
 *
 * Represents a loaded PE module (EXE / DLL) in the current process.
 * Provides safe access to its memory regions, exports, and section headers.
 *
 * Acquired via:
 *   auto mod = Module::Get("client.dll");
 *   auto mod = Module::GetByAddress(some_function_pointer);
 * ──────────────────────────────────────────────────────────────────────────── */

class Module {
public:
    Module() = default;

    static Module Get(std::string_view name) noexcept;
    static Module GetByAddress(void* address) noexcept;

    explicit operator bool() const noexcept { return base_ != 0; }

    [[nodiscard]] u64       Base()      const noexcept { return base_; }
    [[nodiscard]] u64       End()       const noexcept { return base_ + size_; }
    [[nodiscard]] u64       Size()      const noexcept { return size_; }
    [[nodiscard]] std::string_view Name() const noexcept { return name_; }

    [[nodiscard]] u64       GetExport(std::string_view symbol) const noexcept;
    [[nodiscard]] std::span<const u8> CodeSpan() const noexcept;

    [[nodiscard]] bool Contains(u64 address) const noexcept {
        return address >= base_ && address < base_ + size_;
    }

private:
    u64         base_{};
    u64         size_{};
    std::string name_;
};
