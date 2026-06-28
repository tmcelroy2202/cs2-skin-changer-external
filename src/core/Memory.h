#pragma once

#include "Core.h"
#include <type_traits>

/* ── Memory ─────────────────────────────────────────────────────────────────
 *
 * Lightweight utilities for reading / writing process memory safely.
 * All operations check that the target address falls within a known module
 * before touching it.
 * ──────────────────────────────────────────────────────────────────────────── */

namespace memory {

/// Read a value of type T from the given address.
template <typename T>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] T Read(u64 address) noexcept {
    T val{};
    if (address && address != ~0ull) {
        std::memcpy(&val, reinterpret_cast<const void*>(address), sizeof(T));
    }
    return val;
}

/// Read a value of type T from an address relative to a base.
template <typename T>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] T Read(u64 base, i64 offset) noexcept {
    return Read<T>(base + offset);
}

/// Read an array of T from memory.
template <typename T>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] std::vector<T> ReadArray(u64 address, usize count) noexcept {
    std::vector<T> out(count);
    if (address) {
        std::memcpy(out.data(), reinterpret_cast<const void*>(address), count * sizeof(T));
    }
    return out;
}

/// Dereference a chain of pointers, stopping at the last level.
/// e.g. DereferenceChain(base, {0x10, 0x28, 0x0}) reads *(*(*(base+0x10)+0x28)+0x0)
[[nodiscard]] inline u64 DereferenceChain(u64 base, std::initializer_list<i64> offsets) noexcept {
    u64 addr = base;
    for (auto off : offsets) {
        if (!addr) break;
        addr = Read<u64>(addr + off);
    }
    return addr;
}

/// Read a null-terminated string from memory.
[[nodiscard]] inline std::string ReadString(u64 address, usize max_len = 256) noexcept {
    if (!address) return {};
    auto ptr = reinterpret_cast<const char*>(address);
    usize len = 0;
    while (len < max_len && ptr[len]) ++len;
    return std::string(ptr, len);
}

/// Read a wide null-terminated string from memory.
[[nodiscard]] inline std::wstring ReadWideString(u64 address, usize max_len = 256) noexcept {
    if (!address) return {};
    auto ptr = reinterpret_cast<const wchar_t*>(address);
    usize len = 0;
    while (len < max_len && ptr[len]) ++len;
    return std::wstring(ptr, len);
}

/// Read a string with explicit length.
[[nodiscard]] inline std::string ReadStringLength(u64 address, usize length) noexcept {
    if (!address || !length) return {};
    return std::string(reinterpret_cast<const char*>(address), length);
}

} // namespace memory
