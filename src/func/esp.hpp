#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "../game/data.hpp"
#include "../ui/cfg.hpp"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cmath>

extern float g_sw, g_sh;

namespace func::esp {

    // HELPER COLOR
    inline ImU32 toU32(const ImVec4& c, float alphaMul = 1.f) {
        return IM_COL32(
            (int)(c.x * 255.f),
            (int)(c.y * 255.f),
            (int)(c.z * 255.f),
            (int)(c.w * 255.f * alphaMul));
    }

    inline ImU32 withAlpha(ImU32 c, float mul) {
        int a = (int)(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | ((ImU32)a << 24);
    }

    inline ImU32 fadeDist(ImU32 base, float dist, float maxRange) {
        float t = ImClamp(1.f - dist / maxRange, 0.35f, 1.f);
        return withAlpha(base, t);
    }

    inline ImU32 hpColor(float t) {
        t = ImClamp(t, 0.f, 1.f);
        float r, g, b;
        if (t < 0.5f) { float k = t * 2.f; r = 1.f; g = 0.35f + 0.55f * k; b = 0.15f; }
        else          { float k = (t - 0.5f) * 2.f; r = 1.f - 0.9f * k; g = 0.9f; b = 0.15f + 0.30f * k; }
        return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
    }

    inline ImU32 armorColor(float t) {
        t = ImClamp(t, 0.f, 1.f);
        return IM_COL32((int)(80 + 100 * t), (int)(180 + 60 * t), 255, 255);
    }

    // FORMAT
    inline bool ok(const Vector3& v) { return v.x > 0 && v.y > 0; }

    inline void fmtDist(float d, char* out, size_t n) {
        if (d >= 1000.f) snprintf(out, n, "%.1fkm", d / 1000.f);
        else             snprintf(out, n, "%.0fm", d);
    }

    inline void truncName(const char* src, char* out, size_t n) {
        if (!src || !src[0]) { out[0] = 0; return; }
        size_t len = strlen(src);
        if (len <= 12) { snprintf(out, n, "%s", src); return; }
        snprintf(out, n, "%.11s.", src);
    }

    // CONSTANTS
    static constexpr ImU32 C_SHADOW = IM_COL32(0, 0, 0, 180);
    static constexpr ImU32 C_BORDER = IM_COL32(255, 255, 255, 40);

// FONT SIZE (kecil, seimbang)
    static constexpr float ESP_FONT_SIZE = 12.f;
    static constexpr float PANEL_ROW_H   = ESP_FONT_SIZE + 5.f;   // 17px

// PANEL OFFSET DARI KEPALA (jauh di atas)
    static constexpr float PANEL_TOP_OFFSET = 35.f;

    // PLAYER BOX — compute bounding box from Head and Toe
    struct PlayerBox {
        ImVec2 tl, br;
        bool   valid;
    };

    inline PlayerBox computePlayerBox(const PlayerData& p) {
        PlayerBox pb{ {0,0}, {0,0}, false };
        if (!ok(p.Head) || !ok(p.Toe)) return pb;

        float topY = ImMin(p.Head.y, p.Toe.y);
        float botY = ImMax(p.Head.y, p.Toe.y);
        float height = botY - topY;
        if (height < 2.f) return pb;

        float midX = (p.Head.x + p.Toe.x) * 0.5f;
        float width = height * 0.45f;
        float halfW = width * 0.5f;

        const float padY = height * 0.08f;

        pb.tl = ImVec2(midX - halfW, topY - padY);
        pb.br = ImVec2(midX + halfW, botY + padY * 0.5f);
        pb.valid = true;
        return pb;
    }

    // STYLE 09: TOP CLEAN NAME & DISTANCE (NO OVERLAP WITH TRACER)
    inline void drawTopInfo(ImDrawList* dl, const PlayerData& p,
                            float cx, float top_y) {
        if (!cfg::esp::name && !cfg::esp::distance) return;

        char nameBuf[24];
        truncName(p.name, nameBuf, sizeof(nameBuf));
        if (nameBuf[0] == 0)
            snprintf(nameBuf, sizeof(nameBuf), "%s", p.isBot ? "BOT" : "Player");

        char distBuf[16];
        fmtDist(p.distance, distBuf, sizeof(distBuf));

        const char* tag = cfg::esp::bot_tag ? (p.isBot ? "AI " : "") : "";
        char fullText[64];
        if (cfg::esp::name && cfg::esp::distance) {
            snprintf(fullText, sizeof(fullText), "%s%s  %s", tag, nameBuf, distBuf);
        } else if (cfg::esp::name) {
            snprintf(fullText, sizeof(fullText), "%s%s", tag, nameBuf);
        } else {
            snprintf(fullText, sizeof(fullText), "%s", distBuf);
        }

        const float fs = ESP_FONT_SIZE + 1.f;
        ImVec2 ts = ImGui::CalcTextSize(fullText);
        float tx = cx - ts.x * 0.5f;
        float ty = top_y - ts.y - 8.f;

        // Shadow outline for crystal clear readability on any background
        dl->AddText(ImGui::GetFont(), fs, {tx + 1.f, ty + 1.f}, IM_COL32(0, 0, 0, 240), fullText);
        dl->AddText(ImGui::GetFont(), fs, {tx - 1.f, ty}, IM_COL32(0, 0, 0, 180), fullText);
        dl->AddText(ImGui::GetFont(), fs, {tx + 1.f, ty}, IM_COL32(0, 0, 0, 180), fullText);
        dl->AddText(ImGui::GetFont(), fs, {tx, ty - 1.f}, IM_COL32(0, 0, 0, 180), fullText);
        dl->AddText(ImGui::GetFont(), fs, {tx, ty + 1.f}, IM_COL32(0, 0, 0, 180), fullText);

        float curX = tx;
        if (tag[0] != 0) {
            ImVec2 tagSize = ImGui::CalcTextSize(tag);
            ImU32 tagCol = p.isBot ? IM_COL32(255, 75, 75, 255) : IM_COL32(0, 200, 255, 255);
            dl->AddText(ImGui::GetFont(), fs, {curX, ty}, tagCol, tag);
            curX += tagSize.x;
        }

        if (cfg::esp::name) {
            ImVec2 nSize = ImGui::CalcTextSize(nameBuf);
            ImU32 nCol = p.isVisible ? toU32(cfg::esp::name_col) : withAlpha(toU32(cfg::esp::name_col), 0.75f);
            dl->AddText(ImGui::GetFont(), fs, {curX, ty}, nCol, nameBuf);
            curX += nSize.x;
        }

        if (cfg::esp::distance) {
            if (cfg::esp::name) curX += ImGui::CalcTextSize("  ").x;
            ImU32 dCol = p.isVisible ? toU32(cfg::esp::distance_col) : withAlpha(toU32(cfg::esp::distance_col), 0.75f);
            dl->AddText(ImGui::GetFont(), fs, {curX, ty}, dCol, distBuf);
        }
    }

    // STYLE 09: VERTICAL (LOMBA) HEALTH BAR ON RIGHT SIDE
    inline void drawSideHealthBar(ImDrawList* dl, const PlayerData& p, ImVec2 tl, ImVec2 br) {
        if (!cfg::esp::health && !cfg::esp::armor) return;

        float boxH = br.y - tl.y;
        if (boxH <= 4.f) return;

        // Positioned on the RIGHT side of the chaturbhuj box (Style 09)
        float barX = br.x + 5.f;
        float barW = 4.f;

        if (cfg::esp::health) {
            float hp01 = p.maxHP > 0 ? (float)p.curHP / (float)p.maxHP : 0.f;
            hp01 = ImClamp(hp01, 0.f, 1.f);

            // Background track + border
            dl->AddRectFilled({barX - 1.f, tl.y - 1.f}, {barX + barW + 1.f, br.y + 1.f},
                              IM_COL32(10, 14, 20, 220), 1.5f);
            dl->AddRect({barX - 1.f, tl.y - 1.f}, {barX + barW + 1.f, br.y + 1.f},
                        IM_COL32(0, 0, 0, 180), 1.5f, 0, 1.f);

            // Health fill (bottom up with dynamic color)
            float fillH = boxH * hp01;
            dl->AddRectFilled({barX, br.y - fillH}, {barX + barW, br.y},
                              hpColor(hp01), 1.0f);
        }

        if (cfg::esp::armor && p.curArmor > 0) {
            float armX = barX + (cfg::esp::health ? (barW + 3.f) : 0.f);
            float armW = 2.5f;
            float arm01 = ImClamp((float)p.curArmor / 100.f, 0.f, 1.f);
            float fillArm = boxH * arm01;

            dl->AddRectFilled({armX - 1.f, tl.y - 1.f}, {armX + armW + 1.f, br.y + 1.f},
                              IM_COL32(10, 14, 20, 220), 1.f);
            dl->AddRect({armX - 1.f, tl.y - 1.f}, {armX + armW + 1.f, br.y + 1.f},
                        IM_COL32(0, 0, 0, 180), 1.f, 0, 1.f);
            dl->AddRectFilled({armX, br.y - fillArm}, {armX + armW, br.y},
                              armorColor(arm01), 1.0f);
        }
    }

    // BOX
    inline void drawBox(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            float len = (br.x - tl.x) * cfg::esp::bracket_frac;
            ImVec2 pts[8][2] = {
                {tl,                 {tl.x + len, tl.y}},
                {tl,                 {tl.x, tl.y + len}},
                {{br.x - len, tl.y}, {br.x, tl.y}},
                {{br.x, tl.y},       {br.x, tl.y + len}},
                {{tl.x, br.y - len}, {tl.x, br.y}},
                {{tl.x, br.y},       {tl.x + len, br.y}},
                {{br.x - len, br.y}, {br.x, br.y}},
                {{br.x, br.y - len}, {br.x, br.y}},
            };
            for (auto& s : pts) {
                dl->AddLine(s[0], s[1], withAlpha(col, 0.25f), thick + 3.f);
                dl->AddLine({s[0].x + 1, s[0].y + 1},
                            {s[1].x + 1, s[1].y + 1}, C_SHADOW, thick + 1.f);
                dl->AddLine(s[0], s[1], col, thick);
            }
        } else if (type == 1) {
            // Full Chaturbhuj (Rectangular) Box: Outer shadow, crisp border, subtle inner fill
            dl->AddRect({tl.x - 1.f, tl.y - 1.f}, {br.x + 1.f, br.y + 1.f},
                        IM_COL32(0, 0, 0, 180), round, 0, thick + 1.5f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, withAlpha(col, 0.05f), round);
        } else {
            dl->AddRect(tl, br, withAlpha(col, 0.18f), round, 0, thick + 5.f);
            dl->AddRect(tl, br, withAlpha(col, 0.45f), round, 0, thick + 2.f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, withAlpha(col, 0.06f), round);
        }
    }


