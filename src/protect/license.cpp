#include "license.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <string>
#include <unistd.h>
#include <jni.h>
#include <android/log.h>
#include <curl/curl.h>

#define LIC_LOG_TAG "ninmods_lic"
#define LIC_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LIC_LOG_TAG, __VA_ARGS__)
#define LIC_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LIC_LOG_TAG, __VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════════
// XOR-OBFUSCATED CONSTANTS & RUNTIME DECODERS
// ═══════════════════════════════════════════════════════════════════════════════

// start_date = 1793750400 (2026-11-04 00:00:00 UTC) = 0x6AEA3300ULL
static constexpr unsigned char K_START_BYTES[8] = { 0x5D, 0x6E, 0xB7, 0x37, 0x5D, 0x5D, 0x5D, 0x5D };
static constexpr unsigned char K_START_SEED = 0x5D;

// expiry_date = 1794009600 (2026-11-07 00:00:00 UTC) = 0x6AEE2600ULL (Active through 2026-11-06 23:59:59 UTC)
static constexpr unsigned char K_EXPIRY_BYTES[8] = { 0x7E, 0x58, 0x90, 0x14, 0x7E, 0x7E, 0x7E, 0x7E };
static constexpr unsigned char K_EXPIRY_SEED = 0x7E;

// TELEGRAM_URL = "https://t.me/ninmods" (length 20)
static constexpr unsigned char K_TELEGRAM_URL_BYTES[20] = {
    0x0A, 0x16, 0x16, 0x12, 0x11, 0x58, 0x4D, 0x4D, 0x16, 0x4C,
    0x0F, 0x07, 0x4D, 0x0C, 0x0B, 0x0C, 0x0F, 0x0D, 0x06, 0x11
};
static constexpr unsigned char K_TELEGRAM_URL_SEED = 0x62;

static uint64_t decode_uint64(const unsigned char* bytes, unsigned char seed) {
    uint64_t val = 0;
    for (size_t i = 0; i < 8; ++i) {
        uint64_t b = static_cast<uint64_t>(bytes[i] ^ seed);
        val |= (b << (i * 8));
    }
    return val;
}

static std::string decode_string(const unsigned char* bytes, size_t len, unsigned char seed) {
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(bytes[i] ^ seed));
    }
    return s;
}

