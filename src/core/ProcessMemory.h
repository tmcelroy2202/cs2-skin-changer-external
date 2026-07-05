#pragma once

#include "Core.h"
#include <atomic>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <stdexcept>
#include <optional>
#include <cstring>

/* ── ProcessMemory ──────────────────────────────────────────────────────────
 *
 * Memory reader/writer with two modes:
 *   External: ReadProcessMemory/WriteProcessMemory via dynamically-resolved
 *             Win32 APIs (XOR-obfuscated, no IAT traces).
 *   In-process (DLL): Direct memcpy/VirtualAlloc — no kernel round-trips,
 *             no anti-cheat API hooks triggered.
 * ──────────────────────────────────────────────────────────────────────────── */

namespace detail {

// Per-group XOR keys — no single key spans all APIs, defeating
// simple 0xAB-pattern scanners used by most anti-cheats.
static constexpr u8 XOR_KEY_MEM  = 0x5C;  // memory R/W APIs
static constexpr u8 XOR_KEY_SNAP = 0xB3;  // snapshot/enumeration APIs

template <typename Fn, size_t N>
Fn ResolveApi(HMODULE mod, const unsigned char (&enc)[N], u8 key) noexcept {
    char name[64];
    for (size_t i = 0; i < N; ++i) name[i] = static_cast<char>(enc[i] ^ key);
    name[N] = '\0';
    return reinterpret_cast<Fn>(::GetProcAddress(mod, name));
}

struct Win32Api {
    using pfnOpenProcess                = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
    using pfnReadProcessMemory          = BOOL(WINAPI*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
    using pfnWriteProcessMemory         = BOOL(WINAPI*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
    using pfnVirtualAllocEx             = LPVOID(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
    using pfnVirtualFreeEx              = BOOL(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD);
    using pfnCreateToolhelp32Snapshot   = HANDLE(WINAPI*)(DWORD, DWORD);
    using pfnProcess32FirstW            = BOOL(WINAPI*)(HANDLE, LPPROCESSENTRY32W);
    using pfnProcess32NextW             = BOOL(WINAPI*)(HANDLE, LPPROCESSENTRY32W);
    using pfnModule32FirstW             = BOOL(WINAPI*)(HANDLE, LPMODULEENTRY32W);
    using pfnModule32NextW              = BOOL(WINAPI*)(HANDLE, LPMODULEENTRY32W);
    using pfnCloseHandle                = BOOL(WINAPI*)(HANDLE);

    pfnOpenProcess              OpenProcess;
    pfnReadProcessMemory        ReadProcessMemory;
    pfnWriteProcessMemory       WriteProcessMemory;
    pfnVirtualAllocEx           VirtualAllocEx;
    pfnVirtualFreeEx            VirtualFreeEx;
    pfnCreateToolhelp32Snapshot CreateToolhelp32Snapshot;
    pfnProcess32FirstW          Process32FirstW;
    pfnProcess32NextW           Process32NextW;
    pfnModule32FirstW           Module32FirstW;
    pfnModule32NextW            Module32NextW;
    pfnCloseHandle              CloseHandle;

    static Win32Api& Instance() noexcept {
        static Win32Api inst;
        return inst;
    }

private:
    Win32Api() noexcept {
        HMODULE k32 = ::GetModuleHandleW(L"kernel32.dll");

        static constexpr unsigned char kOpenProcess[] = { 0x13, 0x2C, 0x39, 0x32, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F };
        static constexpr unsigned char kReadProcessMemory[] = { 0x0E, 0x39, 0x3D, 0x38, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F, 0x11, 0x39, 0x31, 0x33, 0x2E, 0x25 };
        static constexpr unsigned char kWriteProcessMemory[] = { 0x0B, 0x2E, 0x35, 0x28, 0x39, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F, 0x11, 0x39, 0x31, 0x33, 0x2E, 0x25 };
        static constexpr unsigned char kVirtualAllocEx[] = { 0x0A, 0x35, 0x2E, 0x28, 0x29, 0x3D, 0x30, 0x1D, 0x30, 0x30, 0x33, 0x3F, 0x19, 0x24 };
        static constexpr unsigned char kVirtualFreeEx[] = { 0x0A, 0x35, 0x2E, 0x28, 0x29, 0x3D, 0x30, 0x1A, 0x2E, 0x39, 0x39, 0x19, 0x24 };
        static constexpr unsigned char kCloseHandle[] = { 0x1F, 0x30, 0x33, 0x2F, 0x39, 0x14, 0x3D, 0x32, 0x38, 0x30, 0x39 };

        static constexpr unsigned char kCreateToolhelp32Snapshot[] = { 0xF0, 0xC1, 0xD6, 0xD2, 0xC7, 0xD6, 0xE7, 0xDC, 0xDC, 0xDF, 0xDB, 0xD6, 0xDF, 0xC3, 0x80, 0x81, 0xE0, 0xDD, 0xD2, 0xC3, 0xC0, 0xDB, 0xDC, 0xC7 };
        static constexpr unsigned char kProcess32FirstW[] = { 0xE3, 0xC1, 0xDC, 0xD0, 0xD6, 0xC0, 0xC0, 0x80, 0x81, 0xF5, 0xDA, 0xC1, 0xC0, 0xC7, 0xE4 };
        static constexpr unsigned char kProcess32NextW[] = { 0xE3, 0xC1, 0xDC, 0xD0, 0xD6, 0xC0, 0xC0, 0x80, 0x81, 0xFD, 0xD6, 0xCB, 0xC7, 0xE4 };
        static constexpr unsigned char kModule32FirstW[] = { 0xFE, 0xDC, 0xD7, 0xC6, 0xDF, 0xD6, 0x80, 0x81, 0xF5, 0xDA, 0xC1, 0xC0, 0xC7, 0xE4 };
        static constexpr unsigned char kModule32NextW[] = { 0xFE, 0xDC, 0xD7, 0xC6, 0xDF, 0xD6, 0x80, 0x81, 0xFD, 0xD6, 0xCB, 0xC7, 0xE4 };

        OpenProcess              = ResolveApi<pfnOpenProcess>(k32, kOpenProcess, XOR_KEY_MEM);
        ReadProcessMemory        = ResolveApi<pfnReadProcessMemory>(k32, kReadProcessMemory, XOR_KEY_MEM);
        WriteProcessMemory       = ResolveApi<pfnWriteProcessMemory>(k32, kWriteProcessMemory, XOR_KEY_MEM);
        VirtualAllocEx           = ResolveApi<pfnVirtualAllocEx>(k32, kVirtualAllocEx, XOR_KEY_MEM);
        VirtualFreeEx            = ResolveApi<pfnVirtualFreeEx>(k32, kVirtualFreeEx, XOR_KEY_MEM);
        CreateToolhelp32Snapshot = ResolveApi<pfnCreateToolhelp32Snapshot>(k32, kCreateToolhelp32Snapshot, XOR_KEY_SNAP);
        Process32FirstW          = ResolveApi<pfnProcess32FirstW>(k32, kProcess32FirstW, XOR_KEY_SNAP);
        Process32NextW           = ResolveApi<pfnProcess32NextW>(k32, kProcess32NextW, XOR_KEY_SNAP);
        Module32FirstW           = ResolveApi<pfnModule32FirstW>(k32, kModule32FirstW, XOR_KEY_SNAP);
        Module32NextW            = ResolveApi<pfnModule32NextW>(k32, kModule32NextW, XOR_KEY_SNAP);
        CloseHandle              = ResolveApi<pfnCloseHandle>(k32, kCloseHandle, XOR_KEY_MEM);
    }
};

} // namespace detail

class ProcessMemory {
public:
    ProcessMemory() = default;

    // ── DLL mode: in-process, direct memcpy (no kernel calls) ──────────────
    bool OpenCurrentProcess() noexcept {
        Close();
        in_process_ = true;
        process_id_ = ::GetCurrentProcessId();
        client_ = reinterpret_cast<u64>(::GetModuleHandleW(L"client.dll"));
        engine_ = reinterpret_cast<u64>(::GetModuleHandleW(L"engine2.dll"));
        if (!client_) {
            in_process_ = false;
            return false;
        }
        return true;
    }

    // ── External mode: ReadProcessMemory / WriteProcessMemory ───────────────
    bool Open(std::wstring_view process_name) noexcept {
        Close();

        process_id_ = FindProcessId(process_name);
        if (!process_id_) return false;

        handle_ = s_Api.OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
            FALSE, static_cast<DWORD>(process_id_));
        if (!handle_) return false;

        client_ = GetModuleBase(L"client.dll");
        engine_ = GetModuleBase(L"engine2.dll");

        return client_ != 0;
    }

    void Close() noexcept {
        if (handle_) { s_Api.CloseHandle(handle_); handle_ = nullptr; }
        in_process_ = false;
        process_id_ = 0;
        client_ = 0;
        engine_ = 0;
    }

    ~ProcessMemory() { Close(); }

    explicit operator bool() const noexcept {
        return in_process_ || handle_ != nullptr;
    }

    bool LastOpOk() const noexcept { return last_op_ok_; }

    // ── Read ───────────────────────────────────────────────────────────────
    template <typename T>
    [[nodiscard]] T Read(u64 address) const noexcept {
        T buffer{};
        if (in_process_) {
            if (address && address != ~0ull) {
                std::memcpy(&buffer, reinterpret_cast<const void*>(address), sizeof(T));
                last_op_ok_ = true;
            } else {
                last_op_ok_ = false;
            }
        } else {
            last_op_ok_ = s_Api.ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address),
                                                  &buffer, sizeof(T), nullptr) != 0;
        }
        return buffer;
    }

