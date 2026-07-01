#pragma once
#include "imgui.h"
#include "cs2/WeaponTypes.h"

class SkinTextureCache {
public:
    virtual ~SkinTextureCache() noexcept = default;
    virtual void ProcessPending() noexcept {}
    virtual ImTextureID Get(i32) noexcept { return ImTextureID(0); }
    virtual bool IsLoaded(i32) noexcept { return false; }
    virtual void Request(i32, WeaponsEnum) noexcept {}
    virtual void InitMapping() noexcept {}
};
