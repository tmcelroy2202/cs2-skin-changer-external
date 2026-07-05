#pragma once

#include <windows.h>
#include <string>

/* ── DLL Anti-Detection ─────────────────────────────────────────────────────
 *
 * Compile-time string encryption to prevent plaintext strings appearing
 * in the binary. Anti-debug measures removed — they create more
 * signatures than they prevent.
 * ──────────────────────────────────────────────────────────────────────────── */

namespace antidebug {

// Stub — always returns false. Keeps call sites compiling.
inline bool DetectDebugger() { return false; }

} // namespace antidebug

/* ── Compile-Time String Encryption (DLL version) ─────────────────────────── */

namespace enc {

// Multi-byte key derived from __TIME__ — different per build,
// prevents static pattern matching of the XOR loop.
inline constexpr unsigned char DLL_XOR_KEY[] = {
    static_cast<unsigned char>((__TIME__[0] * 7 + __TIME__[3] * 13) & 0xFF),
    static_cast<unsigned char>((__TIME__[1] * 11 + __TIME__[4] * 17) & 0xFF),
    static_cast<unsigned char>((__TIME__[2] * 3 + __TIME__[5] * 23) & 0xFF),
    static_cast<unsigned char>((__TIME__[0] + __TIME__[2] + __TIME__[5]) & 0xFF),
    static_cast<unsigned char>((__TIME__[1] * 5 + __TIME__[4] * 9) & 0xFF),
    static_cast<unsigned char>((__TIME__[3] * 7 + __TIME__[5] * 11) & 0xFF),
    static_cast<unsigned char>((__TIME__[0] ^ __TIME__[3] ^ 0xA5) & 0xFF),
    static_cast<unsigned char>((__TIME__[2] ^ __TIME__[5] ^ 0x5A) & 0xFF),
};

inline constexpr std::size_t XOR_KEY_LEN = sizeof(DLL_XOR_KEY);

template <std::size_t N>
class EncryptedString {
public:
    constexpr EncryptedString(const char (&str)[N]) : data_{} {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] = static_cast<unsigned char>(str[i]) ^ DLL_XOR_KEY[i % XOR_KEY_LEN];
    }

    std::string decrypt() const {
        std::string result(N - 1, '\0');
        for (std::size_t i = 0; i < N - 1; ++i)
            result[i] = static_cast<char>(data_[i] ^ DLL_XOR_KEY[i % XOR_KEY_LEN]);
        return result;
    }

    const char* c_str() {
        for (std::size_t i = 0; i < N - 1; ++i)
            buf_[i] = static_cast<char>(data_[i] ^ DLL_XOR_KEY[i % XOR_KEY_LEN]);
        buf_[N - 1] = '\0';
        return buf_;
    }

private:
    unsigned char data_[N];
    char buf_[N]{};
};

} // namespace enc

#define DLL_ENC_STR(name, str) constexpr auto name = enc::EncryptedString(str)
#define DLL_ENC_INLINE(str) enc::EncryptedString(str).decrypt()
