#pragma once

#include "engine/Engine.h"
#include "SchemaTypes.h"

/* ── SchemaSystem ───────────────────────────────────────────────────────────
 *
 * High-level wrapper around CSchemaSystem that provides safe, cached access
 * to the Source 2 schema reflection layer.
 *
 * Responsibilities:
 *   1.  Locate and bind to the CSchemaSystem singleton at runtime.
 *   2.  Enumerate all SchemaSystemTypeScope objects (one per module).
 *   3.  Expose typed access to schema class and field information.
 *
 * Thread safety: This class is NOT thread-safe.  All access should
 * happen from the main/render thread after the engine has initialised.
 * ──────────────────────────────────────────────────────────────────────────── */

class SchemaSystem {
public:
    /// Initialise by discovering the schema system in process memory.
    bool Init() noexcept;

    /// Returns true if the schema system was successfully initialised.
    explicit operator bool() const noexcept { return initialised_; }

    /// Enumerate all known type scopes.
    [[nodiscard]] std::vector<SchemaSystemTypeScope*> GetTypeScopes() const noexcept;

    /// Find a type scope by module name (e.g. "client.dll", "server.dll").
    [[nodiscard]] SchemaSystemTypeScope* FindTypeScope(std::string_view module_name) const noexcept;

    /// Find a type scope by index.
    [[nodiscard]] SchemaSystemTypeScope* GetTypeScope(usize index) const noexcept;

    /// Get the number of type scopes.
    [[nodiscard]] usize GetTypeScopeCount() const noexcept;

    // ── Class queries ─────────────────────────────────────────────────────────

    /// Find a declared class by name across all type scopes.
    [[nodiscard]] SchemaClassInfo* FindClass(std::string_view class_name) const noexcept;

    /// Enumerate all classes in a given type scope.
    [[nodiscard]] std::vector<SchemaClassInfo*> GetClassesInScope(
        SchemaSystemTypeScope* scope) const noexcept;

    /// Get the declared class array from a type scope.
    [[nodiscard]] std::span<SchemaClassInfo*> GetDeclaredClasses(
        SchemaSystemTypeScope* scope) const noexcept;

    /// Read a field value from an instance.
    template <typename T>
    [[nodiscard]] T ReadField(void* instance, const SchemaClassFieldData& field) const noexcept {
        return memory::Read<T>(reinterpret_cast<u64>(instance) + field.single_offset);
    }

    /// Get the offset of a named field within a class.
    [[nodiscard]] std::optional<i32> GetFieldOffset(
        std::string_view class_name, std::string_view field_name) const noexcept;

    /// Get field data by name.
    [[nodiscard]] const SchemaClassFieldData* GetField(
        SchemaClassInfo* class_info, std::string_view field_name) const noexcept;

private:
    bool    initialised_{};
    u64     schema_system_addr_{};       // Address of CSchemaSystem singleton

    // Resolved offsets (populated by Init)
    struct {
        u64 type_scope_array   = 0x40;  // CSchemaSystem -> array of type scopes
        u64 type_scope_count   = 0x48;  // CSchemaSystem -> count
        u64 scope_class_array  = 0xE0;  // SchemaSystemTypeScope -> declared class array
        u64 scope_class_count  = 0xE8;  // SchemaSystemTypeScope -> class count
        u64 scope_module_name  = 0x50;  // SchemaSystemTypeScope -> module name string
    } offsets_{};
};
