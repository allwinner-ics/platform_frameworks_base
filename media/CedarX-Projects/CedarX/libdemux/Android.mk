LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/CedarX-Projects/Config.mk

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	cedarx_demux.c \
	epdk_demux/epdk_demux.c \
	music_demux.c \
	idxsub_demux.c \
	stagefright_demux.cpp
		
LOCAL_C_INCLUDES := \
	${CEDARX_TOP}/include/include_platform/CHIP_$(CEDARX_CHIP_VERSION) \
	${CEDARX_TOP}/include/include_cedarv \
	${CEDARX_TOP}/include \
	${CEDARX_TOP}/framework \
	${CEDARX_TOP}/libdemux/epdk_demux \
	${CEDARX_TOP}/libdemux \
	${CEDARX_TOP}/libdemux/cedar_demux \
	${CEDARX_TOP}/libutil \
	${CEDARX_TOP}/libstream \
	${CEDARX_TOP}/libcodecs/include \
	${CEDARX_TOP}/libcodecs/libvdcedar \
	${CEDARX_TOP}/librender \
	${CEDARX_TOP}/include/include_cedarv \
	${CEDARX_TOP}/libsub/include \
	$(TOP)/frameworks/base/media/libstagefright/include 

LOCAL_MODULE_TAGS := optional
 
LOCAL_CFLAGS += -D__OS_ANDROID -D__CDX_ENABLE_NETWORK
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)
#LOCAL_CFLAGS += -march=armv6j

LOCAL_MODULE:= libcedarxdemuxers

#	cp ../include/CDX_PlayerAPI.h ../../CedarXAndroid/

include $(BUILD_STATIC_LIBRARY)