    // FIRING
    inline void drawFiring(ImDrawList* dl, const PlayerData& p, float cx, float ty) {
        if (!p.isFiring) return;
        float t = ImGui::GetTime();
        float pulse = 0.5f + 0.5f * sinf(t * 12.f);
        float r = 3.f + pulse * 1.2f;
        ImVec2 pos(cx, ty - 20.f);

        dl->AddCircleFilled(pos, r + 4.f,
            IM_COL32(255, 200, 40, (int)(70 * pulse)));
        dl->AddCircleFilled(pos, r, IM_COL32(255, 200, 40, 255));
        dl->AddCircle(pos, r + 1.f, IM_COL32(255, 255, 255, 210), 16, 1.2f);
    }

    // WEAPON PILL
    inline void drawWeapon(ImDrawList* dl, const PlayerData& p, ImVec2 foot) {
        if (p.weaponType < 0) return;
        static const char* kNames[] = {
            "AR","SNIPER","SMG","SHOTGUN","PISTOL","MELEE",
            "GRENADE","LMG","RIFLE","MARKSMAN","SPECIAL"
        };
        const char* wn = (p.weaponType < (int)(sizeof(kNames)/sizeof(*kNames)))
                         ? kNames[p.weaponType] : "WPN";

        const float fs   = ESP_FONT_SIZE - 2.f;
        const float padX = 6.f;
        const float padY = 3.f;

        ImVec2 ts = ImGui::CalcTextSize(wn);
        float pw = ts.x + padX * 2.f;
        float ph = fs + padY * 2.f;
        float px = foot.x - pw * 0.5f;
        float py = foot.y + 4.f;

        ImU32 wCol = toU32(cfg::esp::weapon_col);
        ImU32 bg   = p.isVisible ? IM_COL32(0, 0, 0, 210) : IM_COL32(0, 0, 0, 150);

        dl->AddRectFilled({px + 1, py + 1}, {px + pw + 1, py + ph + 1},
                          C_SHADOW, 5.f);
        dl->AddRectFilled({px, py}, {px + pw, py + ph}, bg, 5.f);
        dl->AddRect({px, py}, {px + pw, py + ph}, C_BORDER, 5.f, 0, 1.f);
        dl->AddText(ImGui::GetFont(), fs,
            {px + padX, py + padY}, wCol, wn);
    }

