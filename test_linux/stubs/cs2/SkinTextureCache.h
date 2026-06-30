#pragma once
#include "imgui.h"
#include "cs2/WeaponTypes.h"

class SkinTextureCache {
public:
    void ProcessPending() noexcept {}
    ImTextureID Get(i32) noexcept { return ImTextureID(0); }
    bool IsLoaded(i32) noexcept { return false; }
    void Request(i32, WeaponsEnum) noexcept {}
    void InitMapping() noexcept {}
};
