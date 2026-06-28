#include "SchemaReflection.h"

// ── InspectClass ──────────────────────────────────────────────────────────────

ClassHierarchy SchemaReflection::InspectClass(std::string_view class_name) const noexcept {
    auto* info = schema_->FindClass(class_name);
    if (!info) return {};
    return InspectClass(info);
}

ClassHierarchy SchemaReflection::InspectClass(const SchemaClassInfo* info) const noexcept {
    ClassHierarchy hier;
    if (!info) return hier;

    hier.class_name = info->name ? info->name : "<unnamed>";
    hier.info       = info;

    // Collect fields from base classes first, then this class
    CollectFields(info, hier.fields, false);

    return hier;
}

// ── CollectFields (recursive) ─────────────────────────────────────────────────

void SchemaReflection::CollectFields(
    const SchemaClassInfo* info,
    std::vector<FieldValue>& out,
    bool is_base) const noexcept
{
    if (!info) return;

    // Process base classes first (so base fields appear before derived)
    if (info->base_classes && info->base_class_count > 0) {
        for (u32 i = 0; i < info->base_class_count; ++i) {
            if (info->base_classes[i]) {
                CollectFields(info->base_classes[i], out, true);
            }
        }
    }

    // Add this class's fields
    if (info->fields) {
        for (u16 i = 0; i < info->field_count; ++i) {
            auto& f = info->fields[i];
            FieldValue fv;
            fv.name    = f.name ? f.name : "<unnamed>";
            fv.type    = f.type;
            fv.offset  = f.single_offset;
            fv.is_base = is_base;
            out.push_back(std::move(fv));
        }
    }
}

// ── ReadFields ────────────────────────────────────────────────────────────────

std::vector<FieldValue> SchemaReflection::ReadFields(
    const SchemaClassInfo* info, void* instance) const noexcept
{
    std::vector<FieldValue> fields;
    if (!info || !instance) return fields;

    CollectFields(info, fields, false);

    for (auto& fv : fields) {
        auto* field = schema_->GetField(const_cast<SchemaClassInfo*>(info), fv.name);
        if (field) {
            fv.value_str = FormatFieldValue(*field, instance);
        }
    }

    return fields;
}

// ── SearchClasses ─────────────────────────────────────────────────────────────

std::vector<const SchemaClassInfo*>
SchemaReflection::SearchClasses(std::string_view substr) const noexcept {
    std::vector<const SchemaClassInfo*> results;
    for (auto* scope : schema_->GetTypeScopes()) {
        for (auto* ci : schema_->GetClassesInScope(scope)) {
            if (ci && ci->name) {
                std::string_view name(ci->name);
                if (name.find(substr) != std::string_view::npos) {
                    results.push_back(ci);
                }
            }
        }
    }
    return results;
}

// ── GetTotalFieldCount ────────────────────────────────────────────────────────

u32 SchemaReflection::GetTotalFieldCount(const SchemaClassInfo* info) noexcept {
    if (!info) return 0;
    u32 count = info->field_count;
    if (info->base_classes) {
        for (u32 i = 0; i < info->base_class_count; ++i) {
            if (info->base_classes[i]) {
                count += GetTotalFieldCount(info->base_classes[i]);
            }
        }
    }
    return count;
}

// ── FormatFieldValue ──────────────────────────────────────────────────────────

std::string SchemaReflection::FormatFieldValue(
    const SchemaClassFieldData& field, void* instance) noexcept
{
    u64 base = reinterpret_cast<u64>(instance);
    u64 addr = base + field.single_offset;

    switch (field.type) {
    case SchemaFieldType::Bool:
        return memory::Read<bool>(addr) ? "true" : "false";

    case SchemaFieldType::Int8:
    case SchemaFieldType::Char:
        return std::to_string(memory::Read<i8>(addr));

    case SchemaFieldType::UInt8:
    case SchemaFieldType::UChar:
        return std::to_string(memory::Read<u8>(addr));

    case SchemaFieldType::Int16:
    case SchemaFieldType::Short:
        return std::to_string(memory::Read<i16>(addr));

    case SchemaFieldType::UInt16:
    case SchemaFieldType::UShort:
        return std::to_string(memory::Read<u16>(addr));

    case SchemaFieldType::Int32:
    case SchemaFieldType::Int:
        return std::to_string(memory::Read<i32>(addr));

    case SchemaFieldType::UInt32:
    case SchemaFieldType::UInt:
        return std::to_string(memory::Read<u32>(addr));

    case SchemaFieldType::Int64:
    case SchemaFieldType::Long:
        return std::to_string(memory::Read<i64>(addr));

    case SchemaFieldType::UInt64:
    case SchemaFieldType::ULong:
        return std::to_string(memory::Read<u64>(addr));

    case SchemaFieldType::Float:
        return std::format("{:.6f}", memory::Read<f32>(addr));

    case SchemaFieldType::Double:
        return std::format("{:.6f}", memory::Read<f64>(addr));

    case SchemaFieldType::Vector2D: {
        auto x = memory::Read<f32>(addr);
        auto y = memory::Read<f32>(addr + 4);
        return std::format("({:.3f}, {:.3f})", x, y);
    }

    case SchemaFieldType::Vector3D: {
        auto x = memory::Read<f32>(addr);
        auto y = memory::Read<f32>(addr + 4);
        auto z = memory::Read<f32>(addr + 8);
        return std::format("({:.3f}, {:.3f}, {:.3f})", x, y, z);
    }

    case SchemaFieldType::CUtlString:
    case SchemaFieldType::CUtlString_2:
        return memory::ReadString(addr);

    case SchemaFieldType::EHandle:
    case SchemaFieldType::CEntityHandle:
        return std::format("Handle({})", memory::Read<u32>(addr));

    case SchemaFieldType::Color: {
        u32 c = memory::Read<u32>(addr);
        return std::format("RGBA({},{},{},{})",
                          (c >> 0) & 0xFF, (c >> 8) & 0xFF,
                          (c >> 16) & 0xFF, (c >> 24) & 0xFF);
    }

    case SchemaFieldType::CResourceHandle:
    case SchemaFieldType::CStrongHandle:
        return std::format("ResourceHandle(0x{:X})", memory::Read<u64>(addr));

    default:
        return std::format("(0x{:X}) {} bytes at +0x{:X}",
                          memory::Read<u64>(addr),
                          static_cast<int>(field.type),
                          field.single_offset);
    }
}
