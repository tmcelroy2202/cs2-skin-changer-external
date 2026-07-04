#pragma once

#include <string>
#include <array>
#include <cstddef>

/* ── Compile-Time String Encryption ─────────────────────────────────────────
 *
 * XOR-encrypts string literals at compile time so they never appear as
 * plaintext in the binary.  Decryption happens at runtime on first access.
 *
 * Usage:
 *   ENC_STR(myStr, "sensitive_string");
 *   std::string decrypted = myStr.decrypt();
 *
 * Or inline:
 *   auto s = ENC_STR("some_api_name").decrypt();
 * ──────────────────────────────────────────────────────────────────────────── */

namespace enc {

// XOR key derived from __TIME__ so each build produces different ciphertext.
// The exact value doesn't matter — it just prevents static pattern matching.
inline constexpr unsigned char XOR_KEY = []() constexpr -> unsigned char {
    // "HH:MM:SS" -> sum of digits mod 255 + 1
    constexpr char time[] = __TIME__;
    unsigned char key = 0;
    for (int i = 0; i < 8; ++i)
        key = static_cast<unsigned char>((key + time[i]) % 255 + 1);
    return key;
}();

template <std::size_t N>
class EncryptedString {
public:
    // Encrypt at compile time
    constexpr EncryptedString(const char (&str)[N]) : data_{} {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] = static_cast<unsigned char>(str[i]) ^ XOR_KEY;
    }

    // Decrypt at runtime (returns std::string)
    std::string decrypt() const {
        std::string result(N - 1, '\0');
        for (std::size_t i = 0; i < N - 1; ++i)
            result[i] = static_cast<char>(data_[i] ^ XOR_KEY);
        return result;
    }

    // Decrypt and return as const char* (static buffer — single-use!)
    const char* c_str() {
        decrypt_to(buf_);
        return buf_;
    }

    // Decrypt into a provided buffer
    void decrypt_to(char* out) const {
        for (std::size_t i = 0; i < N - 1; ++i)
            out[i] = static_cast<char>(data_[i] ^ XOR_KEY);
        out[N - 1] = '\0';
    }

    constexpr std::size_t size() const { return N - 1; }

private:
    unsigned char data_[N];
    char buf_[N]{};
};

} // namespace enc

// Convenience macro — use like: ENC_STR(name, "string literal")
// After this, `name` is an enc::EncryptedString and `name.decrypt()` returns std::string.
#define ENC_STR(name, str) constexpr auto name = enc::EncryptedString(str)

// Inline encryption — returns a temporary that can be immediately decrypted
#define ENC_INLINE(str) enc::EncryptedString(str).decrypt()
