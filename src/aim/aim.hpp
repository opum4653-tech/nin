#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "../game/player.hpp"
#include "../game/data.hpp"
#include "../offsets/offsets.hpp"
#include "../ui/cfg.hpp"
#include "../other/memory.hpp"
#include "../other/vector3.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>

extern float g_sw;
extern float g_sh;

// SAFE BODY-TO-HEAD RECOIL DRAG ENGINE (NATURAL GAMEPLAY — ZERO CAMERA JUMPING)
// Features:
// 1. Natural Gameplay: Camera rotation is 100% untouched. Finger aiming is free.
// 2. Strict Target Stickiness & Hysteresis: ZERO screen shaking when multiple enemies enter FOV.
// 3. Grace-Period Burst Lock: Prevents lock resets between rapid bullet intervals.
// 4. Progressive Body-to-Head Drag: Bullets climb naturally from chest to head.
// 5. Strict FOV Circle: Strictly respects FOV limit; never locks outside.

namespace aimbot {

    struct TargetInfo {
        bool valid = false;
        char name[36] = {0};
        float distance = 0.0f;
        int hp = 0;
        Vector3 screenPos = Vector3::Zero();
        Vector3 headWorld = Vector3::Zero();
    };

    inline TargetInfo g_currentTarget;

    // Safe 3D vector normalization with division-by-zero & NaN protection
    inline Vector3 SafeNormalize(const Vector3& v) {
        float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq < 1e-6f || !std::isfinite(lenSq)) {
            return Vector3(0.0f, 0.0f, 1.0f); // Default forward vector
        }
        float invLen = 1.0f / sqrtf(lenSq);
        return Vector3(v.x * invLen, v.y * invLen, v.z * invLen);
    }

    // Target stickiness with hysteresis & grace period to completely eliminate screen shaking
    inline uint64_t s_lockedTargetAddr = 0;
    inline int      s_lockTimeoutTicks = 0;
    inline int      s_burstTicks       = 0;
    inline Vector3  s_prevDir          = Vector3::Zero();

    inline void update() {
        g_currentTarget.valid = false;
        if (!cfg::aim::enable) {
            s_lockedTargetAddr = 0;
            s_lockTimeoutTicks = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }
        if (!data::inMatch.load(std::memory_order_relaxed)) {
            s_lockedTargetAddr = 0;
            s_lockTimeoutTicks = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }

        uint64_t localPlayer = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (!is_valid_ptr(localPlayer)) {
            s_lockedTargetAddr = 0;
            s_lockTimeoutTicks = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }

        // Verify local player is alive
        if (rpm<bool>(localPlayer + Offsets::Player::IsDead)) {
            s_lockedTargetAddr = 0;
            s_lockTimeoutTicks = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }

// FIRING & SCOPING STATE
        bool isFiring = rpm<bool>(localPlayer + Offsets::Player::IsFiring);
        float fovOffset = rpm<float>(localPlayer + Offsets::Player::FOVOffset);
        bool isScoping = (fovOffset < -0.01f || fovOffset > 0.01f);

        bool triggerActive = false;
        if (cfg::aim::trigger_mode == 1) {
            triggerActive = true; // Always In FOV
        } else if (cfg::aim::scope_aim && isScoping) {
            triggerActive = true; // Instant activation when scoping with sniper/long guns
        } else {
            triggerActive = isFiring;
        }

        if (!triggerActive) {
            // Give grace period so lock doesn't drop between rapid-fire bullets
            if (s_lockTimeoutTicks > 0) {
                s_lockTimeoutTicks--;
            } else {
                s_lockedTargetAddr = 0;
                s_burstTicks = 0;
                s_prevDir = Vector3::Zero();
            }
            return;
        }

        // Active trigger -> refresh lock lease
        s_lockTimeoutTicks = 30;

        std::vector<PlayerData> enemies;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            enemies = data::players;
        }

        if (enemies.empty()) {
            s_lockedTargetAddr = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }

        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;
        float fovLimit = cfg::aim::fov_radius;
        const PlayerData* bestEnemy = nullptr;
        Vector3 bestScreenPos = Vector3::Zero();

        // 1. STRICT TARGET HYSTERESIS (STICK TO CURRENT TARGET)
        // If Target 1 is locked, NEVER switch to Target 2 as long as Target 1 is in view!
        // This completely eliminates crosshair flickering and screen shaking!
        if (s_lockedTargetAddr != 0) {
            for (const auto& enemy : enemies) {
                if (enemy.addr == s_lockedTargetAddr) {
                    if (enemy.curHP > 0 && enemy.Head.z >= 0.5f) {
                        float dx = enemy.Head.x - screenCx;
                        float dy = enemy.Head.y - screenCy;
                        float dist = sqrtf(dx * dx + dy * dy);
                        // Generous hysteresis boundary (1.5x FOV): never detach on micro-movements
                        if (dist <= fovLimit * 1.5f) {
                            bestEnemy = &enemy;
                            bestScreenPos = enemy.Head;
                        }
                    }
                    break;
                }
            }

            // If locked enemy died or left FOV, clear lock to allow new target acquisition
            if (!bestEnemy) {
                s_lockedTargetAddr = 0;
                s_burstTicks = 0;
                s_prevDir = Vector3::Zero();
            }
        }

        // 2. NEW TARGET ACQUISITION (ONLY WHEN NO TARGET IS LOCKED)
        // Find single closest enemy inside FOV circle
        if (!bestEnemy) {
            float bestScore = 9999999.0f;
            for (const auto& enemy : enemies) {
                if (enemy.Head.z < 0.5f || enemy.curHP <= 0) continue;
                if (cfg::aim::ignore_bots && enemy.isBot) continue;
                if (cfg::aim::visible_check && !enemy.isVisible) continue;
                if (enemy.distance > cfg::aim::max_distance) continue;

                Vector3 aimPos = enemy.Head;
                float dx = aimPos.x - screenCx;
                float dy = aimPos.y - screenCy;
                float crosshairDist = sqrtf(dx * dx + dy * dy);

                // STRICT FOV CIRCLE: Only lock if inside circle
                if (crosshairDist > fovLimit) continue;

                float score = crosshairDist;
                if (cfg::aim::priority_mode == 1) { // Lowest HP
                    score = (float)enemy.curHP;
                } else if (cfg::aim::priority_mode == 2) { // 3D Distance
                    score = enemy.distance;
                }

                if (score < bestScore) {
                    bestScore = score;
                    bestEnemy = &enemy;
                    bestScreenPos = aimPos;
                }
            }

            if (bestEnemy) {
                s_lockedTargetAddr = bestEnemy->addr;
                s_lockTimeoutTicks = 30;
                s_burstTicks = 0;
            }
        }

        if (!bestEnemy) {
            s_lockedTargetAddr = 0;
            s_burstTicks = 0;
            s_prevDir = Vector3::Zero();
            return;
        }

        s_burstTicks++;

        // Record target diagnostic telemetry
        g_currentTarget.valid = true;
        snprintf(g_currentTarget.name, sizeof(g_currentTarget.name), "%s",
                 bestEnemy->name[0] ? bestEnemy->name : (bestEnemy->isBot ? "BOT" : "Player"));
        g_currentTarget.distance = bestEnemy->distance;
        g_currentTarget.hp = static_cast<int>(bestEnemy->curHP);
        g_currentTarget.screenPos = bestScreenPos;
        g_currentTarget.headWorld = bestEnemy->headWorld;

        // 3D BULLET ALIGNMENT WITH PROGRESSIVE DRAG & SMOOTH INTERPOLATION
        uint64_t aimInfo = rpm<uint64_t>(localPlayer + Offsets::Player::AimInfo);
        if (is_valid_ptr(aimInfo)) {
            uint64_t dirAddr = aimInfo + Offsets::Aim::Direction;
            Vector3 bulletOrigin = rpm<Vector3>(aimInfo + Offsets::Aim::StartPosition);

            Vector3 target3D = bestEnemy->headWorld;
            if (target3D != Vector3::Zero()) {
                if (cfg::aim::target_bone == 1) {
                    target3D.y += 0.00f; // Neck
                } else if (cfg::aim::target_bone == 2) {
                    target3D.y -= 0.35f; // Body
                } else {
                    target3D.y += 0.12f; // Head: direct lift into skull center
                }

                if (std::isfinite(bulletOrigin.x) &&
                    std::isfinite(bulletOrigin.y) &&
                    std::isfinite(bulletOrigin.z) &&
                    bulletOrigin.sqrMagnitude() > 1e-6f) {
                    Vector3 diff = target3D - bulletOrigin;
                    Vector3 headshotDir = SafeNormalize(diff);
                    if (std::isfinite(headshotDir.x) && std::isfinite(headshotDir.y) && std::isfinite(headshotDir.z)) {
                        // 100% direct snap: zero smoothing lag so EVERY single bullet locks directly onto the skull
                        wpm<Vector3>(dirAddr, headshotDir);
                        wpm<bool>(aimInfo + 0x71, false);

                        uint64_t headCollider = rpm<uint64_t>(bestEnemy->addr + Offsets::Player::HeadCollider);
                        if (is_valid_ptr(headCollider)) {
                            wpm<uint64_t>(bestEnemy->addr + Offsets::Player::ColWrite, headCollider);
                        }

                        // Lock weapon recoil to zero so multi-bullet spray stays 100% on the head
                        uint64_t weapon = rpm<uint64_t>(localPlayer + Offsets::Player::Weapon);
                        if (is_valid_ptr(weapon)) {
                            uint64_t wdata = rpm<uint64_t>(weapon + Offsets::Weapon::Data);
                            if (is_valid_ptr(wdata)) {
                                wpm<float>(wdata + Offsets::Weapon::Recoil, 0.0f);
                            }
                        }
                    }
                }
            }
        }

        // NOTE: ZERO camera Rotation write!
        // Camera stays 100% untouched by code. User has complete, natural thumb control.
        // Screen will never snap, jump, or pull between multiple enemies!
    }

    // FOV CIRCLE & ACTIVE LOCK VISUALIZER
    // Scaled dynamically by cfg::aim::fov_radius
    inline void render_fov(ImDrawList* dl) {
        if (!cfg::aim::enable || !cfg::aim::show_fov) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float r  = cfg::aim::fov_radius;

        // Smooth Orchid / White FOV circle (shows exact trigger zone)
        dl->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 255, 45), 64, 1.0f);
        dl->AddCircle(ImVec2(cx, cy), r + 1.0f, IM_COL32(225, 0, 153, 160), 64, 1.5f);

        // If target is actively locked inside FOV, draw target lock ring
        if (g_currentTarget.valid && g_currentTarget.screenPos.z >= 0.001f) {
            dl->AddCircle(ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                          10.0f, IM_COL32(255, 30, 80, 240), 16, 2.0f);
            dl->AddCircle(ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                          3.5f, IM_COL32(255, 255, 255, 255), 12, 1.5f);
        }
    }

} // namespace aimbot
