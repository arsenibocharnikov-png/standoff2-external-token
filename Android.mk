LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := token
LOCAL_SRC_FILES := src/main.cpp src/memory/memory.cpp src/process_manager/process_manager.cpp
LOCAL_CFLAGS := -D_GNU_SOURCE
LOCAL_CPPFLAGS := -std=c++17
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
