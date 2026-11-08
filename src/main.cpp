// ZqwClient — ninmods PROJECT
#include "Android_draw/draw.h"
#include "ui/menu.hpp"
#include "protect/license.hpp"
#include "other/memory.hpp"
#include "other/crash_handler.hpp"
#include "other/display_manager.hpp"
#include "game/data.hpp"
#include "game/Offsets.h"
#include "func/esp.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <android/log.h>
#include <iostream>
#include <string>
#include <atomic>

#define LOG_TAG "gemini"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

// ═══════════════════════════════════════════════════════════
// ANSI COLOR
// ═══════════════════════════════════════════════════════════
#define RST        "\033[0m"
#define BOLD       "\033[1m"
#define FG_BRED    "\033[91m"
#define FG_BGREEN  "\033[92m"
#define FG_BYELLOW "\033[93m"
#define FG_BBLUE   "\033[94m"
#define FG_BCYAN   "\033[96m"
#define FG_BWHITE  "\033[97m"

extern float global_anim_time;
std::atomic<bool> main_thread_flag{true};

// ═══════════════════════════════════════════════════════════
// TARGET GAME
// ═══════════════════════════════════════════════════════════
enum TargetGame {
    GAME_FF     = 0,
    GAME_FF_MAX = 1
};

static TargetGame g_targetGame = GAME_FF;

static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFMaxActivity = "com.dts.freefiremax/com.dts.freefiremax.FFMainActivity";
static const char* kFFPackage     = "com.dts.freefireth";
static const char* kFFMaxPackage  = "com.dts.freefiremax";

static const char* GetTargetActivity() {
    return (g_targetGame == GAME_FF) ? kFFActivity : kFFMaxActivity;
}
static const char* GetTargetPackage() {
    return (g_targetGame == GAME_FF) ? kFFPackage : kFFMaxPackage;
}

