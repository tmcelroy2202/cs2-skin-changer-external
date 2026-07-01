#pragma once

#include "Core.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <stdexcept>
#include <optional>

/* ── ProcessMemory ──────────────────────────────────────────────────────────
 *
 * External process memory reader/writer with dynamically-resolved Win32 APIs.
 * No suspicious function names appear in the import table — all are resolved
 * at runtime via GetProcAddress with XOR-obfuscated strings.
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
    using pfnCreateRemoteThread         = HANDLE(WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
    using pfnWaitForSingleObject        = DWORD(WINAPI*)(HANDLE, DWORD);

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
    pfnCreateRemoteThread       CreateRemoteThread;
    pfnWaitForSingleObject      WaitForSingleObject;

    static Win32Api& Instance() noexcept {
        static Win32Api inst;
        return inst;
    }

private:
    Win32Api() noexcept {
        HMODULE k32 = ::GetModuleHandleW(L"kernel32.dll");

        // ── Memory-operation APIs (key=0x5C) ────────────────────────────────

        static constexpr unsigned char kOpenProcess[] = { 0x13, 0x2C, 0x39, 0x32, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F };
        static constexpr unsigned char kReadProcessMemory[] = { 0x0E, 0x39, 0x3D, 0x38, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F, 0x11, 0x39, 0x31, 0x33, 0x2E, 0x25 };
        static constexpr unsigned char kWriteProcessMemory[] = { 0x0B, 0x2E, 0x35, 0x28, 0x39, 0x0C, 0x2E, 0x33, 0x3F, 0x39, 0x2F, 0x2F, 0x11, 0x39, 0x31, 0x33, 0x2E, 0x25 };
        static constexpr unsigned char kVirtualAllocEx[] = { 0x0A, 0x35, 0x2E, 0x28, 0x29, 0x3D, 0x30, 0x1D, 0x30, 0x30, 0x33, 0x3F, 0x19, 0x24 };
        static constexpr unsigned char kVirtualFreeEx[] = { 0x0A, 0x35, 0x2E, 0x28, 0x29, 0x3D, 0x30, 0x1A, 0x2E, 0x39, 0x39, 0x19, 0x24 };
        static constexpr unsigned char kCloseHandle[] = { 0x1F, 0x30, 0x33, 0x2F, 0x39, 0x14, 0x3D, 0x32, 0x38, 0x30, 0x39 };
        static constexpr unsigned char kCreateRemoteThread[] = { 0x1F, 0x2E, 0x39, 0x3D, 0x28, 0x39, 0x0E, 0x39, 0x31, 0x33, 0x28, 0x39, 0x08, 0x34, 0x2E, 0x39, 0x3D, 0x38 };
        static constexpr unsigned char kWaitForSingleObject[] = { 0x0B, 0x3D, 0x35, 0x28, 0x1A, 0x33, 0x2E, 0x0F, 0x35, 0x32, 0x3B, 0x30, 0x39, 0x13, 0x3E, 0x36, 0x39, 0x3F, 0x28 };

        // ── Snapshot/enumeration APIs (key=0xB3) ────────────────────────────

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
        CreateRemoteThread       = ResolveApi<pfnCreateRemoteThread>(k32, kCreateRemoteThread, XOR_KEY_MEM);
        WaitForSingleObject      = ResolveApi<pfnWaitForSingleObject>(k32, kWaitForSingleObject, XOR_KEY_MEM);
    }
};

} // namespace detail

class ProcessMemory {
public:
    ProcessMemory() = default;

    bool OpenCurrentProcess() noexcept {
        Close();
        process_id_ = ::GetCurrentProcessId();
        handle_ = s_Api.OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION
            | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
            FALSE, static_cast<DWORD>(process_id_));
        if (!handle_) return false;

        client_ = reinterpret_cast<u64>(::GetModuleHandleW(L"client.dll"));
        engine_ = reinterpret_cast<u64>(::GetModuleHandleW(L"engine2.dll"));
        return client_ != 0;
    }

    bool Open(std::wstring_view process_name) noexcept {
        Close();

        process_id_ = FindProcessId(process_name);
        if (!process_id_) return false;

        handle_ = s_Api.OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
            FALSE, static_cast<DWORD>(process_id_));
        if (!handle_) return false;

        client_ = GetModuleBase(L"client.dll");
        engine_ = GetModuleBase(L"engine2.dll");

        return client_ != 0;
    }

    void Close() noexcept {
        if (handle_) { s_Api.CloseHandle(handle_); handle_ = nullptr; }
        process_id_ = 0;
        client_ = 0;
        engine_ = 0;
    }

    ~ProcessMemory() { Close(); }

    explicit operator bool() const noexcept { return handle_ != nullptr; }

    bool LastOpOk() const noexcept { return last_op_ok_; }

    template <typename T>
    [[nodiscard]] T Read(u64 address) const noexcept {
        T buffer{};
        last_op_ok_ = s_Api.ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address),
                                              &buffer, sizeof(T), nullptr) != 0;
        return buffer;
    }

    template <typename T>
    [[nodiscard]] T Read(u64 base, i64 offset) const noexcept {
        return Read<T>(base + offset);
    }

    template <typename T>
    [[nodiscard]] std::optional<T> ReadSafe(u64 address) const noexcept {
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

    template <typename T>
    bool Write(u64 address, const T& value) const noexcept {
        last_op_ok_ = s_Api.WriteProcessMemory(handle_, reinterpret_cast<LPVOID>(address),
                                               &value, sizeof(T), nullptr) != 0;
        return last_op_ok_;
    }

    template <typename T>
    bool Write(u64 base, i64 offset, const T& value) const noexcept {
        return Write<T>(base + offset, value);
    }

    [[nodiscard]] u64 Client() const noexcept { return client_; }
    [[nodiscard]] u64 Engine() const noexcept { return engine_; }
    [[nodiscard]] u64 ProcessId() const noexcept { return process_id_; }

    [[nodiscard]] std::string ReadString(u64 address, usize max_len = 256) const noexcept {
        if (!address) return {};
        std::string result;
        result.reserve(128);
        for (usize i = 0; i < max_len; ++i) {
            char c = Read<char>(address + i);
            if (!c) break;
            result.push_back(c);
        }
        return result;
    }

    [[nodiscard]] u64 Allocate(u64 address = 0, usize size = 0x1000,
                               u32 protection = PAGE_READWRITE) const noexcept {
        return reinterpret_cast<u64>(
            s_Api.VirtualAllocEx(handle_, reinterpret_cast<LPVOID>(address), size,
                                 MEM_COMMIT | MEM_RESERVE, protection));
    }

    bool Free(u64 address) const noexcept {
        return s_Api.VirtualFreeEx(handle_, reinterpret_cast<LPVOID>(address),
                                   0, MEM_RELEASE) != 0;
    }

    u64 ResolveRelative(u64 instruction, i32 offset_offset = 3,
                        i32 instruction_size = 7) const noexcept {
        i32 disp = Read<i32>(instruction + offset_offset);
        return instruction + instruction_size + disp;
    }

    u64 SigScan(std::wstring_view module, std::string_view pattern) const noexcept;

    bool CallRemoteThread(u64 func_address, u64 arg = 0) const noexcept {
        if (!handle_ || !func_address) return false;
        HANDLE hThread = s_Api.CreateRemoteThread(
            handle_, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(func_address),
            reinterpret_cast<LPVOID>(arg), 0, nullptr);
        if (!hThread) return false;
        s_Api.WaitForSingleObject(hThread, 5000);
        s_Api.CloseHandle(hThread);
        return true;
    }

private:
    HANDLE handle_      = nullptr;
    u64    process_id_  = 0;
    u64    client_      = 0;
    u64    engine_      = 0;
    mutable bool last_op_ok_ = true;

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
