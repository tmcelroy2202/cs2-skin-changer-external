#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <filesystem>
#include "AntiDetect.h"
#include "Console.h"

/* ── DLL Injection ─────────────────────────────────────────────────────────
 *
 * Implements DLL injection into the target process (CS2).
 * Uses CreateRemoteThread + LoadLibraryA for injection.
 *
 * Anti-detection measures:
 * - String encryption (via AntiDetect.h)
 * - Minimal API surface
 * - Clean memory handling
 * ──────────────────────────────────────────────────────────────────────────── */

namespace inj {

// Find process by executable name
inline DWORD FindProcess(const char* name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, name) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

// Wait for process to start (with timeout)
inline DWORD WaitForProcess(const char* name, int timeoutSec = 60) {
    con::Info(ENC_INLINE("Waiting for CS2 to start...").c_str());

    for (int i = 0; i < timeoutSec * 10; ++i) {
        DWORD pid = FindProcess(name);
        if (pid) return pid;
        Sleep(100);
    }
    return 0;
}

// Get the directory containing the current executable
inline std::string GetExeDir() {
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string s(path);
    auto pos = s.find_last_of("\\/");
    return (pos != std::string::npos) ? s.substr(0, pos) : s;
}

// Inject DLL into target process
inline bool Inject(DWORD pid, const std::string& dllPath) {
    // Verify DLL exists
    if (!std::filesystem::exists(dllPath)) {
        con::Error(("DLL not found: " + dllPath).c_str());
        return false;
    }

    con::Info(("Target PID: " + std::to_string(pid)).c_str());
    con::Info(("DLL path: " + dllPath).c_str());

    // Open process
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (!process) {
        con::Error("Failed to open process (run as admin?)");
        return false;
    }

    con::Progress("Opening process", 20);

    // Allocate memory for DLL path
    size_t pathLen = dllPath.size() + 1;
    void* remoteMem = VirtualAllocEx(process, nullptr, pathLen,
                                      MEM_COMMIT | MEM_RESERVE,
                                      PAGE_READWRITE);
    if (!remoteMem) {
        con::Error("Failed to allocate memory in target process");
        CloseHandle(process);
        return false;
    }

    con::Progress("Allocating memory", 40);

    // Write DLL path to allocated memory
    if (!WriteProcessMemory(process, remoteMem, dllPath.c_str(), pathLen, nullptr)) {
        con::Error("Failed to write DLL path to target process");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    con::Progress("Writing DLL path", 60);

    // Get LoadLibraryA address
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        con::Error("Failed to get kernel32.dll handle");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    // Decrypt function name at runtime
    auto loadLibName = enc::EncryptedString("LoadLibraryA").decrypt();
    FARPROC loadLibrary = GetProcAddress(kernel32, loadLibName.c_str());
    if (!loadLibrary) {
        con::Error("Failed to find LoadLibraryA");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    con::Progress("Resolving injection point", 80);

    // Create remote thread to call LoadLibraryA
    HANDLE thread = CreateRemoteThread(
        process, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary),
        remoteMem, 0, nullptr);

    if (!thread) {
        con::Error("Failed to create remote thread");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    con::Progress("Injecting DLL", 90);

    // Wait for injection to complete
    DWORD wait_result = WaitForSingleObject(thread, 5000);

    // Check if thread completed
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    // Clean up
    CloseHandle(thread);
    VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
    CloseHandle(process);

    if (wait_result == WAIT_TIMEOUT) {
        con::Warning("Injection thread timed out — DLL may still be loading");
        // Don't report failure — LoadLibrary may still be executing
        con::Progress("Injection complete (async)", 100);
        return true;
    }

    if (exitCode == 0) {
        con::Error("Injection failed — LoadLibraryA returned NULL");
        return false;
    }

    con::Progress("Injection complete", 100);
    return true;
}

// Main injection routine
inline bool Run() {
    con::Banner();

    // Find CS2
    DWORD pid = FindProcess("cs2.exe");
    if (!pid) {
        con::Warning("CS2 not found. Waiting for CS2 to start...");
        pid = WaitForProcess("cs2.exe", 120);
        if (!pid) {
            con::Error("Timed out waiting for CS2");
            return false;
        }
        con::Success("CS2 found!");
    } else {
        con::Success(("CS2 found (PID: " + std::to_string(pid) + ")").c_str());
    }

    // Get DLL path — try multiple names
    std::string exeDir = GetExeDir();
    std::string dllPath;
    for (const auto& name : {"libCS2SkinChanger.dll", "CS2SkinChanger.dll"}) {
        std::string candidate = exeDir + "\\" + name;
        if (std::filesystem::exists(candidate)) {
            dllPath = candidate;
            break;
        }
    }

    if (dllPath.empty()) {
        con::Error("DLL not found in the same folder as this exe.");
        con::Info("Make sure libCS2SkinChanger.dll is in the same folder.");
        return false;
    }

    // Inject
    con::Info("Injecting DLL...");
    if (!Inject(pid, dllPath)) {
        con::Error("Injection failed!");
        return false;
    }

    con::Success("DLL injected successfully!");
    con::Info("The skin changer is now active in CS2.");
    con::Info("Press Insert in-game to toggle the overlay.");

    return true;
}

} // namespace inj
