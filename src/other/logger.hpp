#pragma once

#include <android/log.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <string>

#define NIN_LOG_TAG "NINMODS_NATIVE"
#define NIN_LOG_FILE "/data/local/tmp/ninmods.log"

namespace nin_log {

    inline std::mutex s_logMutex;

    inline void write_log(int priority, const char* levelStr, const char* tag, const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // 1. Android Logcat
        __android_log_write(priority, tag, buffer);

        // 2. Terminal stdout
        time_t now = time(nullptr);
        struct tm* tm_info = localtime(&now);
        char timeStr[32];
        if (tm_info) {
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
        } else {
            snprintf(timeStr, sizeof(timeStr), "00:00:00");
        }

        std::lock_guard<std::mutex> lock(s_logMutex);
        printf("[%s][%s][%s] %s\n", timeStr, tag, levelStr, buffer);
        fflush(stdout);

        // 3. Persistent Diagnostic Log File
        FILE* fp = fopen(NIN_LOG_FILE, "a");
        if (fp) {
            fprintf(fp, "[%s][%s][%s] %s\n", timeStr, tag, levelStr, buffer);
            fflush(fp);
            fclose(fp);
        }
    }

} // namespace nin_log

#define LOG_INFO(...)  ::nin_log::write_log(ANDROID_LOG_INFO,  "INFO",  NIN_LOG_TAG, __VA_ARGS__)
#define LOG_DEBUG(...) ::nin_log::write_log(ANDROID_LOG_DEBUG, "DEBUG", NIN_LOG_TAG, __VA_ARGS__)
#define LOG_WARN(...)  ::nin_log::write_log(ANDROID_LOG_WARN,  "WARN",  NIN_LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) ::nin_log::write_log(ANDROID_LOG_ERROR, "ERROR", NIN_LOG_TAG, __VA_ARGS__)
