#pragma once

#include <string>
#include <ctime>

namespace lic {

    bool is_valid();
    void init();
    std::string current_key();
    bool set_key(const std::string& key);
    time_t start_date();
    time_t expiry_date();
    const char* status_text();
    void open_update_page();
    bool should_show_redirect_toast();
    void trigger_redirect_toast();
    void update_toast_timer(float dt);

}
