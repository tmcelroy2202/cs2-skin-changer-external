#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <span>
#include <optional>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <ranges>
#include <format>
#include <source_location>
#include <charconv>
#include <bit>
#include <concepts>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

using usize = size_t;
using uint  = unsigned int;

// ── Base path — directory of the DLL or EXE, set at startup.
//    Config/cache files stored alongside the binary, not in %APPDATA%.
#include <filesystem>

namespace detail {
inline std::filesystem::path g_base_path;
}

inline void SetBasePath(const std::filesystem::path& p) {
    detail::g_base_path = p.has_parent_path() ? p.parent_path() : p;
}

inline std::filesystem::path BasePath() {
    if (!detail::g_base_path.empty())
        return detail::g_base_path;
    // Fallback: current working directory
    return std::filesystem::current_path();
}
