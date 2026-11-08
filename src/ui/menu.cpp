#include "menu.hpp"
#include "imgui.h"
#include "Android_draw/draw.h"
#include "Android_touch/Touch.hpp"
#include "func/esp.hpp"
#include "func/aim.hpp"
#include "cfg.hpp"
#include "game/data.hpp"
#include "encrypt.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cstdio>

using namespace ImGui;

// Forward-declare Dear ImGui's ClearActiveID to cancel active widgets during scroll
namespace ImGui {
    IMGUI_API void ClearActiveID();
}

extern std::atomic<bool> main_thread_flag;
static int activeTab = 0;
static float ui_opacity = 1.0f;
static std::atomic<bool> menu_open{false};
static std::atomic<bool> g_should_exit{false};

// DESIGN 08: ELECTRIC ORCHID GLASS — COLOR DEFINITIONS
namespace theme_orchid {
    constexpr ImU32 ColPrimary       = IM_COL32(225, 0, 152, 255);       // Electric Orchid #E10098
    constexpr ImU32 ColPrimaryLight  = IM_COL32(255, 90, 205, 255);      // Soft Orchid Glow #FF5ACD
    constexpr ImU32 ColHeaderBg      = IM_COL32(21, 16, 31, 255);        // Header #15101F
    constexpr ImU32 ColTextMuted     = IM_COL32(155, 140, 180, 255);     // Muted Text #9B8CB4
    constexpr ImU32 ColWhite         = IM_COL32(252, 248, 255, 255);     // Clean White
}

// VECTOR ICONS (PRECISE GEOMETRY & CLEAN ARCS)
enum EIconType {
    ICON_EYE = 0,
    ICON_CROSSHAIR,
    ICON_SETTINGS,
    ICON_POWER
};

static void DrawLogoFlame(ImDrawList* dl, ImVec2 center, float size, ImU32 col) {
    float hs = size * 0.5f;
    ImVec2 pts[6] = {
        ImVec2(center.x, center.y - hs),
        ImVec2(center.x + hs * 0.90f, center.y - hs * 0.30f),
        ImVec2(center.x + hs * 0.70f, center.y + hs * 0.60f),
        ImVec2(center.x, center.y + hs),
        ImVec2(center.x - hs * 0.70f, center.y + hs * 0.60f),
        ImVec2(center.x - hs * 0.90f, center.y - hs * 0.30f)
    };
    dl->AddConvexPolyFilled(pts, 6, col);
    dl->AddPolyline(pts, 6, IM_COL32(255, 255, 255, 220), ImDrawFlags_Closed, 1.2f);
}

static void DrawVectorIcon(ImDrawList* dl, EIconType type, ImVec2 center, float size, ImU32 col) {
    float hs = size * 0.5f;
    float cx = center.x;
    float cy = center.y;

    switch (type) {
        case ICON_EYE: {
            // ESP Eye contour
            dl->AddBezierQuadratic(ImVec2(cx - hs, cy), ImVec2(cx, cy - hs * 0.85f), ImVec2(cx + hs, cy), col, 2.0f);
            dl->AddBezierQuadratic(ImVec2(cx - hs, cy), ImVec2(cx, cy + hs * 0.85f), ImVec2(cx + hs, cy), col, 2.0f);
            dl->AddCircleFilled(center, size * 0.22f, col);
            dl->AddCircleFilled(center, size * 0.08f, IM_COL32(14, 12, 20, 255));
            break;
        }
        case ICON_CROSSHAIR: {
            // Combat Crosshair
            dl->AddCircle(center, size * 0.38f, col, 16, 1.8f);
            dl->AddCircleFilled(center, 1.8f, col);
            dl->AddLine(ImVec2(cx, cy - hs), ImVec2(cx, cy - size * 0.22f), col, 1.8f);
            dl->AddLine(ImVec2(cx, cy + size * 0.22f), ImVec2(cx, cy + hs), col, 1.8f);
            dl->AddLine(ImVec2(cx - hs, cy), ImVec2(cx - size * 0.22f, cy), col, 1.8f);
            dl->AddLine(ImVec2(cx + size * 0.22f, cy), ImVec2(cx + hs, cy), col, 1.8f);
            break;
        }
        case ICON_SETTINGS: {
            // Gear icon with radial teeth
            dl->AddCircle(center, size * 0.30f, col, 16, 1.8f);
            dl->AddCircleFilled(center, size * 0.12f, col);
            for (int i = 0; i < 8; ++i) {
                float angle = i * (3.14159265f / 4.0f);
                float x1 = cx + cosf(angle) * (hs * 0.65f);
                float y1 = cy + sinf(angle) * (hs * 0.65f);
                float x2 = cx + cosf(angle) * hs;
                float y2 = cy + sinf(angle) * hs;
                dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), col, 2.0f);
            }
            break;
        }
        case ICON_POWER: {
            // Authentic Power Arc with gap at the top
            float r = size * 0.38f;
            dl->PathClear();
            dl->PathArcTo(center, r, -2.10f, 5.24f, 20); // ~ -120 deg to ~ 300 deg
            dl->PathStroke(col, 0, 2.0f);
            dl->AddLine(ImVec2(cx, cy - hs), ImVec2(cx, cy), col, 2.0f);
            break;
        }
    }
}

