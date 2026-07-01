LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := token
LOCAL_SRC_FILES := $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
LOCAL_CPP_FEATURES := exceptions
LOCAL_CFLAGS := -D_GNU_SOURCE
LOCAL_CPPFLAGS := -std=c++17
LOCAL_LDLIBS := -llog -lc++_static -latomic

include $(BUILD_EXECUTABLE)
