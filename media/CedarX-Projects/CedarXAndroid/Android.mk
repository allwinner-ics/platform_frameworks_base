LOCAL_PATH:= $(call my-dir)

$(info $(PLATFORM_VERSION))

ifeq ($(PLATFORM_VERSION),4.0.1)
include $(LOCAL_PATH)/IceCreamSandwich/Android.mk
else
include $(LOCAL_PATH)/Gingerbread/Android.mk
endif
