#pragma once
#include "core/Core.h"

class SkinChanger {
public:
    u16 GetMusicKit() const noexcept { return music_kit_id_; }
    void SetMusicKit(u16 id) noexcept { music_kit_id_ = id; }
private:
    u16 music_kit_id_ = 0;
};