    template <typename T>
    [[nodiscard]] T Read(u64 base, i64 offset) const noexcept {
        return Read<T>(base + offset);
    }

    // ── ReadSafe ───────────────────────────────────────────────────────────
    template <typename T>
    [[nodiscard]] std::optional<T> ReadSafe(u64 address) const noexcept {
        if (!address || address == ~0ull) return std::nullopt;

        if (in_process_) {
            // Check page is readable before memcpy
            MEMORY_BASIC_INFORMATION mbi{};
            if (!::VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)))
                return std::nullopt;
            if (mbi.State != MEM_COMMIT)
                return std::nullopt;
            if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
                return std::nullopt;
            // Reject non-readable pages (PAGE_EXECUTE only, kernel, etc.)
            DWORD readable = mbi.Protect & 0xFF;
            if (readable != PAGE_READONLY && readable != PAGE_READWRITE &&
                readable != PAGE_EXECUTE_READ && readable != PAGE_EXECUTE_READWRITE &&
                readable != PAGE_WRITECOPY && readable != PAGE_EXECUTE_WRITECOPY)
                return std::nullopt;

            T buffer{};
            std::memcpy(&buffer, reinterpret_cast<const void*>(address), sizeof(T));
            return buffer;
        }

        T buffer{};
        if (s_Api.ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address),
                                    &buffer, sizeof(T), nullptr)) {
            return buffer;
        }
        return std::nullopt;
    }

    template <typename T>
    [[nodiscard]] std::optional<T> ReadSafe(u64 base, i64 offset) const noexcept {
        return ReadSafe<T>(base + offset);
    }

    // ── Write ──────────────────────────────────────────────────────────────
    template <typename T>
    bool Write(u64 address, const T& value) const noexcept {
        if (in_process_) {
            if (!address || address == ~0ull) { last_op_ok_ = false; return false; }
            std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
            last_op_ok_ = true;
            return true;
        }
        last_op_ok_ = s_Api.WriteProcessMemory(handle_, reinterpret_cast<LPVOID>(address),
                                               &value, sizeof(T), nullptr) != 0;
        return last_op_ok_;
    }

    template <typename T>
    bool Write(u64 base, i64 offset, const T& value) const noexcept {
        return Write<T>(base + offset, value);
    }

    // ── Module accessors ───────────────────────────────────────────────────
    [[nodiscard]] u64 Client() const noexcept { return client_; }
    [[nodiscard]] u64 Engine() const noexcept { return engine_; }
    [[nodiscard]] u64 ProcessId() const noexcept { return process_id_; }

    // ── Bulk string read (single memcpy instead of per-byte) ──────────────
    [[nodiscard]] std::string ReadString(u64 address, usize max_len = 256) const noexcept {
        if (!address || address == ~0ull) return {};
        if (in_process_) {
            // Bulk read in one memcpy, then scan for null
            std::string buf(max_len, '\0');
            std::memcpy(buf.data(), reinterpret_cast<const void*>(address), max_len);
            auto nul = buf.find('\0');
            if (nul != std::string::npos) buf.resize(nul);
            return buf;
        }
        // External: bulk read via RPM
        std::string buf(max_len, '\0');
        SIZE_T bytes_read = 0;
        if (!s_Api.ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address),
                                     buf.data(), max_len, &bytes_read))
            return {};
        buf.resize(bytes_read);
        auto nul = buf.find('\0');
        if (nul != std::string::npos) buf.resize(nul);
        return buf;
    }

    // ── Memory allocation ──────────────────────────────────────────────────
    [[nodiscard]] u64 Allocate(u64 address = 0, usize size = 0x1000,
                               u32 protection = PAGE_READWRITE) const noexcept {
        if (in_process_) {
            return reinterpret_cast<u64>(
                ::VirtualAlloc(reinterpret_cast<LPVOID>(address), size,
                               MEM_COMMIT | MEM_RESERVE, protection));
        }
        return reinterpret_cast<u64>(
            s_Api.VirtualAllocEx(handle_, reinterpret_cast<LPVOID>(address), size,
                                 MEM_COMMIT | MEM_RESERVE, protection));
    }

    bool Free(u64 address) const noexcept {
        if (in_process_) {
            return ::VirtualFree(reinterpret_cast<LPVOID>(address), 0, MEM_RELEASE) != 0;
        }
        return s_Api.VirtualFreeEx(handle_, reinterpret_cast<LPVOID>(address),
                                   0, MEM_RELEASE) != 0;
    }

    // ── Helpers ────────────────────────────────────────────────────────────
    u64 ResolveRelative(u64 instruction, i32 offset_offset = 3,
                        i32 instruction_size = 7) const noexcept {
        i32 disp = Read<i32>(instruction + offset_offset);
        return instruction + instruction_size + disp;
    }

    u64 SigScan(std::wstring_view module, std::string_view pattern) const noexcept;

