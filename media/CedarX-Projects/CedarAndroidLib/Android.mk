LOCAL_PATH:= $(call my-dir)

include frameworks/base/media/CedarX-Projects/Config.mk

ifeq ($(CEDARX_DEBUG_LEVEL),L2)
ifeq ($(PLATFORM_VERSION),4.0.1)
include $(LOCAL_PATH)/LIB_ICS_F23/Android.mk
else
include $(LOCAL_PATH)/LIB_F23/Android.mk
endif
endif
