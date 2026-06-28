#include "SchemaSystem.h"

// ── Init ──────────────────────────────────────────────────────────────────────
bool SchemaSystem::Init() noexcept {
    // Attempt to locate CSchemaSystem via interface discovery
    // These are typical interface names across Source 2 games:
    std::initializer_list<const char*> names = {
        "Source2EngineToSchemaSystemV001",
        "Source2EngineToSchemaSystemV002",
        "SchemaSystem_001",
        "VSchemaSystem0",
    };

    for (auto* name : names) {
        auto mod = Module::Get("schemasystem.dll");
        if (!mod) mod = Module::Get("schemasystem.so");
        if (!mod) {
            // Might be linked into the main binary
            mod = Module::Get("engine2.dll");
        }
        if (!mod) continue;

        void* iface = InterfaceDiscovery::Get<void>(mod, name);
        if (iface) {
            schema_system_addr_ = reinterpret_cast<u64>(iface);
            LOG_INFO("SchemaSystem found via '{}' at 0x{:X}", name, schema_system_addr_);
            break;
        }
    }

    // Fallback: scan for the schema system pointer from known offsets
    if (!schema_system_addr_) {
        auto engine = Module::Get("engine2.dll");
        if (engine) {
            // Try to locate via signature: this is game-version specific
            auto found = PatternScanner::FindString(
                engine, "48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50 ? 48 8B D8");
            if (found) {
                i32 disp = memory::Read<i32>(*found + 3);
                schema_system_addr_ = *found + 7 + static_cast<u64>(disp);
                LOG_INFO("SchemaSystem found via fallback scan at 0x{:X}",
                         schema_system_addr_);
            }
        }
    }

    if (!schema_system_addr_) {
        LOG_ERROR("Failed to locate SchemaSystem");
        return false;
    }

    initialised_ = true;
    return true;
}

// ── Type scope access ─────────────────────────────────────────────────────────

usize SchemaSystem::GetTypeScopeCount() const noexcept {
    if (!initialised_) return 0;
    return memory::Read<u32>(schema_system_addr_ + offsets_.type_scope_count);
}

std::vector<SchemaSystemTypeScope*>
SchemaSystem::GetTypeScopes() const noexcept {
    if (!initialised_) return {};

    u32 count = memory::Read<u32>(schema_system_addr_ + offsets_.type_scope_count);
    u64 array = memory::Read<u64>(schema_system_addr_ + offsets_.type_scope_array);

    if (!array || count == 0) return {};

    std::vector<SchemaSystemTypeScope*> scopes;
    scopes.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto* scope = memory::Read<SchemaSystemTypeScope*>(array + i * sizeof(void*));
        if (scope) scopes.push_back(scope);
    }
    return scopes;
}

SchemaSystemTypeScope*
SchemaSystem::FindTypeScope(std::string_view module_name) const noexcept {
    for (auto* scope : GetTypeScopes()) {
        if (!scope) continue;
        u64 name_addr = reinterpret_cast<u64>(scope) + offsets_.scope_module_name;
        auto name = memory::ReadString(name_addr);
        if (name == module_name) return scope;
    }
    return nullptr;
}

SchemaSystemTypeScope*
SchemaSystem::GetTypeScope(usize index) const noexcept {
    if (!initialised_) return nullptr;
    u64 array = memory::Read<u64>(schema_system_addr_ + offsets_.type_scope_array);
    if (!array) return nullptr;
    return memory::Read<SchemaSystemTypeScope*>(array + index * sizeof(void*));
}

// ── Class queries ─────────────────────────────────────────────────────────────

SchemaClassInfo*
SchemaSystem::FindClass(std::string_view class_name) const noexcept {
    for (auto* scope : GetTypeScopes()) {
        auto classes = GetDeclaredClasses(scope);
        for (auto* ci : classes) {
            if (ci && ci->name && class_name == ci->name) {
                return ci;
            }
        }
    }
    return nullptr;
}

std::vector<SchemaClassInfo*>
SchemaSystem::GetClassesInScope(SchemaSystemTypeScope* scope) const noexcept {
    auto span = GetDeclaredClasses(scope);
    return {span.begin(), span.end()};
}

std::span<SchemaClassInfo*>
SchemaSystem::GetDeclaredClasses(SchemaSystemTypeScope* scope) const noexcept {
    if (!scope) return {};
    u64 base = reinterpret_cast<u64>(scope);
    u64 array = memory::Read<u64>(base + offsets_.scope_class_array);
    u32 count = memory::Read<u32>(base + offsets_.scope_class_count);
    if (!array || count == 0) return {};
    return {reinterpret_cast<SchemaClassInfo**>(array), count};
}

std::optional<i32>
SchemaSystem::GetFieldOffset(std::string_view class_name,
                             std::string_view field_name) const noexcept {
    auto* ci = FindClass(class_name);
    if (!ci) return std::nullopt;
    auto* field = GetField(ci, field_name);
    if (!field) return std::nullopt;
    return field->single_offset;
}

const SchemaClassFieldData*
SchemaSystem::GetField(SchemaClassInfo* ci,
                       std::string_view field_name) const noexcept {
    if (!ci || !ci->fields) return nullptr;
    for (u16 i = 0; i < ci->field_count; ++i) {
        auto& f = ci->fields[i];
        if (f.name && field_name == f.name) {
            return &f;
        }
    }
    return nullptr;
}