private:
    HANDLE handle_      = nullptr;
    u64    process_id_  = 0;
    u64    client_      = 0;
    u64    engine_      = 0;
    bool   in_process_  = false;
    mutable std::atomic<bool> last_op_ok_{true};

    static detail::Win32Api& s_Api;

    static u64 FindProcessId(std::wstring_view name) noexcept {
        u64 pid = 0;
        HANDLE snap = s_Api.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        std::wstring name_str(name);

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (s_Api.Process32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, name_str.c_str())) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (s_Api.Process32NextW(snap, &entry));
        }
        s_Api.CloseHandle(snap);
        return pid;
    }

    u64 GetModuleBase(std::wstring_view name) const noexcept {
        HANDLE snap = s_Api.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                                     static_cast<DWORD>(process_id_));
        if (snap == INVALID_HANDLE_VALUE) return 0;

        std::wstring name_str(name);

        u64 base = 0;
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (s_Api.Module32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szModule, name_str.c_str())) {
                    base = reinterpret_cast<u64>(entry.modBaseAddr);
                    break;
                }
            } while (s_Api.Module32NextW(snap, &entry));
        }
        s_Api.CloseHandle(snap);
        return base;
    }
};

inline detail::Win32Api& ProcessMemory::s_Api = detail::Win32Api::Instance();