static bool DrawSidebarIconButton(const char* str_id, EIconType icon_type, bool is_active, ImVec2 size) {
    bool pressed = ImGui::InvisibleButton(str_id, size);
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);

    if (is_active) {
        // Active button background & subtle glowing border
        dl->AddRectFilled(bMin, bMax, IM_COL32(225, 0, 152, 48), 10.0f);
        dl->AddRect(bMin, bMax, IM_COL32(225, 0, 152, 140), 10.0f, 0, 1.2f);

        // Left vertical indicator pill (crisp 3.5px width)
        dl->AddRectFilled(ImVec2(bMin.x + 3.0f, bMin.y + 8.0f), ImVec2(bMin.x + 6.5f, bMax.y - 8.0f), theme_orchid::ColPrimary, 2.0f);

        // Glowing white icon (larger 25px scale for crisp visibility)
        DrawVectorIcon(dl, icon_type, center, 25.0f, theme_orchid::ColWhite);
    } else {
        if (hovered) {
            dl->AddRectFilled(bMin, bMax, IM_COL32(255, 255, 255, 18), 10.0f);
        }
        ImU32 ic_col = hovered ? theme_orchid::ColPrimaryLight : theme_orchid::ColTextMuted;
        // Idle icon (larger 23px scale)
        DrawVectorIcon(dl, icon_type, center, 23.0f, ic_col);
    }

    return pressed;
}

// NATIVE TOUCH-SCROLL HANDLER (100% PUBLIC IMGUI API — ZERO INTERNAL DEPENDENCY)
// Axis-locked gesture discrimination eliminates slider conflicts completely.
static void handleTouchScroll() {
    ImGuiIO& io = ImGui::GetIO();
    enum GestureState { GESTURE_IDLE, GESTURE_HORIZONTAL, GESTURE_VERTICAL };
    static GestureState s_gesture = GESTURE_IDLE;
    static ImVec2 s_touch_start = ImVec2(0, 0);
    static ImGuiID s_active_child = 0;

    ImGuiID child_id = ImGui::GetID("##TouchScrollToken");

    if (io.MouseDown[0]) {
        if (s_gesture == GESTURE_IDLE) {
            if (s_active_child == 0 && ImGui::IsWindowHovered()) {
                s_touch_start = io.MousePos;
                s_active_child = child_id;
            }
        }

        if (s_active_child == child_id) {
            float dx = io.MousePos.x - s_touch_start.x;
            float dy = io.MousePos.y - s_touch_start.y;
            float abs_dx = std::abs(dx);
            float abs_dy = std::abs(dy);

            if (s_gesture == GESTURE_IDLE) {
                // If horizontal movement exceeds 5px and dominates vertical -> lock to HORIZONTAL (Slider/Widget)
                if (abs_dx >= 5.0f && abs_dx > abs_dy * 1.1f) {
                    s_gesture = GESTURE_HORIZONTAL;
                }
                // If vertical movement exceeds 5px and dominates horizontal -> lock to VERTICAL (Scroll)
                else if (abs_dy >= 5.0f && abs_dy > abs_dx * 1.1f) {
                    s_gesture = GESTURE_VERTICAL;
                }
            }

            if (s_gesture == GESTURE_VERTICAL) {
                // Clear any active widget (slider, checkbox) so it cannot mutate during scroll
                ImGui::ClearActiveID();

                float delta_y = io.MouseDelta.y;
                if (std::abs(delta_y) > 0.01f) {
                    ImGui::SetScrollY(ImGui::GetScrollY() - delta_y);
                }
            }
            // In GESTURE_HORIZONTAL mode: ZERO scroll interference. Slider has 100% exclusive control.
        }
    } else {
        // Finger released: reset gesture state cleanly
        s_gesture = GESTURE_IDLE;
        s_active_child = 0;
    }
}

