#pragma once

#include "core/Core.h"
#include "engine/schema/SchemaSystem.h"
#include "engine/schema/SchemaReflection.h"

/* ── ObjectInspector ────────────────────────────────────────────────────────
 *
 * Runtime object inspection utility.  Given a pointer to any schema-registered
 * object, ObjectInspector can:
 *
 *   1. Identify the object's class via vtable -> schema class info.
 *   2. Read and format all field values.
 *   3. Walk base class hierarchies.
 *   4. Follow pointer fields to inspect referenced objects.
 *   5. Dump the full object state as structured text.
 *
 * This is the core analysis primitive used by the memory inspector and
 * entity browser detail views.
 * ──────────────────────────────────────────────────────────────────────────── */

struct InspectedObject {
    u64                     address{};
    const SchemaClassInfo*  class_info{};
    std::string             class_name;
    bool                    valid{};

    std::vector<FieldValue> fields;
    std::vector<InspectedObject> bases;
};

class ObjectInspector {
public:
    explicit ObjectInspector(SchemaSystem* schema) noexcept
        : schema_(schema), reflection_(schema) {}

    /// Inspect an object at the given address.
    [[nodiscard]] InspectedObject Inspect(u64 address) noexcept;

    /// Inspect using schema class info directly (no vtable lookup).
    [[nodiscard]] InspectedObject InspectWithClass(
        u64 address, const SchemaClassInfo* info) noexcept;

    /// Try to identify the schema class from a vtable pointer.
    [[nodiscard]] const SchemaClassInfo* IdentifyClass(u64 vtable) noexcept;

    /// Dump an inspected object as formatted text (for console/file export).
    [[nodiscard]] std::string DumpText(const InspectedObject& obj,
                                        int indent = 0) const noexcept;

private:
    SchemaSystem*      schema_;
    SchemaReflection   reflection_;
};
