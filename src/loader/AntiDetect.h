#pragma once

#include <string>
#include <array>
#include <cstddef>

/* ── Compile-Time String Encryption ─────────────────────────────────────────
 *
 * Multi-byte XOR key derived from __TIME__ so each build produces
 * different ciphertext. Decryption at runtime on first access.
 * ──────────────────────────────────────────────────────────────────────────── */

namespace enc {

inline constexpr unsigned char XOR_KEY[] = {
    static_cast<unsigned char>((__TIME__[0] * 7 + __TIME__[3] * 13) & 0xFF),
    static_cast<unsigned char>((__TIME__[1] * 11 + __TIME__[4] * 17) & 0xFF),
    static_cast<unsigned char>((__TIME__[2] * 3 + __TIME__[5] * 23) & 0xFF),
    static_cast<unsigned char>((__TIME__[0] + __TIME__[2] + __TIME__[5]) & 0xFF),
    static_cast<unsigned char>((__TIME__[1] * 5 + __TIME__[4] * 9) & 0xFF),
    static_cast<unsigned char>((__TIME__[3] * 7 + __TIME__[5] * 11) & 0xFF),
    static_cast<unsigned char>((__TIME__[0] ^ __TIME__[3] ^ 0xA5) & 0xFF),
    static_cast<unsigned char>((__TIME__[2] ^ __TIME__[5] ^ 0x5A) & 0xFF),
};

inline constexpr std::size_t XOR_KEY_LEN = sizeof(XOR_KEY);

template <std::size_t N>
class EncryptedString {
public:
    constexpr EncryptedString(const char (&str)[N]) : data_{} {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] = static_cast<unsigned char>(str[i]) ^ XOR_KEY[i % XOR_KEY_LEN];
    }

    std::string decrypt() const {
        std::string result(N - 1, '\0');
        for (std::size_t i = 0; i < N - 1; ++i)
            result[i] = static_cast<char>(data_[i] ^ XOR_KEY[i % XOR_KEY_LEN]);
        return result;
    }

    const char* c_str() {
        for (std::size_t i = 0; i < N - 1; ++i)
            buf_[i] = static_cast<char>(data_[i] ^ XOR_KEY[i % XOR_KEY_LEN]);
        buf_[N - 1] = '\0';
        return buf_;
    }

    constexpr std::size_t size() const { return N - 1; }

private:
    unsigned char data_[N];
    char buf_[N]{};
};

} // namespace enc

#define ENC_STR(name, str) constexpr auto name = enc::EncryptedString(str)
#define ENC_INLINE(str) enc::EncryptedString(str).decrypt()
