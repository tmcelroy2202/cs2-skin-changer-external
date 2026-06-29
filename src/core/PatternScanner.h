#pragma once

#include "Core.h"
#include "Module.h"

/* ── PatternScanner ─────────────────────────────────────────────────────────
 *
 * Signature scanner for locating byte patterns in module memory.  Supports
 * the IDA / x64dbg style pattern format:
 *
 *   "48 8B 0D ? ? ? ? 48 85 C9 74 ?"
 *
 * Where '?' bytes are wildcards (masked during comparison).
 *
 * The Signature struct also encodes post-scan fixups used to resolve
 * relative addresses into absolute virtual addresses:
 *
 *   auto sig = Signature{
 *       .pattern      = "E8 ? ? ? ? 48 8B",
 *       .mask         = "x????x",
 *       .relative_offset = 0,      // at which byte is the relative offset?
 *       .dereference_count = 1     // how many dereferences to apply?
 *   };
 * ──────────────────────────────────────────────────────────────────────────── */

struct Signature {
    std::vector<u8> pattern_bytes;
    std::string     mask;              // 'x' = match, '?' = wildcard
    i32  relative_offset   = 0;        // offset inside pattern where relative addr sits
    u8   dereference_count = 1;        // how many times to dereference after resolve
    i32  additional_offset = 0;        // added after deref chain
};

class PatternScanner {
public:
    /// Convert IDA-style string "48 8B ? ? ? ? 48" into byte + mask vectors.
    static std::pair<std::vector<u8>, std::string> Parse(std::string_view ida_pattern);

    /// Find the first occurrence of a raw byte pattern in the module.
    static u64 FindRaw(const Module& mod, std::span<const u8> pattern,
                       std::string_view mask);

    /// High-level scan: parse pattern, scan, resolve relative address.
    static std::optional<u64> Find(const Module& mod, const Signature& sig);

    /// Scan for a literal IDA pattern string.
    static std::optional<u64> FindString(const Module& mod,
                                         std::string_view ida_pattern);

private:
    static u64 ResolveRelative(u64 address, const Signature& sig,
                               const Module& mod);
};

// ── Convenience macro ────────────────────────────────────────────────────────
// Allows writing signatures inline:
//   CSC_SIG("48 8B 0D ? ? ? ? 48 85 C9");
#define CSC_SIG(ida_pattern) \
    ([] { \
        auto [bytes, mask] = PatternScanner::Parse(ida_pattern); \
        return Signature{ \
            .pattern_bytes = std::move(bytes), \
            .mask = std::move(mask), \
            .relative_offset = 0, \
            .dereference_count = 0, \
            .additional_offset = 0 \
        }; \
    }())
