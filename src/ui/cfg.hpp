#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
// Master
    inline bool  box              = false;
    inline int   box_type         = 1; // 1 = Full Chaturbhuj (Rectangular) Box
    inline bool  name             = false;
    inline bool  health           = false;
    inline bool  armor            = false;
    inline bool  distance         = false;
    inline bool  weapon           = false;
    inline bool  bot_tag          = false;
    inline bool  ignore_bots      = false;
    inline bool  tracer           = false;
    inline bool  crosshair        = false;
    inline bool  firing_ind       = false;
    inline bool  loot             = false;

// Setting
    inline float max_range        = 250.f;
    inline float loot_range       = 150.f;
    inline float box_rounding     = 3.f;
    inline float box_thickness    = 2.0f;
    inline float tracer_thickness = 1.2f;
    inline float bracket_frac     = 0.28f;

// Warna
    inline ImVec4 box_col         = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible     = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col        = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col    = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col      = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
}

namespace combat {
    inline bool auto_target_closest = false;
    inline bool prioritize_low_hp   = false;
    inline bool filter_invisible    = false;
}

namespace aim {
    inline bool  enable           = false;
    inline bool  scope_aim        = false; // Scoping Headshot: Locks directly to head when scoping
    inline bool  show_fov         = false;
    inline float fov_radius       = 180.0f;
    inline float smooth           = 6.0f;
    inline float headshot_rate    = 100.0f; // 0% to 100% (Adjustable headshot connection rate)
    inline int   target_bone      = 0; // 0 = Head, 1 = Neck/Chest, 2 = Pelvis/Body
    inline int   trigger_mode     = 0; // 0 = When Firing, 1 = Always In FOV
    inline int   priority_mode    = 0; // 0 = Crosshair Distance, 1 = Low HP, 2 = 3D Distance
    inline bool  ignore_knocked   = false;
    inline bool  ignore_bots      = false;
    inline bool  visible_check    = false;
    inline float max_distance     = 200.0f;
    inline bool  prediction       = false;
}
}
