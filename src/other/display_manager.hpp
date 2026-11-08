#pragma once
#include <cstdint>

struct SafeDisplayInfo {
    int32_t width = 1080;
    int32_t height = 2400;
    uint32_t orientation = 0; // 0=0deg, 1=90deg, 2=180deg, 3=270deg
};

namespace display_safe {
    // Queries screen dimensions safely via wm size, dumpsys, and system properties
    // Completely isolated from volatile libgui.so binder calls to eliminate SIGSEGV.
    SafeDisplayInfo get_info();
}
