#include "ObjectInspector.h"

InspectedObject ObjectInspector::Inspect(u64 address) noexcept {
    InspectedObject obj;
    if (!address) return obj;

    obj.address = address;

    // Read vtable pointer
    u64 vtable = memory::Read<u64>(address);
    if (!vtable) return obj;

    // Try to identify class from vtable
    auto* ci = IdentifyClass(vtable);
    if (ci) {
        return InspectWithClass(address, ci);
    }

    obj.class_name = "<unknown>";
    obj.valid = false;
    return obj;
}

InspectedObject ObjectInspector::InspectWithClass(
    u64 address, const SchemaClassInfo* info) noexcept
{
    InspectedObject obj;
    if (!address || !info) return obj;

    obj.address    = address;
    obj.class_info = info;
    obj.class_name = info->name ? info->name : "<unnamed>";
    obj.valid      = true;

    void* instance = reinterpret_cast<void*>(address);

    // Read fields
    obj.fields = reflection_.ReadFields(info, instance);

    return obj;
}

const SchemaClassInfo* ObjectInspector::IdentifyClass(u64 vtable) noexcept {
    if (!vtable) return nullptr;

    // Strategy: search all schema classes for one whose vtable matches.
    // Source 2 typically stores a SchemaClassInfo pointer near the vtable.
    // Common patterns:
    //   - At vtable[-2] (i.e., vtable - 0x10)
    //   - At vtable[0] (first vfunc sometimes points to class info)
    //   - At vtable[-1]

    for (i32 offset : { -0x10, -0x08, 0x00 }) {
        u64 ptr = memory::Read<u64>(vtable + offset);
        if (!ptr) continue;

        // Validate that this looks like a SchemaClassInfo
        auto* candidate = reinterpret_cast<SchemaClassInfo*>(ptr);
        if (candidate->name && candidate->field_count < 1000 &&
            candidate->size > 0 && candidate->size < 0x10000) {
            // Double-check by looking up in schema system
            auto* verified = schema_->FindClass(candidate->name);
            if (verified == candidate) {
                return candidate;
            }
        }
    }

    return nullptr;
}

std::string ObjectInspector::DumpText(const InspectedObject& obj,
                                       int indent) const noexcept {
    std::string result;
    std::string pad(indent * 2, ' ');

    result += std::format("{}class {} @ 0x{:X} ({} bytes)\n",
                          pad, obj.class_name, obj.address,
                          obj.class_info ? obj.class_info->size : 0);

    for (auto& fv : obj.fields) {
        result += std::format("{}  +0x{:04X} {} : {}\n",
                              pad, fv.offset, fv.name, fv.value_str);
    }

    return result;
}
