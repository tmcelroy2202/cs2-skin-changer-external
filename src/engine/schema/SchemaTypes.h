#pragma once

#include "core/Core.h"

/* ── Schema Types ───────────────────────────────────────────────────────────
 *
 * Source 2's Schema System is a comprehensive compile-time + runtime type
 * reflection layer.  Every class, field, and enum used by the engine is
 * registered here, making it the single most important subsystem for
 * runtime inspection.
 *
 * Memory layout (CSchemaSystem):
 *   The main system singleton holds a vector of SchemaSystemTypeScope objects,
 *   one per loaded module (client.dll, server.dll, etc.).
 *
 * SchemaSystemTypeScope:
 *   Contains a hash table mapping class names -> SchemaClassInfo.
 *   Also holds enums, enumsets, and global symbols.
 *
 * SchemaClassInfo:
 *   Describes a single reflected class: its fields, base classes, metadata,
 *   and instance size.  Fields are stored as SchemaClassFieldData.
 * ──────────────────────────────────────────────────────────────────────────── */

/* ── SchemaFieldType ──────────────────────────────────────────────────────────
 * Enumeration of possible schema field types.  These are the building blocks
 * of the Source 2 type system.
 */
enum class SchemaFieldType : u32 {
    Invalid             = 0,
    Bool                = 1,
    Int8                = 2,
    UInt8               = 3,
    Int16               = 4,
    UInt16              = 5,
    Int32               = 6,
    UInt32              = 7,
    Int64               = 8,
    UInt64              = 9,
    Float               = 10,
    Double              = 11,
    Vector4D            = 14,
    CTransform          = 16,
    Quaternion          = 17,
    QuaternionStorage   = 18,
    Matrix3x4           = 19,
    Matrix3x4VStorage   = 20,
    Color               = 24,
    Vector2D            = 25,
    Vector3D            = 26,
    Vector4D_2          = 27,
    CUtlString          = 29,
    CUtlSymbol          = 30,
    CUtlStringToken     = 31,
    CUtlSymbolLarge     = 32,
    CUtlString_2        = 37,
    CBufferString       = 41,
    CUtlBinaryBlock     = 42,
    Char                = 43,
    UChar               = 44,
    Short               = 46,
    UShort              = 47,
    Int                 = 48,
    UInt                = 49,
    Long                = 51,
    ULong               = 52,
    IntPointer          = 53,
    UIntPointer         = 54,
    Float64             = 55,
    Float128            = 57,
    CStrongHandle       = 66,
    CResourceHandle     = 67,
    EHandle             = 72,
    CEntityHandle       = 73,
    CUtlVector          = 75,
    CUtlReference       = 77,
    CUtlHashtable       = 79,
    CUtlNumVector       = 84,
    CUtlPair            = 88,
    CUtlMap             = 93,
    CUtlMultiset        = 99,
    CUtlSet             = 106,
    CUtlStack           = 113,
    CBufferArray        = 118,
    CTransform_2        = 130,
    CUtlStringToken_2   = 141,
    SchemaClassName     = 150,
    SchemaEnumName      = 152,
    SchemaEnum          = 156,
    SchemaClass         = 157,
    SchemaDeclaredClass = 170,
    CScriptArray        = 181,
    CUtlFlags           = 198,
    CUtlLeanVector      = 201,
    CUtlLeanVectorDyn   = 202,
};

/* ── SchemaClassFieldData ────────────────────────────────────────────────────
 * Describes a single field within a schema class.
 */
struct SchemaClassFieldData {
    const char*     name;            // Field name
    SchemaFieldType type;            // Base type
    u32             unknown_0;       // Padding / alignment info
    i32             single_offset;   // Offset from class base for non-array
    i32             array_offset;    // Offset for array data
    u32             array_count;     // Element count (0 = not array)
    i32             metadata_offset; // Offset to metadata
};

/* ── SchemaMetadataEntry ─────────────────────────────────────────────────────
 * Key-value metadata attached to classes and fields.
 */
struct SchemaMetadataEntry {
    u32             key_hash;        // Hash of key string
    u32             key_value;       // Inline value or offset to data
    const char*     key_name;        // Metadata key string
};

