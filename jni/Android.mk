LOCAL_PATH := $(call my-dir)
ROOT_JNI_PATH := $(LOCAL_PATH)

# ============================================================
# Prebuilt: libcurl (Official Network Library)
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := ../includes/network/curl/lib/$(TARGET_ARCH_ABI)/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../includes/network/curl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Prebuilt: OpenSSL libssl
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_SRC_FILES := ../includes/network/openssl/lib/$(TARGET_ARCH_ABI)/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../includes/network/openssl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Prebuilt: OpenSSL libcrypto
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := ../includes/network/openssl/lib/$(TARGET_ARCH_ABI)/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../includes/network/openssl/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# Sub-module: libevdev (Touch & Evdev Static Library)
# ============================================================
include $(ROOT_JNI_PATH)/vendor/libevdev/Android.mk

# Restore ROOT_JNI_PATH for main executable
LOCAL_PATH := $(ROOT_JNI_PATH)

# ============================================================
# MAIN MODULE — gemini-cli
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := gemini-cli

# ─── SOURCE FILES ───
# Auto-collect semua *.cpp di src/ dan includes/draw/
# Kecuali: imgui_demo, imgui_impl_vulkan, stb_image
LOCAL_SRC_FILES := $(filter-out \
    ../includes/draw/ImGui/imgui_demo.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_vulkan.cpp \
    ../includes/draw/ImGui/stb_image.cpp, \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../src -name "*.cpp")) \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../includes/draw -name "*.cpp")))

# ─── INCLUDE DIRS ───
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/vendor/glm \
    $(LOCAL_PATH)/vendor/libevdev/include \
    $(LOCAL_PATH)/vendor/libevdev/source/include \
    $(LOCAL_PATH)/vendor/libevdev/source/libevdev \
    $(LOCAL_PATH)/minitouch \
    $(LOCAL_PATH)/../includes \
    $(LOCAL_PATH)/../includes/network \
    $(LOCAL_PATH)/../includes/network/curl/include \
    $(LOCAL_PATH)/../includes/network/openssl/include \
    $(LOCAL_PATH)/../includes/network/json \
    $(LOCAL_PATH)/../includes/font \
    $(LOCAL_PATH)/../includes/internal \
    $(LOCAL_PATH)/../includes/internal/ImGui \
    $(LOCAL_PATH)/../includes/internal/ImGui/backends \
    $(LOCAL_PATH)/../includes/internal/Android_draw \
    $(LOCAL_PATH)/../includes/internal/Android_touch \
    $(LOCAL_PATH)/../includes/internal/native_surface \
    $(LOCAL_PATH)/../includes/draw/ImGui \
    $(LOCAL_PATH)/../includes/draw/ImGui/backends \
    $(LOCAL_PATH)/../includes/protect \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/ui \
    $(LOCAL_PATH)/../src/ui/png \
    $(LOCAL_PATH)/../src/ui/png/images \
    $(LOCAL_PATH)/../src/ui/styles \
    $(LOCAL_PATH)/../src/ui/widgets \
    $(LOCAL_PATH)/../src/func \
    $(LOCAL_PATH)/../src/game \
    $(LOCAL_PATH)/../src/other \
    $(LOCAL_PATH)/../src/protect \
    $(LOCAL_PATH)/../src/offsets \
    $(LOCAL_PATH)/../src/aim

# ═══════════════════════════════════════════════════════════
#  C++ FLAGS
# ═══════════════════════════════════════════════════════════
LOCAL_CFLAGS += -DUSE_OPENGL
LOCAL_CPPFLAGS := \
    -DUSE_OPENGL \
    -std=c++17 \
    -fexceptions \
    -frtti \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden \
    -Oz \
    -ffunction-sections \
    -fdata-sections \
    -fomit-frame-pointer \
    -Wno-error=format-security \
    -fno-color-diagnostics \
    -fmerge-all-constants \
    -fno-ident

# ─── LINKER FLAGS ───
LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -Wl,--no-undefined \
    -pie \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,-z,noexecstack \
    -Wl,-z,max-page-size=16384

# ─── STATIC LIBS ───
LOCAL_STATIC_LIBRARIES := curl ssl crypto libevdev

# ─── SYSTEM LIBS ───
LOCAL_LDLIBS := \
    -llog \
    -lEGL \
    -lGLESv3 \
    -lz \
    -ldl

include $(BUILD_EXECUTABLE)
