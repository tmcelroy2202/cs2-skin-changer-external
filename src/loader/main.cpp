/* ── VeneeR Loader ──────────────────────────────────────────────────────────
 *
 * DLL injector for CS2 Skin Changer.
 * Finds the CS2 process and injects the DLL.
 *
 * Features:
 * - String encryption (compile-time XOR)
 * - Styled console output
 * - Automatic CS2 detection
 * - Clean error handling
 * ──────────────────────────────────────────────────────────────────────────── */

#include "Console.h"
#include "Injection.h"
#include <cstdio>

int main() {
    // Initialize console
    con::Init();

    // Run injection
    bool success = inj::Run();

    // Wait for user
    if (success) {
        con::Success("VeneeR is ready! Launch CS2 and press Insert to open the overlay.");
    } else {
        con::Error("Something went wrong. Make sure:");
        con::Info("  1. CS2 is running (or will be started)");
        con::Info("  2. CS2SkinChanger.dll is in the same folder as this loader");
        con::Info("  3. You are running as Administrator");
    }

    con::WaitForExit();
    return success ? 0 : 1;
}
