#include "MaterialSystem.h"
#include "core/Module.h"
#include "core/Memory.h"
#include "core/PatternScanner.h"
#include "core/Logging.h"
#include "core/InterfaceDiscovery.h"

bool MaterialSystem::Init() noexcept {
    auto mod = Module::Get("materialsystem2.dll");
    if (!mod) mod = Module::Get("engine2.dll");
    if (!mod) return false;

    auto* iface = InterfaceDiscovery::Get<void>(mod,
        "Source2EngineToMaterialSystemV001");
    if (iface) {
        material_system_addr_ = reinterpret_cast<u64>(iface);
        LOG_INFO("MaterialSystem at 0x{:X}", material_system_addr_);
        initialised_ = true;
        return true;
    }

    // Fallback scan
    auto found = PatternScanner::FindString(
        mod, "48 8B 05 ? ? ? ? 48 8B 48 ? 48 85 C9 74 ? 33 D2 48 8B 01 FF 50");
    if (found) {
        i32 disp = memory::Read<i32>(*found + 3);
        u64 global = *found + 7 + disp;
        material_system_addr_ = memory::Read<u64>(global);
        LOG_INFO("MaterialSystem via scan at 0x{:X}", material_system_addr_);
        initialised_ = true;
        return true;
    }

    LOG_WARN("MaterialSystem not found");
    return false;
}

std::vector<MaterialInfo> MaterialSystem::GetAllMaterials() const noexcept {
    std::vector<MaterialInfo> materials;
    if (!initialised_) return materials;

    // Walk the material list
    u64 material_list = memory::Read<u64>(material_system_addr_ + 0x18);
    u32 count = memory::Read<u32>(material_system_addr_ + 0x20);
    if (!material_list || count == 0 || count > 10000) return materials;

    for (u32 i = 0; i < count; ++i) {
        u64 mat_ptr = memory::Read<u64>(material_list + i * sizeof(void*));
        if (!mat_ptr) continue;

        MaterialInfo info;
        info.name = memory::ReadString(memory::Read<u64>(mat_ptr + 0x08));
        if (info.name.empty()) continue;

        info.shader_name = memory::ReadString(memory::Read<u64>(mat_ptr + 0x10));
        info.group_name  = memory::ReadString(memory::Read<u64>(mat_ptr + 0x18));
        info.memory_size = memory::Read<u64>(mat_ptr + 0x28);
        info.translucent = memory::Read<bool>(mat_ptr + 0x30);
        info.loaded = true;

        materials.push_back(std::move(info));
    }

    return materials;
}

std::optional<MaterialInfo>
MaterialSystem::FindMaterial(std::string_view name) const noexcept {
    for (auto& mat : GetAllMaterials()) {
        if (mat.name == name) return mat;
    }
    return std::nullopt;
}

u64 MaterialSystem::GetMaterialPtr(std::string_view name) const noexcept {
    u64 list = memory::Read<u64>(material_system_addr_ + 0x18);
    u32 count = memory::Read<u32>(material_system_addr_ + 0x20);
    if (!list) return 0;

    for (u32 i = 0; i < count; ++i) {
        u64 mat_ptr = memory::Read<u64>(list + i * sizeof(void*));
        if (!mat_ptr) continue;

        auto mat_name = memory::ReadString(memory::Read<u64>(mat_ptr + 0x08));
        if (mat_name == name) return mat_ptr;
    }
    return 0;
}
