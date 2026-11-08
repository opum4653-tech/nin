#pragma once

#include <string>
#include <ctime>

// CLIENT-SIDE LICENSE KEY SYSTEM & DATE-WINDOW ENFORCEMENT
namespace lic {

    // Returns true only if start_date <= now < expiry_date AND key matches
    bool is_valid();

    // Idempotent initialization: loads persistent state from storage & evaluates
    void init();

    // Returns the current stored key (empty if none)
    std::string current_key();

    // Applies a new key with NINMODS-after-expiry redirect rule
    bool set_key(const std::string& key);

    // Returns start date timestamp (1793750400 = 2026-11-04 00:00:00 UTC)
    time_t start_date();

    // Returns expiry date timestamp (1794009600 = 2026-11-07 00:00:00 UTC)
    time_t expiry_date();

    // Returns readable status: "VALID" | "EXPIRED" | "INVALID" | "COMING SOON" | "NO KEY"
    const char* status_text();

    // Opens https://t.me/ninmods via Android Intent (JNI or system fallback)
    void open_update_page();

    // Toast helpers for non-blocking UI feedback
    bool should_show_redirect_toast();
    void trigger_redirect_toast();
    void update_toast_timer(float dt);

} // namespace lic
