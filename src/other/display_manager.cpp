#include "display_manager.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/system_properties.h>

namespace display_safe {

SafeDisplayInfo get_info() {
    SafeDisplayInfo info{};
    info.width = 0;
    info.height = 0;
    info.orientation = 0;

    // 1. Probe Screen Size via /system/bin/wm size
    FILE *fp = popen("wm size 2>/dev/null", "r");
    if (fp) {
        char buf[256] = {0};
        while (fgets(buf, sizeof(buf), fp)) {
            // Check for "Override size: WxH" first, then "Physical size: WxH"
            char *p = strstr(buf, "size:");
            if (p) {
                int w = 0, h = 0;
                if (sscanf(p + 5, " %dx%d", &w, &h) == 2 && w > 0 && h > 0) {
                    info.width = w;
                    info.height = h;
                }
            }
        }
        pclose(fp);
    }

    // 2. Probe Orientation via dumpsys input or dumpsys window
    FILE *fp_rot = popen("dumpsys input 2>/dev/null | grep -m 1 SurfaceOrientation", "r");
    if (fp_rot) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), fp_rot)) {
            char *p = strstr(buf, "SurfaceOrientation:");
            if (p) {
                int rot = 0;
                if (sscanf(p + 19, " %d", &rot) == 1 && rot >= 0 && rot <= 3) {
                    info.orientation = static_cast<uint32_t>(rot);
                }
            }
        }
        pclose(fp_rot);
    } else {
        // Fallback: check system properties
        char propVal[PROP_VALUE_MAX] = {0};
        if (__system_property_get("ro.sf.hwrotation", propVal) > 0) {
            int deg = atoi(propVal);
            if (deg == 90) info.orientation = 1;
            else if (deg == 180) info.orientation = 2;
            else if (deg == 270) info.orientation = 3;
        }
    }

    // 3. Fallback defaults if hardware driver returns zero
    if (info.width <= 0 || info.height <= 0) {
        info.width = 1080;
        info.height = 2400;
    }

    return info;
}

} // namespace display_safe