namespace ui::menu {

    void render() {
        // Render overlays on background
        func::esp::draw();
        func::aim::update();
        func::aim::render_fov(ImGui::GetBackgroundDrawList());

        ImGuiIO& io = ImGui::GetIO();

        float realW = (float)displayInfo.width;
        float realH = (float)displayInfo.height;
        if (realW <= 0) realW = 1920.0f;
        if (realH <= 0) realH = 1080.0f;

        // SCREEN ROTATION & RESOLUTION TRACKER
        // Tracks orientation flips globally across both Pill and Main Menu
        static int last_orientation = -1;
        static float last_screen_w = 0.0f;
        static float last_screen_h = 0.0f;
        static bool menu_needs_reposition = false;
        static bool pill_needs_reposition = false;

        bool screen_changed = (last_orientation != displayInfo.orientation ||
                               last_screen_w != realW ||
                               last_screen_h != realH);

        if (screen_changed) {
            last_orientation = displayInfo.orientation;
            last_screen_w = realW;
            last_screen_h = realH;
            menu_needs_reposition = true;
            pill_needs_reposition = true;
        }

        // FLOATING RE-OPEN PILL (SHOWN ONLY WHEN MENU IS MINIMIZED)
        if (!menu_open) {
            const float pillW = 136.0f;
            const float pillH = 44.0f;
            const float minX = 10.0f;
            const float maxX = (realW - pillW - 10.0f > minX) ? (realW - pillW - 10.0f) : minX;
            const float minY = 10.0f;
            const float maxY = (realH - pillH - 10.0f > minY) ? (realH - pillH - 10.0f) : minY;

            float pillInitX = (realW > 1000.0f) ? 60.0f : 30.0f;
            float pillInitY = 32.0f;

            static ImVec2 s_pill_pos = ImVec2(-1, -1);
            if (s_pill_pos.x < 0.0f || pill_needs_reposition) {
                s_pill_pos = ImVec2(pillInitX, pillInitY);
                pill_needs_reposition = false;
            }

            // Continuous hard clamp within screen bounds
            if (s_pill_pos.x < minX) s_pill_pos.x = minX;
            if (s_pill_pos.x > maxX) s_pill_pos.x = maxX;
            if (s_pill_pos.y < minY) s_pill_pos.y = minY;
            if (s_pill_pos.y > maxY) s_pill_pos.y = maxY;

            ImGui::SetNextWindowPos(s_pill_pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(pillW, pillH));

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.063f, 0.122f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.882f, 0.000f, 0.596f, 0.90f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 22.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));

            ImGuiWindowFlags pillFlags = ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoMove;

            if (ImGui::Begin("##NIN_MINI", nullptr, pillFlags)) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                bool clicked = ImGui::InvisibleButton("##ReOpenPill", avail);

                static bool pill_was_dragged = false;

