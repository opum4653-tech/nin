LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := minitouch-common

LOCAL_SRC_FILES := \
	minitouch.c

LOCAL_STATIC_LIBRARIES := \
	libevdev

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../vendor/libevdev/include \
	$(LOCAL_PATH)/../vendor/libevdev/source/include \
	$(LOCAL_PATH)/../vendor/libevdev/source/libevdev

include $(BUILD_STATIC_LIBRARY)
