LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/libstagefright/codecs/common/Config.mk
include frameworks/base/media/CedarX-Projects/Config.mk

LOCAL_SRC_FILES:=                         \
		CedarARender.cpp \
        CedarAPlayer.cpp				  


LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/include \
	${CEDARX_TOP}/libcodecs/include \
	$(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/frameworks/base/include/media/stagefright/openmax

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui

ifeq ($(CEDARX_DEBUG_LEVEL),L2)
LOCAL_LDFLAGS += \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedara_decoder.a
endif

LOCAL_LDFLAGS += \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libac3.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdts.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libwma.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libaac.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libmp3.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libatrc.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcook.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libsipr.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libamr.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libape.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libogg.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libflac.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libwav.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libGetAudio_format.a \
	$(LOCAL_PATH)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libaacenc.a

ifneq ($(CEDARX_DEBUG_LEVEL),L2)
LOCAL_STATIC_LIBRARIES += \
	libcedara_decoder
endif

ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread -ldl
        LOCAL_SHARED_LIBRARIES += libdvm
        LOCAL_CPPFLAGS += -DANDROID_SIMULATOR
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
        LOCAL_LDLIBS += -lpthread
endif

LOCAL_CFLAGS += -Wno-multichar 

ifeq ($(PLATFORM_VERSION),"2.3")
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3
else
LOCAL_CFLAGS += -D__ANDROID_VERSION_2_3_4
endif
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libCedarA

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