static std::string normalize(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════════
// INTERNAL STATE
// ═══════════════════════════════════════════════════════════════════════════════
static std::mutex s_lic_mutex;
static std::string s_stored_key = "";
static time_t s_last_seen_ts = 0;
static time_t s_online_expiry_ts = 0;
static bool s_online_verified = false;
static bool s_is_valid = false;
static bool s_clock_tampered = false;
static bool s_initialized = false;
static float s_toast_timer = 0.0f;

static JavaVM* s_jvm = nullptr;
static jobject s_activity = nullptr;

// ═══════════════════════════════════════════════════════════════════════════════
// PERSISTENT STORAGE
// ═══════════════════════════════════════════════════════════════════════════════
static std::string get_storage_path() {
    if (s_jvm && s_activity) {
        JNIEnv* env = nullptr;
        jint res = s_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        bool attached = false;
        if (res == JNI_EDETACHED) {
            if (s_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) attached = true;
        }
        if (env) {
            jclass ctxClass = env->GetObjectClass(s_activity);
            if (ctxClass) {
                jmethodID getFilesDir = env->GetMethodID(ctxClass, "getFilesDir", "()Ljava/io/File;");
                if (getFilesDir) {
                    jobject fileObj = env->CallObjectMethod(s_activity, getFilesDir);
                    if (fileObj) {
                        jclass fileClass = env->GetObjectClass(fileObj);
                        if (fileClass) {
                            jmethodID getAbsPath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
                            if (getAbsPath) {
                                jstring pathStr = reinterpret_cast<jstring>(env->CallObjectMethod(fileObj, getAbsPath));
                                if (pathStr) {
                                    const char* chars = env->GetStringUTFChars(pathStr, nullptr);
                                    std::string dir = chars ? chars : "";
                                    if (chars) env->ReleaseStringUTFChars(pathStr, chars);
                                    env->DeleteLocalRef(pathStr);
                                    if (!dir.empty()) {
                                        env->DeleteLocalRef(fileClass);
                                        env->DeleteLocalRef(fileObj);
                                        env->DeleteLocalRef(ctxClass);
                                        if (attached) s_jvm->DetachCurrentThread();
                                        return dir + "/license.dat";
                                    }
                                }
                            }
                            env->DeleteLocalRef(fileClass);
                        }
                        env->DeleteLocalRef(fileObj);
                    }
                }
                env->DeleteLocalRef(ctxClass);
            }
            if (attached) s_jvm->DetachCurrentThread();
        }
    }

    const char* home = getenv("HOME");
    if (home && access(home, W_OK) == 0) {
        return std::string(home) + "/.nin_license.dat";
    }
    const char* tmp = getenv("TMPDIR");
    if (tmp && access(tmp, W_OK) == 0 && strcmp(tmp, "/data/local/tmp") != 0) {
        return std::string(tmp) + "/.nin_license.dat";
    }
    if (access("/data/adb", W_OK) == 0) {
        return "/data/adb/.nin_license.dat";
    }
    return "/dev/.nin_license.dat";
}

static void save_state_to_disk() {
    std::string path = get_storage_path();
    FILE* fp = fopen(path.c_str(), "w");
    if (!fp) return;

    // Content format: "v2\n<key>\n<last_seen_ts>\n<online_expiry_ts>\n"
    fprintf(fp, "v2\n%s\n%lld\n%lld\n",
        s_stored_key.c_str(),
        static_cast<long long>(s_last_seen_ts),
        static_cast<long long>(s_online_expiry_ts)
    );
    fclose(fp);
}

static void evaluate_state(time_t now) {
    if (s_clock_tampered) {
        s_is_valid = false;
        return;
    }

    if (s_stored_key.empty()) {
        s_is_valid = false;
        return;
    }

    // 1. Cloud-Verified Online License State (Commercial Subscription Authority)
    if (s_online_verified) {
        if (s_online_expiry_ts > 0 && now >= s_online_expiry_ts) {
            s_is_valid = false; // License expired!
            return;
        }
        s_is_valid = true;
        return;
    }

    // Require authentic online verified subscription session
    s_is_valid = false;
}

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC API IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════════
namespace lic {

    time_t start_date() {
        return static_cast<time_t>(decode_uint64(K_START_BYTES, K_START_SEED));
    }

    time_t expiry_date() {
        if (s_online_verified && s_online_expiry_ts > 0) {
            return s_online_expiry_ts;
        }
        return static_cast<time_t>(decode_uint64(K_EXPIRY_BYTES, K_EXPIRY_SEED));
    }

    void init() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        if (s_initialized) return;

        s_stored_key.clear();
        s_last_seen_ts = 0;
        s_clock_tampered = false;
        s_is_valid = false;

        std::string path = get_storage_path();
        FILE* fp = fopen(path.c_str(), "r");
        if (fp) {
            char header[16] = {0};
            char keyBuf[128] = {0};
            long long ts = 0;
            long long exp_ts = 0;

            if (fscanf(fp, "%15s\n%127s\n%lld\n%lld", header, keyBuf, &ts, &exp_ts) >= 2) {
                if (strcmp(header, "v2") == 0) {
                    s_stored_key = keyBuf;
                    s_last_seen_ts = static_cast<time_t>(ts);
                    s_online_expiry_ts = static_cast<time_t>(exp_ts);
                    time_t now_check = time(nullptr);
                    if (s_online_expiry_ts > 0 && now_check < s_online_expiry_ts) {
                        s_online_verified = true;
                    }
                } else if (strcmp(header, "v1") == 0) {
                    s_stored_key = keyBuf;
                    s_last_seen_ts = static_cast<time_t>(ts);
                }
            }
            fclose(fp);
        }

        time_t now = time(nullptr);

        // Clock rollback guard: 1 day tolerance
        if (s_last_seen_ts > 0 && now < (s_last_seen_ts - 86400)) {
            s_clock_tampered = true;
            s_is_valid = false;
        } else {
            s_last_seen_ts = std::max(now, s_last_seen_ts);
            evaluate_state(now);
            if (!s_stored_key.empty()) {
                save_state_to_disk();
            }
        }

        s_initialized = true;
    }

    bool is_valid() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        if (!s_initialized) return false;
        time_t now = time(nullptr);
        evaluate_state(now);
        return s_is_valid;
    }

    std::string current_key() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        return s_stored_key;
    }

    static std::string get_device_hwid() {
        // 1. Environment variable passed directly from DriverLoader / root shell
        const char* env_hwid = getenv("NIN_DEVICE_HWID");
        if (env_hwid && strlen(env_hwid) > 0) {
            return std::string(env_hwid);
        }

        // 2. Persistent file written by DriverLoader app (pure RAM /dev or /data/adb)
        const char* hwid_candidates[] = {
            "/dev/.nin_hwid",
            "/data/adb/.nin_hwid"
        };
        for (const char* path : hwid_candidates) {
            FILE* fp_file = fopen(path, "r");
            if (fp_file) {
                char buf[128] = {0};
                if (fgets(buf, sizeof(buf), fp_file)) {
                    buf[strcspn(buf, "\r\n")] = 0;
                    if (strlen(buf) > 0) {
                        fclose(fp_file);
                        return std::string(buf);
                    }
                }
                fclose(fp_file);
            }
        }

        // 3. System properties fallback
        char buf[128] = {0};
        FILE* fp = popen("getprop ro.serialno 2>/dev/null", "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = 0;
            }
            pclose(fp);
        }
        if (strlen(buf) > 0 && strcmp(buf, "unknown") != 0) {
            return std::string(buf);
        }
        fp = popen("getprop ro.build.id 2>/dev/null", "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = 0;
            }
            pclose(fp);
        }
        return strlen(buf) > 0 ? std::string(buf) : "NIN_DEVICE_HWID";
    }

    static size_t curl_write_string_cb(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        auto* s = reinterpret_cast<std::string*>(userp);
        if (s) s->append(reinterpret_cast<const char*>(contents), total);
        return total;
    }

    static bool verify_license_online(const std::string& key) {
        std::string hwid = get_device_hwid();
        std::string server_url = "https://ninmods.smmtor.com";
        const char* env_url = getenv("NIN_SERVER_URL");
        if (env_url && strlen(env_url) > 0) {
            server_url = std::string(env_url);
        }

        char response[2048] = {0};
        bool executed = false;

        // 1. Primary: Official Embedded libcurl HTTP/HTTPS Client
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string curl_resp;
            std::string endpoint = server_url + "/api/v2/app/activate";
            std::string json_body = "{\"license_key\":\"" + key + "\",\"hwid\":\"" + hwid + "\"}";

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string_cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_resp);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 12L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK && !curl_resp.empty()) {
                strncpy(response, curl_resp.c_str(), sizeof(response) - 1);
                executed = true;
            }
        }

        // 2. Secondary: Fallback process pool if direct socket blocked in sandbox
        if (!executed) {
            const char* clients[] = {
                "curl -k -s -m 12",
                "/data/adb/ap/bin/curl -k -s -m 12",
                "/data/adb/magisk/busybox curl -k -s -m 12",
                "/data/adb/ksu/bin/curl -k -s -m 12",
                "/system/bin/curl -k -s -m 12",
                "/system/xbin/curl -k -s -m 12",
                "wget -q --no-check-certificate -O - -T 12"
            };

            for (const char* client : clients) {
                char cmd[1536];
                if (strstr(client, "wget")) {
                    snprintf(cmd, sizeof(cmd),
                        "%s --header='Content-Type: application/json' --post-data='{\"license_key\":\"%s\",\"hwid\":\"%s\"}' %s/api/v2/app/activate 2>/dev/null",
                        client, key.c_str(), hwid.c_str(), server_url.c_str());
                } else {
                    snprintf(cmd, sizeof(cmd),
                        "%s -X POST -H 'Content-Type: application/json' -d '{\"license_key\":\"%s\",\"hwid\":\"%s\"}' %s/api/v2/app/activate 2>/dev/null",
                        client, key.c_str(), hwid.c_str(), server_url.c_str());
                }

                FILE* fp = popen(cmd, "r");
                if (fp) {
                    size_t bytesRead = fread(response, 1, sizeof(response) - 1, fp);
                    pclose(fp);
                    if (bytesRead > 0) {
                        response[bytesRead] = 0;
                        executed = true;
                        break;
                    }
                }
            }
        }

        if (!executed || strlen(response) == 0) {
            printf("[-] Network error: Unable to reach license verification gateway.\n");
            fflush(stdout);
            return false;
        }

        // Check if server returned authentic active license
        if (strstr(response, "\"success\":true") || 
            strstr(response, "\"valid\":true") ||
            strstr(response, "\"status\":\"active\"") || 
            strstr(response, "\"status\":\"activated\"") || 
            strstr(response, "\"status\":true")) {
            time_t now = time(nullptr);
            long remSec = 86400;
            const char* remPtr = strstr(response, "\"remaining_seconds\":");
            if (remPtr) {
                long parsed = atol(remPtr + 20);
                if (parsed > 0) remSec = parsed;
            }

            s_last_seen_ts = now;
            s_stored_key = key;
            s_online_verified = true;
            s_online_expiry_ts = now + remSec;
            s_is_valid = true;
            s_clock_tampered = false;
            save_state_to_disk();
            return true;
        }

        // Print server's descriptive rejection reason (e.g. expired, invalid, device limit)
        const char* errPtr = strstr(response, "\"error\":\"");
        if (errPtr) {
            char errMsg[256] = {0};
            sscanf(errPtr + 9, "%255[^\"]", errMsg);
            printf("[-] License Server: %s\n", errMsg);
            fflush(stdout);
        }
        s_is_valid = false;
        return false;
    }

    bool set_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        if (key.empty()) return false;

        // 1. Primary: Real-Time Cloud Server License Verification
        if (verify_license_online(key)) {
            return true;
        }

        return false;
    }

    const char* status_text() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        time_t now = time(nullptr);
        time_t t_start = start_date();
        time_t t_exp   = expiry_date();

        if (s_clock_tampered) return "INVALID";
        if (now < t_start)   return "COMING SOON";
        if (now >= t_exp)    return "EXPIRED";
        if (s_stored_key.empty()) return "NO KEY";
        if (s_is_valid)      return "VALID";
        return "INVALID";
    }

    static void open_url_jni(const char* url) {
        if (!url || !*url) return;

        // Try JNI Android Intent first if JavaVM is available
        if (s_jvm && s_activity) {
            JNIEnv* env = nullptr;
            jint res = s_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            bool attached = false;
            if (res == JNI_EDETACHED) {
                if (s_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) attached = true;
            }
            if (env) {
                jclass uriClass = env->FindClass("android/net/Uri");
                if (uriClass) {
                    jmethodID parseMethod = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
                    if (parseMethod) {
                        jstring urlStr = env->NewStringUTF(url);
                        if (urlStr) {
                            jobject uriObj = env->CallStaticObjectMethod(uriClass, parseMethod, urlStr);
                            if (uriObj) {
                                jclass intentClass = env->FindClass("android/content/Intent");
                                if (intentClass) {
                                    jmethodID intentInit = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                                    if (intentInit) {
                                        jstring actionView = env->NewStringUTF("android.intent.action.VIEW");
                                        if (actionView) {
                                            jobject intentObj = env->NewObject(intentClass, intentInit, actionView, uriObj);
                                            if (intentObj) {
                                                jmethodID addFlagsMethod = env->GetMethodID(intentClass, "addFlags", "(I)Landroid/content/Intent;");
                                                if (addFlagsMethod) {
                                                    env->CallObjectMethod(intentObj, addFlagsMethod, 0x10000000); // FLAG_ACTIVITY_NEW_TASK
                                                }
                                                jclass activityClass = env->GetObjectClass(s_activity);
                                                if (activityClass) {
                                                    jmethodID startActivityMethod = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
                                                    if (startActivityMethod) {
                                                        env->CallVoidMethod(s_activity, startActivityMethod, intentObj);
                                                    }
                                                    env->DeleteLocalRef(activityClass);
                                                }
                                                env->DeleteLocalRef(intentObj);
                                            }
                                            env->DeleteLocalRef(actionView);
                                        }
                                    }
                                    env->DeleteLocalRef(intentClass);
                                }
                                env->DeleteLocalRef(uriObj);
                            }
                            env->DeleteLocalRef(urlStr);
                        }
                    }
                    env->DeleteLocalRef(uriClass);
                }
                if (attached) s_jvm->DetachCurrentThread();
                return;
            }
        }

        // Fallback: Launch intent via Android shell command
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "am start -a android.intent.action.VIEW -d '%s' >/dev/null 2>&1", url);
        system(cmd);
    }

    void open_update_page() {
        std::string url = decode_string(K_TELEGRAM_URL_BYTES, sizeof(K_TELEGRAM_URL_BYTES), K_TELEGRAM_URL_SEED);
        open_url_jni(url.c_str());
    }

    bool should_show_redirect_toast() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        return s_toast_timer > 0.0f;
    }

    void trigger_redirect_toast() {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        s_toast_timer = 2.0f;
    }

    void update_toast_timer(float dt) {
        std::lock_guard<std::mutex> lock(s_lic_mutex);
        if (s_toast_timer > 0.0f) {
            s_toast_timer -= dt;
            if (s_toast_timer < 0.0f) {
                s_toast_timer = 0.0f;
            }
        }
    }

} // namespace lic
