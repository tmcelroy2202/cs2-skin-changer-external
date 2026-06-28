#pragma once

#include "Core.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <stdexcept>

/* ── ProcessMemory ──────────────────────────────────────────────────────────
 *
 * External process memory reader/writer (ReadProcessMemory / WriteProcessMemory).
 * Mirrors the reference's `Memory` class but with RAII and type safety.
 *
 * Connects to cs2.exe with full access for reading and writing entity state.
 * ──────────────────────────────────────────────────────────────────────────── */

class ProcessMemory {
public:
    ProcessMemory() = default;

    /// Open a process by executable name.
    bool Open(std::wstring_view process_name) noexcept {
        Close();

        process_id_ = FindProcessId(process_name);
        if (!process_id_) return false;

        handle_ = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
            FALSE, process_id_);
        if (!handle_) return false;

        // Cache module bases
        client_ = GetModuleBase(L"client.dll");
        engine_ = GetModuleBase(L"engine2.dll");

        return client_ != 0;
    }

    void Close() noexcept {
        if (handle_) { CloseHandle(handle_); handle_ = nullptr; }
        process_id_ = 0;
        client_ = 0;
        engine_ = 0;
    }

    ~ProcessMemory() { Close(); }

    explicit operator bool() const noexcept { return handle_ != nullptr; }

    // ── Read ─────────────────────────────────────────────────────────────────
    template <typename T>
    [[nodiscard]] T Read(u64 address) const noexcept {
        T buffer{};
        ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address),
                          &buffer, sizeof(T), nullptr);
        return buffer;
    }

    template <typename T>
    [[nodiscard]] T Read(u64 base, i64 offset) const noexcept {
        return Read<T>(base + offset);
    }

    // ── Write ────────────────────────────────────────────────────────────────
    template <typename T>
    void Write(u64 address, const T& value) const noexcept {
        WriteProcessMemory(handle_, reinterpret_cast<LPVOID>(address),
                           &value, sizeof(T), nullptr);
    }

    // ── Utilities ────────────────────────────────────────────────────────────

    [[nodiscard]] u64 Client() const noexcept { return client_; }
    [[nodiscard]] u64 Engine() const noexcept { return engine_; }
    [[nodiscard]] u64 ProcessId() const noexcept { return process_id_; }

    /// Read a null-terminated string from the target process.
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

    /// Allocate memory in the target process.
    [[nodiscard]] u64 Allocate(u64 address = 0, usize size = 0x1000,
                               u32 protection = PAGE_EXECUTE_READWRITE) const noexcept {
        return reinterpret_cast<u64>(
            VirtualAllocEx(handle_, reinterpret_cast<LPVOID>(address), size,
                           MEM_COMMIT | MEM_RESERVE, protection));
    }

    /// Free memory in the target process.
    bool Free(u64 address) const noexcept {
        return VirtualFreeEx(handle_, reinterpret_cast<LPVOID>(address),
                             0, MEM_RELEASE) != 0;
    }

    /// Resolve a RIP-relative address at an instruction.
    u64 ResolveRelative(u64 instruction, i32 offset_offset = 3,
                        i32 instruction_size = 7) const noexcept {
        i32 disp = Read<i32>(instruction + offset_offset);
        return instruction + instruction_size + disp;
    }

    /// Scan for a signature in a module.
    u64 SigScan(std::wstring_view module, std::string_view pattern) const noexcept;

    /// Create a remote thread and wait for completion.
    HANDLE CreateRemoteThread(u64 func_addr, void* param = nullptr) const noexcept {
        return CreateThreadWrapper(func_addr, param, false);
    }

    void CallRemoteThread(u64 func_addr, void* param = nullptr) const noexcept {
        HANDLE h = CreateThreadWrapper(func_addr, param, true);
        if (h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
    }

    /// Fire-and-forget remote thread — does NOT block.
    void CallRemoteThreadAsync(u64 func_addr, void* param = nullptr) const noexcept {
        HANDLE h = ::CreateRemoteThread(handle_, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(func_addr), param,
            0, nullptr);
        if (h) CloseHandle(h);
    }

private:
    HANDLE handle_      = nullptr;
    u64    process_id_  = 0;
    u64    client_      = 0;
    u64    engine_      = 0;

    static u64 FindProcessId(std::wstring_view name) noexcept {
        u64 pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, name.data())) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &entry));
        }
        CloseHandle(snap);
        return pid;
    }

    u64 GetModuleBase(std::wstring_view name) const noexcept {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                                static_cast<DWORD>(process_id_));
        if (snap == INVALID_HANDLE_VALUE) return 0;

        u64 base = 0;
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(snap, &entry)) {
            do {
                if (!_wcsicmp(entry.szModule, name.data())) {
                    base = reinterpret_cast<u64>(entry.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snap, &entry));
        }
        CloseHandle(snap);
        return base;
    }

    HANDLE CreateThreadWrapper(u64 func_addr, void* param,
                                bool suspended) const noexcept {
        return ::CreateRemoteThread(handle_, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(func_addr), param,
            suspended ? CREATE_SUSPENDED : 0, nullptr);
    }
};

// ── Signature scanning ────────────────────────────────────────────────────────
inline u64 ProcessMemory::SigScan(std::wstring_view module,
                                   std::string_view pattern) const noexcept {
    u64 base = GetModuleBase(module);
    if (!base) return 0;

    // Get module size
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                            static_cast<DWORD>(process_id_));
    if (snap == INVALID_HANDLE_VALUE) return 0;

    usize module_size = 0;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Module32FirstW(snap, &entry)) {
        do {
            if (!_wcsicmp(entry.szModule, module.data())) {
                module_size = entry.modBaseSize;
                break;
            }
        } while (Module32NextW(snap, &entry));
    }
    CloseHandle(snap);
    if (!module_size) return 0;

    // Read the full module
    std::vector<u8> buffer(module_size);
    SIZE_T bytes_read = 0;
    if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(base),
                           buffer.data(), module_size, &bytes_read))
        return 0;

    // Parse pattern: "48 83 EC ? E8 ? ? ? ?"
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

    // Scan
    for (usize i = 0; i + sig.size() <= buffer.size(); ++i) {
        bool match = true;
        for (usize j = 0; j < sig.size(); ++j) {
            if (!sig[j].wildcard && buffer[i + j] != sig[j].byte) {
                match = false;
                break;
            }
        }
        if (match) return base + i;
    }

    return 0;
}