    // LOOT
    inline const char* lootCat(uint32_t resID) {
        uint32_t c = (resID >> 24) & 0xFF;
        switch (c) {
            case 0x01: return "GUN";
            case 0x02: return "AMMO";
            case 0x03: return "ARMOR";
            case 0x04: return "HEAL";
            case 0x05: return "NADE";
            case 0x06: return "BAG";
            default:   return "ITEM";
        }
    }

    inline ImU32 lootColor(const char* cat) {
        if (strcmp(cat, "GUN")   == 0) return IM_COL32(255, 100, 100, 255);
        if (strcmp(cat, "AMMO")  == 0) return IM_COL32(255, 200, 80, 255);
        if (strcmp(cat, "ARMOR") == 0) return IM_COL32(80, 180, 255, 255);
        if (strcmp(cat, "HEAL")  == 0) return IM_COL32(80, 255, 120, 255);
        if (strcmp(cat, "NADE")  == 0) return IM_COL32(200, 80, 255, 255);
        if (strcmp(cat, "BAG")   == 0) return IM_COL32(160, 120, 60, 255);
        return IM_COL32(200, 200, 200, 255);
    }

    inline void drawLootLabel(ImDrawList* dl, const LootItem& li) {
        const char* cat = lootCat(li.resID);
        ImU32 col = lootColor(cat);

        char buf[64];
        snprintf(buf, sizeof(buf), "%s  %.0fm", cat, li.distance);

        ImVec2 ts = ImGui::CalcTextSize(buf);
        float pad = 4.f;
        float px = li.screenPos.x - ts.x * 0.5f - pad;
        float py = li.screenPos.y;

        dl->AddRectFilled({px + 1, py + 1},
                          {px + ts.x + pad * 2 + 1, py + ts.y + pad * 2 + 1},
                          C_SHADOW, 4.f);
        dl->AddRectFilled({px, py},
                          {px + ts.x + pad * 2, py + ts.y + pad * 2},
                          IM_COL32(15, 15, 20, 220), 4.f);
        dl->AddRect({px, py},
                    {px + ts.x + pad * 2, py + ts.y + pad * 2},
                    col, 4.f, 0, 1.5f);

        dl->AddText(ImGui::GetFont(), ESP_FONT_SIZE - 2.f,
                    {px + pad, py + pad}, col, buf);
        dl->AddCircleFilled({li.screenPos.x, li.screenPos.y}, 3.f, col);
    }

