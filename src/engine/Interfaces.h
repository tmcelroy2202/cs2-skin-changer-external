#pragma once

#include "core/Core.h"

/* ── Engine Interfaces ──────────────────────────────────────────────────────
 *
 * Typed definitions for Source 2 engine interfaces accessed via
 * InterfaceDiscovery.  Each struct represents a known vtable interface
 * exported by the engine.
 *
 * The member functions listed here are the subset relevant to schema
 * reflection, entity inspection, resource introspection, and render
 * debugging — the core concerns of this framework.
 *
 * Virtual indices are documented as comments above each method.
 * ──────────────────────────────────────────────────────────────────────────── */

// ── Schema System ─────────────────────────────────────────────────────────────
// Accessed via CreateInterface("Source2EngineToSchemaSystemV001")
struct CSchemaSystem {
    //  0: Destructor
    //  1: FindTypeScope (by module name)
    using FindTypeScopeFn = void*(*)(CSchemaSystem*, const char*);
    //  2: FindTypeScope (by module pointer)
    //  3: FindDeclaredClass
    //  4: ...
    void** vtable{};

    // Resolved offset-based helpers (not vtable calls) are in SchemaSystem.h
};

// ── Entity System ────────────────────────────────────────────────────────────
// Accessed via CreateInterface("Source2EngineToEntitySystemV001")
struct CEntitySystem {
    void** vtable{};

    // Helper methods in EntitySystem.h
};

// ── Resource System ──────────────────────────────────────────────────────────
struct CResourceSystem {
    void** vtable{};
};

// ── Render / Material System ─────────────────────────────────────────────────
struct CMaterialSystem {
    void** vtable{};
};
