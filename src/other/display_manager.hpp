#pragma once
#include <cstdint>

struct SafeDisplayInfo {
    int32_t width = 1080;
    int32_t height = 2400;
    uint32_t orientation = 0; // 0=0deg, 1=90deg, 2=180deg, 3=270deg
};

namespace display_safe {
    // Queries screen dimensions safely using zero-fork sysfs and system properties first,
    // with guarded fallbacks. Completely isolated from volatile libgui.so binder calls.
    // Results are cached in RAM so calling from the 60fps render loop is instant.
    SafeDisplayInfo get_info(bool force_refresh = false);

    // Explicitly update orientation if device rotates
    void update_orientation(uint32_t rot);
}
