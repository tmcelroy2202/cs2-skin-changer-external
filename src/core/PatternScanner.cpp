#include "PatternScanner.h"

#include <cstring>

// ── Parse ─────────────────────────────────────────────────────────────────────
std::pair<std::vector<u8>, std::string>
PatternScanner::Parse(std::string_view ida_pattern) {
    std::vector<u8> bytes;
    std::string     mask;

    auto it = ida_pattern.begin();
    while (it != ida_pattern.end()) {
        // Skip whitespace
        while (it != ida_pattern.end() && (*it == ' ' || *it == '\t'))
            ++it;
        if (it == ida_pattern.end()) break;

        if (*it == '?') {
            bytes.push_back(0);
            mask.push_back('?');
            ++it;
            // skip optional second '?'
            if (it != ida_pattern.end() && *it == '?') ++it;
        } else {
            char hex[3]{it[0], 0, 0};
            if (it + 1 != ida_pattern.end()) hex[1] = it[1];
            char* end{};
            u8 val = static_cast<u8>(std::strtol(hex, &end, 16));
            bytes.push_back(val);
            mask.push_back('x');
            it += (end - hex);
        }
    }
    return {std::move(bytes), std::move(mask)};
}

// ── FindRaw ───────────────────────────────────────────────────────────────────
u64 PatternScanner::FindRaw(const Module& mod, std::span<const u8> pattern,
                            std::string_view mask) {
    auto code = mod.CodeSpan();
    if (code.empty() || pattern.empty()) return 0;

    const usize pattern_len = pattern.size();

    for (usize i = 0; i + pattern_len <= code.size(); ++i) {
        bool match = true;
        for (usize j = 0; j < pattern_len; ++j) {
            if (mask[j] == 'x' && code[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return mod.Base() + i;
        }
    }
    return 0;
}

// ── ResolveRelative ───────────────────────────────────────────────────────────
u64 PatternScanner::ResolveRelative(u64 address, const Signature& sig) {
    if (sig.dereference_count == 0)
        return address + sig.additional_offset;

    // Read the relative displacement (usually a signed 32-bit int)
    i32 displacement = *reinterpret_cast<const i32*>(address + sig.relative_offset);
    u64 result = address + sig.relative_offset + sizeof(i32) + displacement;

    // Chain of dereferences
    for (u8 i = 1; i < sig.dereference_count; ++i) {
        result = *reinterpret_cast<const u64*>(result);
    }

    return result + sig.additional_offset;
}

// ── Find ──────────────────────────────────────────────────────────────────────
std::optional<u64> PatternScanner::Find(const Module& mod, const Signature& sig) {
    auto [bytes, mask] = Parse(sig.pattern_bytes);
    // If mask was explicitly provided in the Signature, use it
    if (!sig.mask.empty()) mask = sig.mask;

    u64 addr = FindRaw(mod, bytes, mask);
    if (!addr) return std::nullopt;

    if (sig.dereference_count > 0 || sig.additional_offset != 0) {
        addr = ResolveRelative(addr, sig);
    }
    return addr;
}

// ── FindString ────────────────────────────────────────────────────────────────
std::optional<u64> PatternScanner::FindString(const Module& mod,
                                              std::string_view ida_pattern) {
    auto [bytes, mask] = Parse(ida_pattern);
    u64 addr = FindRaw(mod, bytes, mask);
    if (!addr) return std::nullopt;
    return addr;
}
