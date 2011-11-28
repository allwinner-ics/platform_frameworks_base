LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/CedarX-Projects/Config.mk
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	cedarx_stream.c \
	cedarx_stream_file.c
	
#	../librender/video_render_linux_$(CEDARX_CHIP_VERSION).c
		
LOCAL_C_INCLUDES := \
		${CEDARX_TOP}/include \
		${CEDARX_TOP}/libstream \
		${CEDARX_TOP}/include/include_cedarv \
		${CEDARX_TOP}/libutil
LOCAL_MODULE_TAGS := optional
 
LOCAL_CFLAGS += $(CEDARX_EXT_CFLAGS)

LOCAL_MODULE:= libcedarxstream

include $(BUILD_STATIC_LIBRARY)

