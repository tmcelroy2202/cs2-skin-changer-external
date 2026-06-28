#include "ResourceSystem.h"

// ── Init ──────────────────────────────────────────────────────────────────────
bool ResourceSystem::Init() noexcept {
    auto mod = Module::Get("materialsystem2.dll");
    if (!mod) mod = Module::Get("materialsystem2.so");
    if (!mod) mod = Module::Get("engine2.dll");
    if (!mod) return false;

    // Interface discovery
    auto* iface = InterfaceDiscovery::Get<void>(mod,
        "Source2EngineToResourceSystemV001");
    if (iface) {
        resource_system_addr_ = reinterpret_cast<u64>(iface);
        LOG_INFO("ResourceSystem at 0x{:X}", resource_system_addr_);
        initialised_ = true;
        return true;
    }

    // Fallback pattern scan
    auto found = PatternScanner::FindString(
        mod, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 48 ? 48 85 C9 74 ? FF 41");
    if (found) {
        i32 disp = memory::Read<i32>(*found + 3);
        u64 global = *found + 7 + disp;
        resource_system_addr_ = memory::Read<u64>(global);
        LOG_INFO("ResourceSystem via scan at 0x{:X}", resource_system_addr_);
        initialised_ = true;
        return true;
    }

    LOG_WARN("ResourceSystem not found");
    return false;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

u64 ResourceSystem::GetResourceList() const noexcept {
    // Typical offset to resource list in CResourceSystem
    return memory::Read<u64>(resource_system_addr_ + 0x10);
}

u32 ResourceSystem::GetResourceListCount() const noexcept {
    // Typical offset to resource count
    return memory::Read<u32>(resource_system_addr_ + 0x18);
}

// ── Material info ─────────────────────────────────────────────────────────────
MaterialInfo ResourceSystem::ReadMaterialInfo(u64 addr) const noexcept {
    MaterialInfo info;
    if (!addr) return info;

    info.name = memory::ReadString(memory::Read<u64>(addr + 0x08));
    info.shader_name = memory::ReadString(memory::Read<u64>(addr + 0x10));
    info.memory_size = memory::Read<u64>(addr + 0x20);
    info.translucent = memory::Read<bool>(addr + 0x28);
    info.loaded = true;
    return info;
}

TextureInfo ResourceSystem::ReadTextureInfo(u64 addr) const noexcept {
    TextureInfo info;
    if (!addr) return info;

    info.name = memory::ReadString(memory::Read<u64>(addr + 0x08));
    info.width = memory::Read<u32>(addr + 0x18);
    info.height = memory::Read<u32>(addr + 0x1C);
    info.format = memory::Read<u32>(addr + 0x20);
    info.memory_size = memory::Read<u64>(addr + 0x28);
    info.loaded = true;
    return info;
}

// ── Resource queries ──────────────────────────────────────────────────────────

u32 ResourceSystem::GetResourceCount() const noexcept {
    return GetResourceListCount();
}

std::vector<MaterialInfo>
ResourceSystem::GetMaterials(std::string_view filter) const noexcept {
    std::vector<MaterialInfo> result;
    u64 list = GetResourceList();
    u32 count = GetResourceListCount();
    if (!list || count == 0) return result;

    for (u32 i = 0; i < count; ++i) {
        u64 entry = memory::Read<u64>(list + i * sizeof(void*));
        if (!entry) continue;

        // Read the vtable to determine resource type; check if it's a material
        u64 vtable = memory::Read<u64>(entry);
        // Materials have characteristic vtable function pointers
        auto name = memory::ReadString(memory::Read<u64>(entry + 0x08));
        if (name.empty()) continue;

        if (!filter.empty() &&
            name.find(filter) == std::string_view::npos) continue;

        auto mat = ReadMaterialInfo(entry);
        if (!mat.name.empty())
            result.push_back(std::move(mat));
    }
    return result;
}

std::vector<TextureInfo>
ResourceSystem::GetTextures(std::string_view filter) const noexcept {
    std::vector<TextureInfo> result;
    u64 list = GetResourceList();
    u32 count = GetResourceListCount();
    if (!list || count == 0) return result;

    for (u32 i = 0; i < count; ++i) {
        u64 entry = memory::Read<u64>(list + i * sizeof(void*));
        if (!entry) continue;

        auto name = memory::ReadString(memory::Read<u64>(entry + 0x08));
        if (name.empty()) continue;

        if (!filter.empty() &&
            name.find(filter) == std::string_view::npos) continue;

        auto tex = ReadTextureInfo(entry);
        if (!tex.name.empty())
            result.push_back(std::move(tex));
    }
    return result;
}

ResourceState ResourceSystem::GetResourceState(
    const CResourceHandle& handle) const noexcept {
    if (!handle) return ResourceState::None;
    // Resolve handle to resource pointer
    u64 resource_ptr = handle.handle & ~0xFFF; // handle masking
    if (!resource_ptr) return ResourceState::None;
    u8 state = memory::Read<u8>(resource_ptr + 0x1C);
    return static_cast<ResourceState>(state);
}

bool ResourceSystem::IsResourceLoaded(
    const CResourceHandle& handle) const noexcept {
    return GetResourceState(handle) == ResourceState::Loaded;
}

u64 ResourceSystem::GetTotalMemoryUsage() const noexcept {
    u64 total = 0;
    u64 list = GetResourceList();
    u32 count = GetResourceListCount();
    if (!list) return 0;

    for (u32 i = 0; i < count; ++i) {
        u64 entry = memory::Read<u64>(list + i * sizeof(void*));
        if (!entry) continue;
        total += memory::Read<u64>(entry + 0x20);
    }
    return total;
}

u32 ResourceSystem::GetPendingLoadCount() const noexcept {
    u32 pending = 0;
    u64 list = GetResourceList();
    u32 count = GetResourceListCount();
    if (!list) return 0;

    for (u32 i = 0; i < count; ++i) {
        u64 entry = memory::Read<u64>(list + i * sizeof(void*));
        if (!entry) continue;
        u8 state = memory::Read<u8>(entry + 0x1C);
        if (state == static_cast<u8>(ResourceState::Pending) ||
            state == static_cast<u8>(ResourceState::Loading)) {
            ++pending;
        }
    }
    return pending;
}
