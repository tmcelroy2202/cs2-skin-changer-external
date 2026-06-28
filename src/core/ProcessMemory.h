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

static constexpr char XOR_KEY = 0xAB;

template <typename Fn, size_t N>
Fn ResolveApi(HMODULE mod, const unsigned char (&enc)[N]) noexcept {
    char name[64];
    for (size_t i = 0; i < N; ++i) name[i] = static_cast<char>(enc[i] ^ XOR_KEY);
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
    using pfnWaitForSingleObject        = DWORD(WINAPI*)(HANDLE, DWORD);
    using pfnCreateRemoteThread         = HANDLE(WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);

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
    pfnWaitForSingleObject      WaitForSingleObject;
    pfnCreateRemoteThread       CreateRemoteThread;

    static Win32Api& Instance() noexcept {
        static Win32Api inst;
        return inst;
    }

private:
    Win32Api() noexcept {
        HMODULE k32 = ::GetModuleHandleW(L"kernel32.dll");

        // ── XOR-obfuscated API names ────────────────────────────────────────
        // key = 0xAB

        // OpenProcess (11 chars)
        static constexpr unsigned char kOpenProcess[] = { 0xE4, 0xDB, 0xCE, 0xC5, 0xFB, 0xD9, 0xC4, 0xC8, 0xCE, 0xD8, 0xD8 };
        // ReadProcessMemory (17 chars)
        static constexpr unsigned char kReadProcessMemory[] = { 0xF9, 0xCE, 0xCA, 0xCF, 0xFB, 0xD9, 0xC4, 0xC8, 0xCE, 0xD8, 0xD8, 0xE6, 0xCE, 0xC6, 0xC4, 0xD9, 0xD2 };
        // WriteProcessMemory (18 chars)
        static constexpr unsigned char kWriteProcessMemory[] = { 0xFC, 0xD9, 0xC2, 0xDF, 0xCE, 0xFB, 0xD9, 0xC4, 0xC8, 0xCE, 0xD8, 0xD8, 0xE6, 0xCE, 0xC6, 0xC4, 0xD9, 0xD2 };
        // VirtualAllocEx (14 chars)
        static constexpr unsigned char kVirtualAllocEx[] = { 0xFD, 0xC2, 0xD9, 0xDF, 0xDE, 0xCA, 0xC7, 0xEA, 0xC7, 0xC7, 0xC4, 0xC8, 0xEE, 0xD3 };
        // VirtualFreeEx (13 chars)
        static constexpr unsigned char kVirtualFreeEx[] = { 0xFD, 0xC2, 0xD9, 0xDF, 0xDE, 0xCA, 0xC7, 0xED, 0xD9, 0xCE, 0xCE, 0xEE, 0xD3 };
        // CreateToolhelp32Snapshot (24 chars)
        static constexpr unsigned char kCreateToolhelp32Snapshot[] = { 0xE8, 0xD9, 0xCE, 0xCA, 0xDF, 0xCE, 0xFF, 0xC4, 0xC4, 0xC7, 0xC3, 0xCE, 0xC7, 0xDB, 0x98, 0x99, 0xF8, 0xC5, 0xCA, 0xDB, 0xD8, 0xC3, 0xC4, 0xDF };
        // Process32FirstW (15 chars)
        static constexpr unsigned char kProcess32FirstW[] = { 0xFB, 0xD9, 0xC4, 0xC8, 0xCE, 0xD8, 0xD8, 0x98, 0x99, 0xED, 0xC2, 0xD9, 0xD8, 0xDF, 0xFC };
        // Process32NextW (14 chars)
        static constexpr unsigned char kProcess32NextW[] = { 0xFB, 0xD9, 0xC4, 0xC8, 0xCE, 0xD8, 0xD8, 0x98, 0x99, 0xE5, 0xCE, 0xD3, 0xDF, 0xFC };
        // Module32FirstW (14 chars)
        static constexpr unsigned char kModule32FirstW[] = { 0xE6, 0xC4, 0xCF, 0xDE, 0xC7, 0xCE, 0x98, 0x99, 0xED, 0xC2, 0xD9, 0xD8, 0xDF, 0xFC };
        // Module32NextW (13 chars)
        static constexpr unsigned char kModule32NextW[] = { 0xE6, 0xC4, 0xCF, 0xDE, 0xC7, 0xCE, 0x98, 0x99, 0xE5, 0xCE, 0xD3, 0xDF, 0xFC };
        // CloseHandle (11 chars)
        static constexpr unsigned char kCloseHandle[] = { 0xE8, 0xC7, 0xC4, 0xD8, 0xCE, 0xE3, 0xCA, 0xC5, 0xCF, 0xC7, 0xCE };
        // WaitForSingleObject (19 chars)
        static constexpr unsigned char kWaitForSingleObject[] = { 0xFC, 0xCA, 0xC2, 0xDF, 0xED, 0xC4, 0xD9, 0xF8, 0xC2, 0xC5, 0xCC, 0xC7, 0xCE, 0xE4, 0xC9, 0xC1, 0xCE, 0xC8, 0xDF };
        // CreateRemoteThread (18 chars)
        static constexpr unsigned char kCreateRemoteThread[] = { 0xE8, 0xD9, 0xCE, 0xCA, 0xDF, 0xCE, 0xF9, 0xCE, 0xC6, 0xC4, 0xDF, 0xCE, 0xFF, 0xC3, 0xD9, 0xCE, 0xCA, 0xCF };

        OpenProcess              = ResolveApi<pfnOpenProcess>(k32, kOpenProcess);
        ReadProcessMemory        = ResolveApi<pfnReadProcessMemory>(k32, kReadProcessMemory);
        WriteProcessMemory       = ResolveApi<pfnWriteProcessMemory>(k32, kWriteProcessMemory);
        VirtualAllocEx           = ResolveApi<pfnVirtualAllocEx>(k32, kVirtualAllocEx);
        VirtualFreeEx            = ResolveApi<pfnVirtualFreeEx>(k32, kVirtualFreeEx);
        CreateToolhelp32Snapshot = ResolveApi<pfnCreateToolhelp32Snapshot>(k32, kCreateToolhelp32Snapshot);
        Process32FirstW          = ResolveApi<pfnProcess32FirstW>(k32, kProcess32FirstW);
        Process32NextW           = ResolveApi<pfnProcess32NextW>(k32, kProcess32NextW);
        Module32FirstW           = ResolveApi<pfnModule32FirstW>(k32, kModule32FirstW);
        Module32NextW            = ResolveApi<pfnModule32NextW>(k32, kModule32NextW);
        CloseHandle              = ResolveApi<pfnCloseHandle>(k32, kCloseHandle);
        WaitForSingleObject      = ResolveApi<pfnWaitForSingleObject>(k32, kWaitForSingleObject);
        CreateRemoteThread       = ResolveApi<pfnCreateRemoteThread>(k32, kCreateRemoteThread);
    }
};

} // namespace detail