inline u64 ProcessMemory::SigScan(std::wstring_view module,
                                   std::string_view pattern) const noexcept {
    std::wstring module_str(module);
    u64 base = GetModuleBase(module);
    if (!base) return 0;

    HANDLE snap = s_Api.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                                 static_cast<DWORD>(process_id_));
    if (snap == INVALID_HANDLE_VALUE) return 0;

    usize module_size = 0;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (s_Api.Module32FirstW(snap, &entry)) {
        do {
            if (!_wcsicmp(entry.szModule, module_str.c_str())) {
                module_size = entry.modBaseSize;
                break;
            }
        } while (s_Api.Module32NextW(snap, &entry));
    }
    s_Api.CloseHandle(snap);
    if (!module_size) return 0;

    struct PatternByte { u8 byte; bool wildcard; };
    std::vector<PatternByte> sig;
    sig.reserve(pattern.size());
    auto it = pattern.begin();
    while (it != pattern.end()) {
        if (*it == ' ') { ++it; continue; }
        if (*it == '?') {
            sig.push_back({0, true});
            ++it;
            if (it != pattern.end() && *it == '?') ++it;
        } else {
            char hex[3]{it[0], 0, 0};
            if (it + 1 != pattern.end()) hex[1] = it[1];
            char* end{};
            sig.push_back({static_cast<u8>(std::strtoul(hex, &end, 16)), false});
            it += (end - hex) ? (end - hex) : 1;
        }
    }

    if (sig.empty()) return 0;

    // In-process: read directly from mapped module
    if (in_process_) {
        const u8* module_bytes = reinterpret_cast<const u8*>(base);
        for (usize i = 0; i + sig.size() <= module_size; ++i) {
            bool match = true;
            for (usize j = 0; j < sig.size(); ++j) {
                if (!sig[j].wildcard && module_bytes[i + j] != sig[j].byte) {
                    match = false;
                    break;
                }
            }
            if (match) return base + i;
        }
        return 0;
    }

    // External: chunked RPM
    constexpr usize kChunkSize = 1 << 20;
    std::vector<u8> chunk(kChunkSize);

    for (usize offset = 0; offset + sig.size() <= module_size; ) {
        usize to_read = std::min(kChunkSize, module_size - offset);
        SIZE_T bytes_read = 0;
        if (!s_Api.ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(base + offset),
                                     chunk.data(), to_read, &bytes_read))
            return 0;
        to_read = bytes_read;

        for (usize i = 0; i + sig.size() <= to_read; ++i) {
            bool match = true;
            for (usize j = 0; j < sig.size(); ++j) {
                if (!sig[j].wildcard && chunk[i + j] != sig[j].byte) {
                    match = false;
                    break;
                }
            }
            if (match) return base + offset + i;
        }

        offset += (to_read >= sig.size() - 1) ? to_read - sig.size() + 1 : to_read;
        if (offset >= module_size) break;
        if (offset + sig.size() > module_size) break;
    }

    return 0;
}
