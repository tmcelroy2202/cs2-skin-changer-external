#include "PatternScanner.h"

#include <cctype>
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
            if (!std::isxdigit(static_cast<unsigned char>(*it))) {
                ++it;
                continue;
            }
            char hex[3]{it[0], 0, 0};
            if (it + 1 != ida_pattern.end()) hex[1] = it[1];
            char* end{};
            u8 val = static_cast<u8>(std::strtol(hex, &end, 16));
            bytes.push_back(val);
            mask.push_back('x');
            it += (end - hex);
            if (end == hex) ++it;
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
    const usize mask_len = mask.size();

    for (usize i = 0; i + pattern_len <= code.size(); ++i) {
        bool match = true;
        for (usize j = 0; j < pattern_len; ++j) {
            if (j < mask_len && mask[j] != 'x') continue;
            if (code[i + j] != pattern[j]) {
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
u64 PatternScanner::ResolveRelative(u64 address, const Signature& sig,
                                    const Module& mod) {
    if (sig.dereference_count == 0)
        return address + sig.additional_offset;

    // Read the relative displacement (usually a signed 32-bit int)
    if (!mod.Contains(address + sig.relative_offset + sizeof(i32) - 1))
        return 0;
    i32 displacement;
    std::memcpy(&displacement,
        reinterpret_cast<const void*>(address + sig.relative_offset),
        sizeof(displacement));
    u64 result = address + sig.relative_offset + sizeof(i32) + displacement;

    // Chain of dereferences
    for (u8 i = 1; i < sig.dereference_count; ++i) {
        if (!mod.Contains(result)) return 0;
        std::memcpy(&result,
            reinterpret_cast<const void*>(result), sizeof(result));
    }

    return result + sig.additional_offset;
}

// ── Find ──────────────────────────────────────────────────────────────────────
std::optional<u64> PatternScanner::Find(const Module& mod, const Signature& sig) {
    if (sig.pattern_bytes.empty()) return std::nullopt;

    u64 addr = FindRaw(mod, sig.pattern_bytes, sig.mask);
    if (!addr) return std::nullopt;

    if (sig.dereference_count > 0 || sig.additional_offset != 0) {
        addr = ResolveRelative(addr, sig, mod);
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
