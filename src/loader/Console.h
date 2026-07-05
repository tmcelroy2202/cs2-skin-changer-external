#pragma once

#include <windows.h>
#include <string>
#include <cstdio>

/* ── Styled Console Output ─────────────────────────────────────────────────
 *
 * Provides colored console output for the loader with a consistent
 * visual style.  Uses Win32 console API for colors.
 * ──────────────────────────────────────────────────────────────────────────── */

namespace con {

enum Color : WORD {
    Black        = 0,
    DarkBlue     = 1,
    DarkGreen    = 2,
    DarkCyan     = 3,
    DarkRed      = 4,
    DarkMagenta  = 5,
    DarkYellow   = 6,
    Gray         = 7,
    DarkGray     = 8,
    Blue         = 9,
    Green        = 10,
    Cyan         = 11,
    Red          = 12,
    Magenta      = 13,
    Yellow       = 14,
    White        = 15,
};

inline HANDLE hConsole = nullptr;

inline void Init() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // Enable ANSI escape codes on Windows 10+
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

inline void SetColor(WORD color) {
    SetConsoleTextAttribute(hConsole, color);
}

inline void ResetColor() {
    SetColor(White);
}

inline void Print(const char* msg, WORD color = White) {
    SetColor(color);
    printf("%s", msg);
    ResetColor();
}

inline void PrintLine(const char* msg, WORD color = White) {
    SetColor(color);
    printf("%s\n", msg);
    ResetColor();
}

inline void Banner() {
    printf("\n");
    SetColor(Cyan);
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║                                                  ║\n");
    printf("  ║   ");
    SetColor(White);
    printf("VeneeR");
    SetColor(DarkGray);
    printf(" — Runtime Library                             ");
    SetColor(Cyan);
    printf("║\n");
    printf("  ║   ");
    SetColor(DarkGray);
    printf("Runtime Library                                ");
    SetColor(Cyan);
    printf("║\n");
    printf("  ║                                                  ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n");
    ResetColor();
    printf("\n");
}

inline void Success(const char* msg) {
    SetColor(Green);
    printf("  [+] ");
    ResetColor();
    printf("%s\n", msg);
}

inline void Info(const char* msg) {
    SetColor(Cyan);
    printf("  [*] ");
    ResetColor();
    printf("%s\n", msg);
}

inline void Warning(const char* msg) {
    SetColor(Yellow);
    printf("  [!] ");
    ResetColor();
    printf("%s\n", msg);
}

inline void Error(const char* msg) {
    SetColor(Red);
    printf("  [-] ");
    ResetColor();
    printf("%s\n", msg);
}

inline void Progress(const char* msg, int percent) {
    SetColor(Cyan);
    printf("  [*] ");
    ResetColor();
    printf("%s ", msg);

    // Draw progress bar
    SetColor(DarkGray);
    printf("[");
    SetColor(Green);
    int filled = percent / 5;
    for (int i = 0; i < 20; ++i) {
        printf(i < filled ? "=" : " ");
    }
    SetColor(DarkGray);
    printf("] ");
    ResetColor();
    printf("%d%%\n", percent);
}

inline void WaitForExit() {
    printf("\n");
    SetColor(DarkGray);
    printf("  Press Enter to exit...");
    ResetColor();
    getchar();
}

} // namespace con
