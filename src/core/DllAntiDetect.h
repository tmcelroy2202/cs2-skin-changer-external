#pragma once

#include <windows.h>
#include <string>

/* ── DLL Anti-Detection ─────────────────────────────────────────────────────
 *
 * Anti-debugging and anti-analysis measures for the DLL.
 * These run before the skin changer initializes.
 *
 * Measures:
 * - Debugger detection (IsDebuggerPresent, CheckRemoteDebuggerPresent)
 * - Hardware breakpoint detection
 * - Timing checks (RDTSC)
 * - Module validation
 * ──────────────────────────────────────────────────────────────────────────── */

namespace antidebug {

// Check if a debugger is present
inline bool IsDebuggerAttached() {
    // Check 1: IsDebuggerPresent
    if (::IsDebuggerPresent())
        return true;

    // Check 2: CheckRemoteDebuggerPresent
    BOOL debuggerPresent = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &debuggerPresent);
    if (debuggerPresent)
        return true;

    // Check 3: NtGlobalFlag (debugger sets specific flags)
    #ifdef _WIN64
    constexpr DWORD NT_GLOBAL_FLAG_DEBUGGED = 0x70;
    #else
    constexpr DWORD NT_GLOBAL_FLAG_DEBUGGED = 0x70;
    #endif
    DWORD* pNtGlobalFlag = (DWORD*)((PBYTE)__readgsqword(0x60) + 0xBC); // PEB->NtGlobalFlag
    if (*pNtGlobalFlag & NT_GLOBAL_FLAG_DEBUGGED)
        return true;

    return false;
}

// Check for hardware breakpoints via exception handler
inline bool HasHardwareBreakpoints() {
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3)
            return true;
    }
    return false;
}

// Timing-based detection (high precision)
inline bool CheckTiming() {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    // Do some work
    volatile int x = 0;
    for (int i = 0; i < 1000; ++i) x += i;

    QueryPerformanceCounter(&end);

    // If a debugger is attached, the timing will be significantly off
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    return elapsed > 0.1; // Should be microseconds, not milliseconds
}

// Run all anti-debug checks
inline bool DetectDebugger() {
    if (IsDebuggerAttached()) return true;
    if (HasHardwareBreakpoints()) return true;
    if (CheckTiming()) return true;
    return false;
}

} // namespace antidebug

/* ── Compile-Time String Encryption (DLL version) ─────────────────────────── */

namespace enc {

inline constexpr unsigned char DLL_XOR_KEY = []() constexpr -> unsigned char {
    constexpr char time[] = __TIME__;
    unsigned char key = 0;
    for (int i = 0; i < 8; ++i)
        key = static_cast<unsigned char>((key + time[i]) % 255 + 1);
    return key;
}();

template <std::size_t N>
class EncryptedString {
public:
    constexpr EncryptedString(const char (&str)[N]) : data_{} {
        for (std::size_t i = 0; i < N; ++i)
            data_[i] = static_cast<unsigned char>(str[i]) ^ DLL_XOR_KEY;
    }

    std::string decrypt() const {
        std::string result(N - 1, '\0');
        for (std::size_t i = 0; i < N - 1; ++i)
            result[i] = static_cast<char>(data_[i] ^ DLL_XOR_KEY);
        return result;
    }

    const char* c_str() {
        for (std::size_t i = 0; i < N - 1; ++i)
            buf_[i] = static_cast<char>(data_[i] ^ DLL_XOR_KEY);
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
