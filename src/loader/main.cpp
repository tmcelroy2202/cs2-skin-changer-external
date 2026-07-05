/* ── VeneeR Loader ──────────────────────────────────────────────────────────
 *
 * Single entry point for VeneeR CS2 Skin Changer.
 * Finds the CS2 process and injects the skin changer DLL.
 * ──────────────────────────────────────────────────────────────────────────── */

#include "Console.h"
#include "Injection.h"
#include <cstdio>

int main() {
    con::Init();
    bool success = inj::Run();

    if (success) {
        con::Success("VeneeR v1.8.1 is ready!");
        con::Info("Press Insert in-game to open the overlay.");
    } else {
        con::Error("Something went wrong. Make sure:");
        con::Info("  1. CS2 is running (or will be started soon)");
        con::Info("  2. libCS2SkinChanger.dll is in the same folder as this exe");
        con::Info("  3. You are running as Administrator");
    }

    con::WaitForExit();
    return success ? 0 : 1;
}
