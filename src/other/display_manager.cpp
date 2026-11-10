#include "display_manager.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/system_properties.h>
#include <mutex>
#include <chrono>

namespace display_safe {

static SafeDisplayInfo s_cached_info{ 1080, 2400, 0 };
static bool s_probed = false;
static std::mutex s_disp_mutex;

static bool read_file_line(const char* path, char* out, size_t max_len) {
    if (!path || !out || max_len == 0) return false;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;
    ssize_t n = read(fd, out, max_len - 1);
    close(fd);
    if (n > 0) {
        out[n] = '\0';
        // trim trailing whitespace/newline
        while (n > 0 && (out[n-1] == '\n' || out[n-1] == '\r' || out[n-1] == ' ')) {
            out[--n] = '\0';
        }
        return true;
    }
    return false;
}

SafeDisplayInfo get_info(bool force_refresh) {
    std::lock_guard<std::mutex> lock(s_disp_mutex);
    if (s_probed && !force_refresh) {
        return s_cached_info;
    }

    SafeDisplayInfo info{};
    info.width = 0;
    info.height = 0;
    info.orientation = 0;

    // Tier 1: Zero-Fork Direct Sysfs Nodes (Fastest, 0.05ms, Zero Fork)
    char buf[128] = {0};
    if (read_file_line("/sys/class/graphics/fb0/virtual_size", buf, sizeof(buf))) {
        int w = 0, h = 0;
        if (sscanf(buf, "%d,%d", &w, &h) == 2 && w > 0 && h > 0) {
            info.width = w;
            info.height = h;
        }
    }

    if (info.width <= 0) {
        if (read_file_line("/sys/class/graphics/fb0/modes", buf, sizeof(buf))) {
            char* p = strchr(buf, ':');
            const char* start = p ? p + 1 : buf;
            int w = 0, h = 0;
            if (sscanf(start, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
                info.width = w;
                info.height = h;
            }
        }
    }

    // Tier 2: Linux Framebuffer IOCTL (Zero Fork, Hardware Direct)
    if (info.width <= 0) {
        int fb_fd = open("/dev/graphics/fb0", O_RDONLY);
        if (fb_fd >= 0) {
            struct fb_var_screeninfo vinfo;
            if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
                if (vinfo.xres > 0 && vinfo.yres > 0) {
                    info.width = static_cast<int32_t>(vinfo.xres);
                    info.height = static_cast<int32_t>(vinfo.yres);
                }
            }
            close(fb_fd);
        }
    }

    // Tier 3: Android System Properties (Zero Fork, In-Memory Bionic Properties)
    if (info.width <= 0) {
        char prop_w[PROP_VALUE_MAX] = {0};
        char prop_h[PROP_VALUE_MAX] = {0};
        if (__system_property_get("ro.surface_flinger.max_graphics_width", prop_w) > 0 &&
            __system_property_get("ro.surface_flinger.max_graphics_height", prop_h) > 0) {
            int w = atoi(prop_w);
            int h = atoi(prop_h);
            if (w > 0 && h > 0) {
                info.width = w;
                info.height = h;
            }
        }
    }

    // Tier 4: Single-Shot Guarded Fallback via wm size (Bounded Execution)
    if (info.width <= 0 || info.height <= 0) {
        FILE *fp = popen("PATH=/system/bin:/bin wm size 2>/dev/null", "r");
        if (fp) {
            char line[256] = {0};
            while (fgets(line, sizeof(line), fp)) {
                char *p = strstr(line, "size:");
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
    }

    // Absolute Safety Fallback
    if (info.width <= 0 || info.height <= 0) {
        info.width = 1080;
        info.height = 2400;
    }

    // Orientation Detection — default to 1 (Landscape) for gaming overlay
    info.orientation = 1;
    char rotProp[PROP_VALUE_MAX] = {0};
    if (__system_property_get("ro.sf.hwrotation", rotProp) > 0) {
        int deg = atoi(rotProp);
        if (deg == 90) info.orientation = 1;
        else if (deg == 180) info.orientation = 2;
        else if (deg == 270) info.orientation = 3;
        else info.orientation = 0;
    }

    s_cached_info = info;
    s_probed = true;
    return s_cached_info;
}

void update_orientation(uint32_t rot) {
    std::lock_guard<std::mutex> lock(s_disp_mutex);
    s_cached_info.orientation = rot;
}

uint32_t probe_orientation() {
    static uint32_t s_last_rot = 1;
    static auto s_last_time = std::chrono::steady_clock::time_point{};
    auto now = std::chrono::steady_clock::now();

    // Throttle dumpsys query to at most once every 3 seconds to avoid blocking system_server IPC
    if (s_last_time.time_since_epoch().count() != 0 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_time).count() < 3000) {
        return s_last_rot;
    }
    s_last_time = now;

    uint32_t rot = s_last_rot;
    FILE *fp = popen("dumpsys input 2>/dev/null | grep -m 1 SurfaceOrientation", "r");
    if (fp) {
        char buf[128] = {0};
        if (fgets(buf, sizeof(buf), fp)) {
            char *p = strstr(buf, "SurfaceOrientation:");
            if (p) {
                int r = 0;
                if (sscanf(p + 19, " %d", &r) == 1 && r >= 0 && r <= 3) {
                    rot = static_cast<uint32_t>(r);
                }
            }
        }
        pclose(fp);
    }
    s_last_rot = rot;
    update_orientation(rot);
    return rot;
}

} // namespace display_safe