                // Dragging logic with instant delta update and boundary clamping
                if (ImGui::IsItemActive()) {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
                        pill_was_dragged = true;
                        s_pill_pos.x += io.MouseDelta.x;
                        s_pill_pos.y += io.MouseDelta.y;

                        if (s_pill_pos.x < minX) s_pill_pos.x = minX;
                        if (s_pill_pos.x > maxX) s_pill_pos.x = maxX;
                        if (s_pill_pos.y < minY) s_pill_pos.y = minY;
                        if (s_pill_pos.y > maxY) s_pill_pos.y = maxY;
                    }
                }

                // Strict separation: Drag release NEVER triggers Click (eliminates double-trigger)
                if (io.MouseReleased[0]) {
                    if (pill_was_dragged) {
                        pill_was_dragged = false; // Drag completed; suppress menu opening
                    } else if (clicked) {
                        menu_open = true; // Clean single tap; open menu
                    }
                } else if (!io.MouseDown[0] && pill_was_dragged) {
                    pill_was_dragged = false;
                }

                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 p0 = ImGui::GetItemRectMin();
                ImVec2 p1 = ImGui::GetItemRectMax();
                float cy = (p0.y + p1.y) * 0.5f;

                DrawLogoFlame(dl, ImVec2(p0.x + 20, cy), 16.0f, theme_orchid::ColPrimary);
                dl->AddText(ImVec2(p0.x + 36, cy - 8), theme_orchid::ColWhite, "NINMODS");
            }
            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
            return;
        }

        // MAIN MENU: DESIGN 08 (ELECTRIC ORCHID GLASS)
        // Expanded horizontal width so labels have generous breathing space
        const float winW = ImMin(820.0f, realW * 0.94f);
        const float winH = ImMin(460.0f, realH * 0.94f);

        if (menu_needs_reposition) {
            // Re-center and resize cleanly upon screen rotation
            ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
            ImGui::SetNextWindowPos(
                ImVec2(realW * 0.5f, realH * 0.5f),
                ImGuiCond_Always,
                ImVec2(0.5f, 0.5f)
            );
            menu_needs_reposition = false;
        } else {
            ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(
                ImVec2(realW * 0.5f, realH * 0.5f),
                ImGuiCond_FirstUseEver,
                ImVec2(0.5f, 0.5f)
            );
        }

        // Precise styling for Electric Orchid Glass + Complete Scrollbar Hiding
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.047f, 0.078f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.882f, 0.000f, 0.596f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.110f, 0.086f, 0.157f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.160f, 0.120f, 0.220f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.220f, 0.160f, 0.300f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.882f, 0.000f, 0.596f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.882f, 0.000f, 0.596f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.000f, 0.353f, 0.804f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.110f, 0.086f, 0.157f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.200f, 0.140f, 0.280f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.882f, 0.000f, 0.596f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.882f, 0.000f, 0.596f, 0.40f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.882f, 0.000f, 0.596f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.882f, 0.000f, 0.596f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.988f, 0.973f, 1.000f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.608f, 0.549f, 0.706f, 1.0f));

        // Hide scrollbars completely (always hidden, but touch dragging works!)
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0, 0, 0, 0));

        // Window-scoped Alpha & Zero Scrollbar Size
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ui_opacity);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f); // Zero scrollbar size allocates 0px margin!

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("##NIN_MAIN_PANEL", nullptr, winFlags)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();

            // 1. TOP HEADER (44px)
            const float headerH = 44.0f;
            ImVec2 headerP0 = winPos;
            ImVec2 headerP1 = ImVec2(winPos.x + winSize.x, winPos.y + headerH);

            // Header Background & Underline
            dl->AddRectFilled(headerP0, headerP1, theme_orchid::ColHeaderBg, 12.0f, ImDrawFlags_RoundCornersTop);
            dl->AddLine(ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(54, 38, 76, 255), 1.0f);

            // Header BAM PASE: Logo + Name "NINMODS"
            float logoX = winPos.x + 22.0f;
            float headerCY = winPos.y + headerH * 0.5f;
            DrawLogoFlame(dl, ImVec2(logoX, headerCY), 18.0f, theme_orchid::ColPrimary);
            dl->AddText(ImVec2(logoX + 16.0f, headerCY - 9.0f), theme_orchid::ColWhite, "NINMODS");

            // Header DAN PASE: CLOSE ICON BUTTON (JUST BUTTON, NO BACKGROUND, NO BORDER!)
            const float closeBtnSize = 36.0f;
            ImVec2 closePos = ImVec2(winPos.x + winSize.x - closeBtnSize - 8.0f, winPos.y + (headerH - closeBtnSize) * 0.5f);
            ImGui::SetCursorScreenPos(closePos);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

            if (ImGui::Button("##CloseBtn", ImVec2(closeBtnSize, closeBtnSize))) {
                menu_open = false; // Pure close action
            }
            bool closeHovered = ImGui::IsItemHovered();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            // Draw Clean Close Icon (X) centered in touch target
            ImVec2 closeCenter = ImVec2(closePos.x + closeBtnSize * 0.5f, closePos.y + closeBtnSize * 0.5f);
            ImU32 closeCol = closeHovered ? theme_orchid::ColPrimaryLight : IM_COL32(200, 195, 215, 230);
            const float xRadius = 6.0f;
            dl->AddLine(ImVec2(closeCenter.x - xRadius, closeCenter.y - xRadius), ImVec2(closeCenter.x + xRadius, closeCenter.y + xRadius), closeCol, 2.2f);
            dl->AddLine(ImVec2(closeCenter.x - xRadius, closeCenter.y + xRadius), ImVec2(closeCenter.x + xRadius, closeCenter.y - xRadius), closeCol, 2.2f);

            // 2. MAIN BODY: LEFT SIDEBAR (ICON-ONLY) + RIGHT CONTENT AREA
            const float bodyY = headerH;
            const float bodyH = winSize.y - headerH;
            const float sidebarW = 76.0f;

            // SIDEBAR CONTAINER
            ImGui::SetCursorPos(ImVec2(0, bodyY));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.067f, 0.055f, 0.094f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            if (ImGui::BeginChild("##Sidebar", ImVec2(sidebarW, bodyH), false, ImGuiWindowFlags_NoScrollbar)) {
                // Divider line between sidebar & content
                dl->AddLine(ImVec2(winPos.x + sidebarW, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(40, 30, 56, 255), 1.0f);

                const ImVec2 btnSize = ImVec2(56.0f, 52.0f);
                const float btnX = (sidebarW - btnSize.x) * 0.5f; // Exactly 10.0f -> centered between 0 and 76

                // Tab 0: ESP (Eye Icon)
                ImGui::SetCursorPos(ImVec2(btnX, 12.0f));
                if (DrawSidebarIconButton("##TabESP", ICON_EYE, (activeTab == 0), btnSize)) {
                    activeTab = 0;
                }

                // Tab 1: Combat / Aim (Crosshair Icon)
                ImGui::SetCursorPos(ImVec2(btnX, 12.0f + btnSize.y + 8.0f));
                if (DrawSidebarIconButton("##TabCombat", ICON_CROSSHAIR, (activeTab == 1), btnSize)) {
                    activeTab = 1;
                }

                // Tab 2: Settings (Gear Icon)
                ImGui::SetCursorPos(ImVec2(btnX, 12.0f + (btnSize.y + 8.0f) * 2.0f));
                if (DrawSidebarIconButton("##TabSettings", ICON_SETTINGS, (activeTab == 2), btnSize)) {
                    activeTab = 2;
                }

                // Bottom Safe Exit Icon (Anchored cleanly to bottom with equal margin)
                ImGui::SetCursorPos(ImVec2(btnX, bodyH - btnSize.y - 12.0f));
                if (DrawSidebarIconButton("##TabPower", ICON_POWER, false, btnSize)) {
                    g_should_exit = true; // Signal exit cleanly outside ImGui frame
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            // 3. MAIN CONTENT AREA (EXPANDED DUAL-COLUMN GEOMETRY)
            const float contentX = sidebarW + 14.0f;
            const float contentY = headerH + 10.0f;
            const float contentW = winSize.x - contentX - 14.0f;
            const float contentH = bodyH - 20.0f;
            const float colGap   = 14.0f;
            const float colW     = (contentW - colGap) * 0.5f;

            // Card background styling
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.086f, 0.067f, 0.130f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.196f, 0.141f, 0.282f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 10));

            // TAB 0: ESP VISUALS (DUAL COLUMN: BAM PASE + DAN PASE)
            if (activeTab == 0) {
// LEFT COLUMN CARD (BAM PASE OPTION): ESP VISUALS
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftESP", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "ESP VISUALS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 85, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("ESP Box (Chaturbhuj)", &cfg::esp::box);
                    ImGui::Checkbox("ESP Health Bar (Right)", &cfg::esp::health);
                    ImGui::Checkbox("ESP Armor Bar", &cfg::esp::armor);
                    ImGui::Checkbox("ESP Laser Line", &cfg::esp::tracer);
                    ImGui::Checkbox("ESP Player Name", &cfg::esp::name);
                    ImGui::Checkbox("ESP Distance", &cfg::esp::distance);
                    ImGui::Checkbox("ESP Bot Tag (AI)", &cfg::esp::bot_tag);
                    ImGui::Checkbox("ESP Weapon Name", &cfg::esp::weapon);
                    ImGui::Checkbox("Firing Indicator", &cfg::esp::firing_ind);

                    handleTouchScroll();
                }
                ImGui::EndChild();

// RIGHT COLUMN CARD (DAN PASE OPTION): FILTERS & CONTROLS
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightESP", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "SETTINGS & FILTERS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 120, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Ignore Bot Players", &cfg::esp::ignore_bots);
                    ImGui::Checkbox("Crosshair Center", &cfg::esp::crosshair);
                    ImGui::Checkbox("Loot ESP", &cfg::esp::loot);

                    ImGui::Dummy(ImVec2(0, 4));
                    const char* boxTypes[] = { "Corner", "Full Chaturbhuj", "Glow" };
                    ImGui::SetNextItemWidth(-1);
                    ImGui::Combo("##BoxStyle", &cfg::esp::box_type, boxTypes, 3);
                    ImGui::TextDisabled("Box Style Mode");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##MaxRange", &cfg::esp::max_range, 50.0f, 500.0f, "%.0f m");
                    ImGui::TextDisabled("Max Render Distance");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##LaserThickness", &cfg::esp::tracer_thickness, 0.5f, 4.0f, "%.1f px");
                    ImGui::TextDisabled("Laser Line Width");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##BoxThickness", &cfg::esp::box_thickness, 1.0f, 5.0f, "%.1f px");
                    ImGui::TextDisabled("Box Border Width");

                    handleTouchScroll();
                }
                ImGui::EndChild();
            }
            // TAB 1: COMBAT & AIM ASSIST (DUAL COLUMN)
            else if (activeTab == 1) {
                // Left Column: Aim Controls & Target Selectors
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftCombat", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "AIMBOT SETTINGS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 120, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Enable Aim Assist", &cfg::aim::enable);
                    ImGui::Checkbox("Scope Headshot (Instant Head)", &cfg::aim::scope_aim);

                    ImGui::Dummy(ImVec2(0, 4));
                    const char* triggerModes[] = { "When Firing", "Always In FOV" };
                    ImGui::SetNextItemWidth(-1);
                    ImGui::Combo("##AimTrigger", &cfg::aim::trigger_mode, triggerModes, 2);
                    ImGui::TextDisabled("Aim Trigger Condition");

                    ImGui::Dummy(ImVec2(0, 4));
                    const char* boneTargets[] = { "Head (Auto Drag)", "Neck / Upper Chest", "Pelvis / Body" };
                    ImGui::SetNextItemWidth(-1);
                    ImGui::Combo("##AimBone", &cfg::aim::target_bone, boneTargets, 3);
                    ImGui::TextDisabled("Target Aim Bone");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##HeadshotRate", &cfg::aim::headshot_rate, 0.0f, 100.0f, "%.0f%%");
                    ImGui::TextDisabled("Headshot Rate Percentage (0%% - 100%%)");

                    ImGui::Dummy(ImVec2(0, 4));
                    const char* priorityModes[] = { "Crosshair Distance", "Lowest Health", "3D Distance" };
                    ImGui::SetNextItemWidth(-1);
                    ImGui::Combo("##AimPriority", &cfg::aim::priority_mode, priorityModes, 3);
                    ImGui::TextDisabled("Target Selection Priority");

                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "SAFETY FILTERS");
                    lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 100, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Ignore Knocked Enemies", &cfg::aim::ignore_knocked);
                    ImGui::Checkbox("Ignore AI Bot Players", &cfg::aim::ignore_bots);
                    ImGui::Checkbox("Check Visibility Only", &cfg::aim::visible_check);

                    handleTouchScroll();
                }
                ImGui::EndChild();

                // Right Column: FOV & Smooth Speed & Live Lock HUD
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightCombat", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "FOV & SMOOTHNESS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 125, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Draw FOV Circle", &cfg::aim::show_fov);

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##FovRadius", &cfg::aim::fov_radius, 40.0f, 450.0f, "%.0f px");
                    ImGui::TextDisabled("Aim FOV Circle Radius");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##AimSmooth", &cfg::aim::smooth, 1.0f, 25.0f, "%.1f (Smooth)");
                    ImGui::TextDisabled("Smooth Factor (Higher = Safe & Human)");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##AimMaxDist", &cfg::aim::max_distance, 20.0f, 300.0f, "%.0f m");
                    ImGui::TextDisabled("Max Aiming Range");

                    ImGui::Dummy(ImVec2(0, 10));
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "LIVE TARGET HUD");
                    rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 105, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    if (func::aim::g_currentTarget.valid) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Target: %s", func::aim::g_currentTarget.name);
                        ImGui::Text("Distance: %.1f m", func::aim::g_currentTarget.distance);
                        ImGui::Text("Health: %d HP", func::aim::g_currentTarget.hp);
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.6f, 1.0f), "Status: LOCKED [IN FOV]");
                    } else {
                        ImGui::TextDisabled("Target: None (Scanning)");
                        ImGui::TextDisabled("Distance: -- m");
                        ImGui::TextDisabled("Health: --");
                        ImGui::TextDisabled("Status: STANDBY");
                    }

                    handleTouchScroll();
                }
                ImGui::EndChild();
            }
            // TAB 2: SETTINGS & SYSTEM STATUS (DUAL COLUMN)
            else if (activeTab == 2) {
                // Left Column: UI Customization
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftSettings", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "UI APPEARANCE");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 105, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::SliderFloat("##Opacity", &ui_opacity, 0.2f, 1.0f, "Opacity: %.2f");

                    ImGui::Dummy(ImVec2(0, 12));
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Engine Architecture:");
                    ImGui::Text("Kernel DMA / RPM Memory");
                    ImGui::Text("100% Non-Invasive");
                    ImGui::Text("Screen Record: Unblocked");
                    ImGui::Text("Screenshot: Allowed");

                    handleTouchScroll();
                }
                ImGui::EndChild();

                // Right Column: Live Process Diagnostics
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightSettings", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "LIVE DIAGNOSTICS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 115, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Text("Target PID: %d", static_cast<int>(proc::pid));

                    // Separate clean format if/else without ternary format mismatches
                    if (proc::lib) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "libil2cpp: 0x%lx", (unsigned long)proc::lib);
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Waiting for Game...");
                    }

                    ImGui::Text("Overlay FPS: %.1f", io.Framerate);
                    ImGui::Text("Screen: %.0f x %.0f", realW, realH);

                    ImGui::Dummy(ImVec2(0, 10));
                    if (ImGui::Button("SAFE EXIT APPLICATION", ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
                        g_should_exit = true; // Signal exit cleanly outside ImGui frame
                    }

                    handleTouchScroll();
                }
                ImGui::EndChild();
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }
        ImGui::End();

        ImGui::PopStyleVar(7);
        ImGui::PopStyleColor(20);

        // DEFERRED SAFE SHUTDOWN (CALLED AFTER ALL IMGUI FRAMES UNWOUND)
        if (g_should_exit) {
            main_thread_flag = false;
            Touch_Close();
            data::thread_stop();
            shutdown();
            exit(0);
        }
    }
}