    inline void drawLoot() {
        if (!data::inMatch.load()) return;
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<LootItem> snap;
        {
            std::lock_guard<std::mutex> lk(data::loots_mutex);
            snap = data::loots;
        }

        for (const auto& li : snap) {
            if (li.screenPos.x < 0 || li.screenPos.y < 0) continue;
            if (li.screenPos.x > g_sw || li.screenPos.y > g_sh) continue;
            drawLootLabel(dl, li);
        }
    }

    // CROSSHAIR
    inline void drawCrosshair(ImDrawList* dl) {
        float cx = g_sw * 0.5f, cy = g_sh * 0.5f;
        float gap = 6.f, len = 10.f;
        ImU32 c = IM_COL32(220, 40, 40, 240);

        dl->AddLine({cx - gap - len, cy}, {cx - gap, cy}, c, 1.8f);
        dl->AddLine({cx + gap, cy}, {cx + gap + len, cy}, c, 1.8f);
        dl->AddLine({cx, cy - gap - len}, {cx, cy - gap}, c, 1.8f);
        dl->AddLine({cx, cy + gap}, {cx, cy + gap + len}, c, 1.8f);
        dl->AddCircleFilled({cx, cy}, 1.5f, c);
    }

    // MAIN DRAW
    inline void draw() {
        if (!data::inMatch.load()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<PlayerData> snap;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            snap = data::players;
        }

        ImVec2 anchor(g_sw * 0.5f, 0.f);

        for (const auto& p : snap) {
            if (p.Head.z < 0.5f || p.Toe.z < 0.5f) continue;
            if (p.curHP <= 0) continue;
            if (cfg::esp::ignore_bots && p.isBot) continue;

            if (p.distance > cfg::esp::max_range) continue;

            PlayerBox pb = computePlayerBox(p);
            if (!pb.valid) continue;

            ImVec2 tl = pb.tl;
            ImVec2 br = pb.br;

            if (br.x < -30.f || tl.x > g_sw + 30.f) continue;
            if (br.y < -30.f || tl.y > g_sh + 30.f) continue;

            float cx = (tl.x + br.x) * 0.5f;

            ImU32 colBase = p.isVisible ? toU32(cfg::esp::box_visible)
                                        : toU32(cfg::esp::box_col);
            colBase = fadeDist(colBase, p.distance, cfg::esp::max_range);

            if (cfg::esp::tracer) {
                // Strictly draw tracer ONLY when head point is within visible screen bounds
                if (p.Head.x >= 0.f && p.Head.x <= g_sw &&
                    p.Head.y >= 0.f && p.Head.y <= g_sh) {
                    ImVec2 head(p.Head.x, p.Head.y);
                    float aa = ImClamp(1.f - p.distance / cfg::esp::max_range, 0.25f, 1.f);
                    float tThick = cfg::esp::tracer_thickness;
                    // Sleek dynamic laser line with soft glow
                    dl->AddLine(anchor, head,
                        withAlpha(toU32(cfg::esp::box_col), 0.25f * aa), tThick + 1.4f);
                    dl->AddLine(anchor, head,
                        withAlpha(toU32(cfg::esp::box_col), 0.95f * aa), tThick);
                }
            }

            if (cfg::esp::box)
                drawBox(dl, tl, br, colBase, cfg::esp::box_type);

            if (cfg::esp::firing_ind)
                drawFiring(dl, p, cx, tl.y);

            // Style 09: Clean top info + Right side vertical health bar
            PlayerData tmp = p;
            if (tmp.name[0] == 0)
                snprintf(tmp.name, sizeof(tmp.name), "%s",
                         p.isBot ? "BOT" : "Player");

            drawTopInfo(dl, tmp, cx, tl.y);
            drawSideHealthBar(dl, tmp, tl, br);

            if (cfg::esp::weapon)
                drawWeapon(dl, p, {cx, br.y});
        }

        if (cfg::esp::loot)      drawLoot();
        if (cfg::esp::crosshair) drawCrosshair(dl);
    }
}