#pragma once

#include "engine/Engine.h"
#include "SchemaTypes.h"

/* ── SchemaReflection ───────────────────────────────────────────────────────
 *
 * Higher-level reflection utilities built on top of SchemaSystem.
 * Provides structured access to:
 *   - Class hierarchy traversal (base -> derived)
 *   - Field enumeration with type metadata
 *   - Instance introspection (read fields from an object pointer)
 *   - Schema data formatted as key-value pairs
 *
 * This is the primary API used by the UI panels for schema browsing.
 * ──────────────────────────────────────────────────────────────────────────── */

struct FieldValue {
    std::string             name;
    SchemaFieldType         type;
    i32                     offset;
    std::string             value_str;        // Formatted value
    bool                    is_base{false};    // True if inherited from base
};

struct ClassHierarchy {
    std::string                         class_name;
    const SchemaClassInfo*              info{};
    std::vector<FieldValue>             fields;
    std::vector<ClassHierarchy>         bases;
};

class SchemaReflection {
public:
    explicit SchemaReflection(const SchemaSystem* system) noexcept
        : schema_(system) {}

    /// Build a full class hierarchy (with inherited fields) for a given class.
    [[nodiscard]] ClassHierarchy InspectClass(std::string_view class_name) const noexcept;

    /// Build class hierarchy from SchemaClassInfo pointer.
    [[nodiscard]] ClassHierarchy InspectClass(const SchemaClassInfo* info) const noexcept;

    /// Read all field values from an instance of a class.
    [[nodiscard]] std::vector<FieldValue> ReadFields(
        const SchemaClassInfo* info, void* instance) const noexcept;

    /// Enumerate all classes whose name contains the given substring.
    [[nodiscard]] std::vector<const SchemaClassInfo*> SearchClasses(
        std::string_view substr) const noexcept;

    /// Get the total number of fields (including inherited ones).
    [[nodiscard]] static u32 GetTotalFieldCount(const SchemaClassInfo* info) noexcept;

    /// Format a field's value from an instance into a string.
    [[nodiscard]] static std::string FormatFieldValue(
        const SchemaClassFieldData& field, void* instance) noexcept;

private:
    const SchemaSystem* schema_;

    void CollectFields(const SchemaClassInfo* info,
                       std::vector<FieldValue>& out,
                       bool is_base) const noexcept;
};