/* ── SchemaClassInfo ─────────────────────────────────────────────────────────
 * Full description of a reflected class.
 */
struct SchemaClassInfo {
    const char*             name;               // Class name
    const char*             module;             // Owning module
    u32                     size;               // Instance size in bytes
    u16                     field_count;        // Number of fields
    u16                     static_size;        // Size of static data
    u32                     base_class_count;   // Number of base classes
    SchemaClassInfo**       base_classes;       // Array of base class infos
    SchemaClassFieldData*   fields;             // Array of field descriptors
    SchemaMetadataEntry*    metadata;           // Array of metadata entries
    u32                     metadata_count;     // Number of metadata entries
    u64                     this_offset;        // Offset adjustment for multiple inheritance
    u64                     alignment;          // Required alignment
    u8                      pad[0x20];          // Remaining padding
};

/* ── SchemaEnumInfo ─────────────────────────────────────────────────────────
 * Describes a schema-registered enumeration.
 */
struct SchemaEnumInfo {
    const char*     name;
    u32             field_count;
    struct Field {
        const char* name;
        i32         value;
    }*              fields;
};

/* ── SchemaSystemTypeScope ───────────────────────────────────────────────────
 * Per-module type scope.  Each loaded module (client, server, engine, etc.)
 * has its own type scope containing all registered types.
 */
struct SchemaSystemTypeScope {
    u8              pad[0x050];             // VTable + internal state
    const char*     module_name;            // Name of the owning module
    u8              pad2[0x0E0 - 0x058];    // Hash tables and lookups
    // At offset 0x0E0 typically: array of DeclaredClass pointers
    // At offset 0x0E8 typically: count of declared classes
    // Structure is version-dependent; fields below are resolved via offsets
};

/* ── Schema System Instance ──────────────────────────────────────────────────
 * The main system interface.  Acquired via CreateInterface.
 */
struct SchemaSystemSchemaInstance {
    u8              pad[0x40];
    // Many engine versions store the type scope array at offsets
    // resolved dynamically by SchemaReflection.
};

// ── Helper enumeration names ─────────────────────────────────────────────────
inline constexpr std::string_view SchemaFieldTypeName(SchemaFieldType t) {
    using namespace std::literals;
    switch (t) {
    case SchemaFieldType::Invalid:           return "Invalid"sv;
    case SchemaFieldType::Bool:              return "bool"sv;
    case SchemaFieldType::Int8:              return "int8"sv;
    case SchemaFieldType::UInt8:             return "uint8"sv;
    case SchemaFieldType::Int16:             return "int16"sv;
    case SchemaFieldType::UInt16:            return "uint16"sv;
    case SchemaFieldType::Int32:             return "int32"sv;
    case SchemaFieldType::UInt32:            return "uint32"sv;
    case SchemaFieldType::Int64:             return "int64"sv;
    case SchemaFieldType::UInt64:            return "uint64"sv;
    case SchemaFieldType::Float:             return "float"sv;
    case SchemaFieldType::Double:            return "double"sv;
    case SchemaFieldType::Vector2D:          return "Vector2D"sv;
    case SchemaFieldType::Vector3D:          return "Vector3D"sv;
    case SchemaFieldType::Vector4D:          return "Vector4D"sv;
    case SchemaFieldType::CTransform:        return "CTransform"sv;
    case SchemaFieldType::Quaternion:        return "Quaternion"sv;
    case SchemaFieldType::CUtlString:        return "CUtlString"sv;
    case SchemaFieldType::CUtlSymbol:        return "CUtlSymbol"sv;
    case SchemaFieldType::CUtlVector:        return "CUtlVector"sv;
    case SchemaFieldType::EHandle:           return "EHandle"sv;
    case SchemaFieldType::CEntityHandle:     return "CEntityHandle"sv;
    case SchemaFieldType::SchemaDeclaredClass: return "SchemaDeclaredClass"sv;
    case SchemaFieldType::SchemaClass:       return "SchemaClass"sv;
    default:                                 return "Unknown"sv;
    }
}
