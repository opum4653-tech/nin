#pragma once

#include <string>
#include <ctime>

// CLIENT-SIDE LICENSE KEY SYSTEM (CLOUD-AUTHENTICATED)
namespace lic {

    // Returns true only if license is authenticated online and active
    bool is_valid();

    // Idempotent initialization: loads persistent state from loader package storage & evaluates
    void init();

    // Returns the current stored key (empty if none)
    std::string current_key();

    // Applies and verifies a new key against the online server
    bool set_key(const std::string& key);

    // Returns start date timestamp (0 if dynamic)
    time_t start_date();

    // Returns expiry date timestamp from online subscription session (0 if unbounded)
    time_t expiry_date();

    // Returns readable status: "VALID" | "EXPIRED" | "INVALID" | "NO KEY"
    const char* status_text();

    // Opens https://t.me/ninmods via Android Intent (JNI or system fallback)
    void open_update_page();

    // Toast helpers for non-blocking UI feedback
    bool should_show_redirect_toast();
    void trigger_redirect_toast();
    void update_toast_timer(float dt);

} // namespace lic