class ProcessMemory {
public:
    ProcessMemory() = default;

    bool Open(std::wstring_view process_name) noexcept {
        Close();

        process_id_ = FindProcessId(process_name);
        if (!process_id_) return false;

        handle_ = s_Api.OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
            PROCESS_CREATE_THREAD,
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
                               u32 protection = PAGE_EXECUTE_READWRITE) const noexcept {
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

    HANDLE SpawnRemoteThread(u64 func_addr, void* param = nullptr) const noexcept {
        return RemoteThreadWrap(func_addr, param, false);
    }

    void CallRemoteThread(u64 func_addr, void* param = nullptr) const noexcept {
        HANDLE h = RemoteThreadWrap(func_addr, param, true);
        if (h) { s_Api.WaitForSingleObject(h, INFINITE); s_Api.CloseHandle(h); }
    }

    void CallRemoteThreadAsync(u64 func_addr, void* param = nullptr) const noexcept {
        HANDLE h = s_Api.CreateRemoteThread(handle_, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(func_addr), param,
            0, nullptr);
        if (h) {
            s_Api.WaitForSingleObject(h, 100);
            s_Api.CloseHandle(h);
        }
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

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (s_Api.Process32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, name.data())) {
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

        u64 base = 0;
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (s_Api.Module32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szModule, name.data())) {
                    base = reinterpret_cast<u64>(entry.modBaseAddr);
                    break;
                }
            } while (s_Api.Module32NextW(snap, &entry));
        }
        s_Api.CloseHandle(snap);
        return base;
    }

    HANDLE RemoteThreadWrap(u64 func_addr, void* param, bool suspended) const noexcept {
        return s_Api.CreateRemoteThread(handle_, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(func_addr), param,
            suspended ? CREATE_SUSPENDED : 0, nullptr);
    }
};

inline detail::Win32Api& ProcessMemory::s_Api = detail::Win32Api::Instance();

inline u64 ProcessMemory::SigScan(std::wstring_view module,
                                   std::string_view pattern) const noexcept {
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
            if (!_wcsicmp(entry.szModule, module.data())) {
                module_size = entry.modBaseSize;
                break;
            }
        } while (s_Api.Module32NextW(snap, &entry));
    }
    s_Api.CloseHandle(snap);
    if (!module_size) return 0;

    struct PatternByte { u8 byte; bool wildcard; };
    std::vector<PatternByte> sig;
    const char* p = pattern.data();
    while (*p) {
        if (*p == ' ') { ++p; continue; }
        if (*p == '?') {
            sig.push_back({0, true});
            ++p;
            if (*p == '?') ++p;
        } else {
            char hex[3] = {p[0], p[1] ? p[1] : 0, 0};
            sig.push_back({static_cast<u8>(strtoul(hex, nullptr, 16)), false});
            p += 2;
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
