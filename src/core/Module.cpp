#include "Module.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cstring>

Module Module::Get(std::string_view name) noexcept {
    Module mod;
    HANDLE process = GetCurrentProcess();
    HMODULE hMods[1024]{};
    DWORD  needed{};

    if (EnumProcessModules(process, hMods, sizeof(hMods), &needed)) {
        DWORD count = needed / sizeof(HMODULE);
        for (DWORD i = 0; i < count; ++i) {
            char path[MAX_PATH]{};
            if (!GetModuleFileNameExA(process, hMods[i], path, MAX_PATH))
                continue;

            // Extract file name
            std::string_view full(path);
            auto pos = full.find_last_of('\\');
            std::string_view file = (pos != std::string_view::npos)
                                        ? full.substr(pos + 1)
                                        : full;

            if (file.size() == name.size() &&
                _strnicmp(file.data(), name.data(), name.size()) == 0) {
                MODULEINFO info{};
                if (GetModuleInformation(process, hMods[i], &info, sizeof(info))) {
                    mod.base_ = reinterpret_cast<u64>(info.lpBaseOfDll);
                    mod.size_ = info.SizeOfImage;
                    mod.name_ = file;
                }
                break;
            }
        }
    }
    return mod;
}

Module Module::GetByAddress(void* address) noexcept {
    Module mod;
    HMODULE hMod{};
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(address), &hMod)) {
        MODULEINFO info{};
        HANDLE process = GetCurrentProcess();
        if (GetModuleInformation(process, hMod, &info, sizeof(info))) {
            mod.base_ = reinterpret_cast<u64>(info.lpBaseOfDll);
            mod.size_ = info.SizeOfImage;

            char path[MAX_PATH]{};
            if (GetModuleFileNameExA(process, hMod, path, MAX_PATH)) {
                std::string_view full(path);
                auto pos = full.find_last_of('\\');
                mod.name_ = (pos != std::string_view::npos)
                                ? std::string(full.substr(pos + 1))
                                : std::string(full);
            }
        }
    }
    return mod;
}

u64 Module::GetExport(std::string_view symbol) const noexcept {
    if (!*this) return 0;
    HMODULE hMod = reinterpret_cast<HMODULE>(base_);
    std::string sym(symbol);
    FARPROC proc = GetProcAddress(hMod, sym.c_str());
    return proc ? reinterpret_cast<u64>(proc) : 0;
}

std::span<const u8> Module::CodeSpan() const noexcept {
    if (!*this) return {};
    return {reinterpret_cast<const u8*>(base_), static_cast<usize>(size_)};
}
