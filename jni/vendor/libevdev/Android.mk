LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libevdev

LOCAL_SRC_FILES := \
    source/libevdev/libevdev.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/source/include \
    $(LOCAL_PATH)/source/libevdev

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/source/include \
    $(LOCAL_PATH)/source/libevdev

include $(BUILD_STATIC_LIBRARY)
