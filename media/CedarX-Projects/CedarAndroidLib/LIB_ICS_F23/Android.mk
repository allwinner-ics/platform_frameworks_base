LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libcedarxosal.so libcedarxbase.so libcedarv.so libswdrm.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
