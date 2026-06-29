#include "InterfaceDiscovery.h"

#include <cstring>

InterfaceDiscovery::CreateInterfaceFn
InterfaceDiscovery::GetFactory(const Module& mod) noexcept {
    u64 addr = mod.GetExport("CreateInterface");
    return reinterpret_cast<CreateInterfaceFn>(addr);
}

std::vector<InterfaceDiscovery::InterfaceEntry>
InterfaceDiscovery::Enumerate(const Module& mod) noexcept {
    std::vector<InterfaceEntry> entries;

    // Look for the s_pInterfaceReg list.  It's referenced inside CreateInterface.
    // We scan for the "?" pattern: lea rcx, s_pInterfaceReg
    // 48 8D 0D ? ? ? ?   (lea rcx, [rip + disp])
    // Then dereference to get the linked list head.
    auto factory = GetFactory(mod);
    if (!factory) return entries;

    // Search for the interface registration list
    // Common pattern: the interface list is referenced near CreateInterface via LEA
    u64 addr = reinterpret_cast<u64>(factory);
    auto mod_obj = Module::GetByAddress(reinterpret_cast<void*>(addr));
    if (!mod_obj) return entries;

    // Scan backwards from CreateInterface for the interface registration pointer
    // Pattern: 48 8D 0D ? ? ? ?  (lea rcx, [rip+disp]) followed by indirect call
    // This finds the interface linked list head.
    const u8 pattern[] = {0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00};
    const char mask[]  = "xxx????";

    u64 found = 0;
    auto code = mod_obj.CodeSpan();
    if (addr < mod_obj.Base()) return entries;
    usize start = (addr > mod_obj.Base() + 0x100) ? (addr - mod_obj.Base() - 0x100) : 0;
    usize end   = addr - mod_obj.Base();
    if (end > code.size()) end = code.size();

    for (usize i = start; i + sizeof(pattern) <= end; ++i) {
        bool match = true;
        for (usize j = 0; j < sizeof(pattern); ++j) {
            if (mask[j] == 'x' && code[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            i32 disp;
            std::memcpy(&disp, &code[i + 3], sizeof(disp));
            found = mod_obj.Base() + i + 7 + disp;
            break;
        }
    }

    if (!found) return entries;

    // Walk the linked list (32k max entries to prevent infinite loop on corruption)
    InterfaceReg* reg = *reinterpret_cast<InterfaceReg**>(found);
    u32 seen = 0;
    while (reg && seen < 32768) {
        entries.push_back(InterfaceEntry{
            .name    = reg->name ? reg->name : "<unnamed>",
            .address = reinterpret_cast<u64>(reg->create_fn)
        });
        reg = reg->next;
        ++seen;
    }

    return entries;
}
