#pragma once

#include "Core.h"
#include "Module.h"

/* ── InterfaceDiscovery ─────────────────────────────────────────────────────
 *
 * Source 2 exposes its subsystem interfaces through a well-known exported
 * function: CreateInterface.  This class wraps that function and provides
 * typed access to engine interfaces.
 *
 * Typical use:
 *   auto engine = InterfaceDiscovery::Get<IEngineClient>("VEngineClient015");
 *   auto schema = InterfaceDiscovery::Get<CSchemaSystem>("Source2EngineToSchemaSystemV001");
 *
 * The returned pointer is a non-owning view of the singleton interface.
 * ──────────────────────────────────────────────────────────────────────────── */

class InterfaceDiscovery {
public:
    /// Function pointer type for CreateInterface export.
    using CreateInterfaceFn = void* (*)(const char* name, i32* return_code);

    /// Describes a registered interface entry.
    struct InterfaceEntry {
        std::string name;
        u64         address;
    };

    /// Enumerate all interfaces registered in a module.
    static std::vector<InterfaceEntry> Enumerate(const Module& mod) noexcept;

    /// Retrieve the CreateInterface function from a module.
    static CreateInterfaceFn GetFactory(const Module& mod) noexcept;

    /// Generic typed interface retrieval.
    template <typename T>
    static T* Get(const Module& mod, std::string_view name) noexcept {
        auto factory = GetFactory(mod);
        if (!factory) return nullptr;
        return static_cast<T*>(factory(name.data(), nullptr));
    }

private:
    /// Internal struct used by the interface list.
    struct InterfaceReg {
        void*           create_fn;
        const char*     name;
        InterfaceReg*   next;
    };
};