// ═══════════════════════════════════════════════════════════
// WAIT FOR GAME (DAEMON TASK SYNC)
// ═══════════════════════════════════════════════════════════
static int wait_for_game() {
    printf("[*] Synchronizing runtime context with target subsystem...\n");
    fflush(stdout);

    while (true) {
        proc::pid = get_pid();
        if (proc::pid > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    printf("[+] Context bound (Task PID: %d)\n", proc::pid);
    fflush(stdout);
    return proc::pid;
}

// ═══════════════════════════════════════════════════════════
// GEMINI CLI DASHBOARD & INITIALIZATION GATE
// ═══════════════════════════════════════════════════════════
static void print_dashboard() {
    time_t exp = lic::expiry_date();
    struct tm* tm_info = gmtime(&exp);
    char exp_buf[64] = {0};
    if (tm_info) {
        strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d %H:%M:%S UTC", tm_info);
    } else {
        snprintf(exp_buf, sizeof(exp_buf), "2026-11-07 00:00:00 UTC");
    }

    printf("Google Gemini Developer API CLI v1.5.0\n");
    printf("Environment    : Linux aarch64 (Embedded Core Engine)\n");
    printf("API Key Status : Valid / Active (%s)\n", lic::status_text());
    printf("Token Expiry   : %s\n", exp_buf);
    printf("Runtime Status : Ready | Daemon Task Attached\n\n");
    fflush(stdout);
}

static bool check_license_gate(int argc, char** argv) {
    lic::init();

    printf("Google Gemini Core Engine — Authentication\n");
    printf("API Key is required to initialize runtime pipeline.\n");

    // 1. Check command-line argument for license key (allows standalone launch: sh gemini-cli.sh <KEY>)
    for (int i = 1; i < argc; i++) {
        if (argv[i] && strlen(argv[i]) >= 8 && strcmp(argv[i], "ffmax") != 0 && strcmp(argv[i], "START") != 0) {
            if (lic::set_key(argv[i]) && lic::is_valid()) {
                printf("[+] Pre-authorized via CLI argument.\n");
                printf("[+] API Key validated successfully. Core engine authorized.\n");
                printf("[+] Loading overlay engine into RAM...\n\n");
                fflush(stdout);
                return true;
            }
        }
    }

    // 2. Check environment variable from loader
    const char* env_key = getenv("NIN_LICENSE_KEY");
    if (env_key && strlen(env_key) > 0) {
        if (lic::set_key(env_key) && lic::is_valid()) {
            printf("[+] Pre-authorized via secure environment session.\n");
            printf("[+] API Key validated successfully. Core engine authorized.\n\n");
            printf("[+] Loading overlay engine into RAM...\n\n");
            fflush(stdout);
            return true;
        }
    }

    // 3. Interactive stdin prompt for license key
    printf("[?] Enter API Access Key: ");
    fflush(stdout);

    char key_input[128] = {0};
    if (fgets(key_input, sizeof(key_input), stdin)) {
        size_t len = strlen(key_input);
        while (len > 0 && (key_input[len - 1] == '\n' || key_input[len - 1] == '\r' || key_input[len - 1] == ' ' || key_input[len - 1] == '\t')) {
            key_input[len - 1] = '\0';
            len--;
        }
    }

    bool key_ok = false;
    if (strlen(key_input) > 0) {
        key_ok = lic::set_key(key_input);
    } else if (lic::is_valid()) {
        key_ok = true;
    }

    if (!key_ok || !lic::is_valid()) {
        printf("[-] Authentication failed: Invalid or expired API token.\n");
        fflush(stdout);
        return false;
    }

    printf("[+] API Key validated successfully. Core engine authorized.\n");

    // 4. Interactive terminal check (MT Manager terminal, Termux, ADB) -> launch immediately
    if (isatty(fileno(stdin))) {
        printf("[+] Interactive terminal detected (MT Manager / Termux).\n");
        printf("[+] Loading overlay engine into RAM immediately...\n\n");
        fflush(stdout);
        return true;
    }

    // 5. Piped from DriverLoader app -> wait for user tap on Start button
    printf("[?] Ready to load into memory. Awaiting START signal...\n");
    fflush(stdout);

    char start_input[64] = {0};
    if (fgets(start_input, sizeof(start_input), stdin)) {
        // Unblocked by Start button tap from app
    }

    printf("[+] Loading overlay engine into RAM...\n\n");
    fflush(stdout);
    return true;
}

// ═══════════════════════════════════════════════════════════
// START OVERLAY
// ═══════════════════════════════════════════════════════════
static int start_overlay() {
    LOGI("=== ninmods starting ===");
    printf("[*] Probing display dimensions and screen orientation...\n");
    fflush(stdout);

    screen_config();

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    printf("[*] Screen dimensions: %dx%d (Orientation: %u)\n", native_window_screen_x, native_window_screen_y, displayInfo.orientation);
    printf("[*] Configuring Android system touch security policies...\n");
    fflush(stdout);

    system("settings put global block_untrusted_touches 0 2>/dev/null; settings put system block_untrusted_touches 0 2>/dev/null");

    printf("[*] Creating SurfaceComposerClient Native Window and initializing EGL...\n");
    fflush(stdout);

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true, false)) {
        printf("[-] Failed to initialize overlay GUI (NativeWindow or EGL error)\n");
        fflush(stdout);
        return -1;
    }
    printf("[+] Floating overlay engine initialized successfully!\n");
    printf("[*] Attaching Linux input touch listener...\n");
    fflush(stdout);

    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, true);

    // ─── START BACKGROUND ATTACH THREAD ───
    // Allows overlay GUI & floating logo to appear immediately upon launching,
    // without blocking for game launch.
    std::thread attach_worker([]() {
        while (main_thread_flag) {
            proc::pid = get_pid();
            if (proc::pid > 0) {
                proc::lib = get_lib();
                if (proc::lib > 0) {
                    // Verify memory
                    uint32_t elfMagic = rpm<uint32_t>(proc::lib);
                    if (elfMagic == 0x464C457F) {
                        data::thread_start();
                        printf("[+] Runtime subsystem attached (PID: %d, Lib: 0x%lx)\n", proc::pid, (unsigned long)proc::lib);
                        fflush(stdout);

                        // Monitor target lifecycle
                        while (main_thread_flag) {
                            if (get_pid() <= 0) {
                                data::thread_stop();
                                printf("[*] Session detached. Waiting for reconnect...\n");
                                fflush(stdout);
                                break;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    attach_worker.detach();

    printf("[+] Gemini CLI runtime daemon active. Processing stream.\n\n");
    fflush(stdout);

    // ═══════════════════════════════════════════════════════
    //  MAIN LOOP — RENDER LOOP (MINIMAL LOGGING)
    // ═══════════════════════════════════════════════════════
    int dbgCounter = 0;
    while (main_thread_flag) {
        global_anim_time += 0.016f;
        drawBegin();
        ui::menu::render();
        drawEnd();
        usleep(16000);

        // ─── PERIODIC DAEMON HEARTBEAT (EVERY ~5 SECONDS) ───
        if (++dbgCounter >= 300) {
            dbgCounter = 0;
            bool inM = data::inMatch.load(std::memory_order_relaxed);
            printf("[gemini-cli] daemon status: %s | stream objects: active\n", inM ? "processing" : "idle");
            fflush(stdout);
        }
    }

    shutdown();
    Touch_Close();
    data::thread_stop();
    return 0;
}

// ═══════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════
static void mask_process_name(int argc, char** argv, const char* fake_name) {
    if (!argv || !argv[0] || !fake_name) return;

    // 1. Update /proc/self/comm via prctl (max 16 chars)
    prctl(PR_SET_NAME, fake_name, 0, 0, 0);

    // 2. Safe argv[0] in-place rename without touching succeeding memory buffers
    size_t len = strlen(argv[0]);
    if (len > 0) {
        memset(argv[0], 0, len);
        strncpy(argv[0], fake_name, len);
    }
}

int main(int argc, char** argv) {
    // 0. Install crash interception handler
    crash_guard::install();

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Mask process name immediately upon entry to gemini-cli
    mask_process_name(argc, argv, "gemini-cli");

    if (argc >= 2) {
        if (strcmp(argv[1], "ffmax") == 0) {
            g_targetGame = GAME_FF_MAX;
        }
    }

    if (!check_license_gate(argc, argv)) {
        return 1;
    }

    print_dashboard();

    return start_overlay();
}
