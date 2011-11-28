LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/libstagefright/codecs/common/Config.mk
include frameworks/base/media/CedarX-Projects/Config.mk

LOCAL_SRC_FILES:=                         \
		CedarXAudioPlayer.cpp			  \
        CedarXPlayer.cpp				  \
        CedarXMetadataRetriever.cpp		  \
        CedarXMetaData.cpp				  \
        CedarXNativeRenderer.cpp		  \
        CedarXRecorder.cpp
		 

LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/frameworks/base/include/media/stagefright/openmax \
    $(CEDARX_TOP)/include \
    $(CEDARX_TOP)/libutil \
    $(CEDARX_TOP)/include/include_cedarv \
    $(CEDARX_TOP)/include/include_audio \
    ${CEDARX_TOP}/include/include_camera \
    $(TOP)/frameworks/base/media/libstagefright/include \
    $(TOP)/frameworks/base \
    $(TOP)/frameworks/base/include \
    $(TOP)/external/openssl/include

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libgui			  \
        libsurfaceflinger_client \
        libcamera_client \
        libstagefright_foundation \
        libicuuc \
		libsurfaceflinger_client \
		libskia 
#  		libskiagl 
	
ifeq ($(CEDARX_DEBUG_LEVEL),L0)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxstream \
	libcedarxrender \
	libsub \
	libsub_inline \
	libcedarv \
	libcedarv_osal \
	libvecore \
	libcedarxalloc \
	libjpgenc \
	libh264enc \
	libmp4_muxer \
	libdemux_cedarm \
	libdemux_asf \
	libdemux_avi \
	libdemux_flv \
	libdemux_mkv \
	libdemux_mov \
	libdemux_mpg \
	libdemux_rmvb \
	libdemux_ts \
	libdemux_pmp \
	libdemux_idxsub \
	libiconv \
	libcedarx_rtsp
	
ifeq ($(CEDARX_ENABLE_MEMWATCH),Y)
LOCAL_STATIC_LIBRARIES += libmemwatch
endif

else #L0

ifeq ($(CEDARX_DEBUG_LEVEL),L1)
LOCAL_STATIC_LIBRARIES += \
	libcedarxplayer \
	libcedarxcomponents \
	libcedarxdemuxers \
	libcedarxstream \
	libcedarxrender \
	libdemux_cedarm \
	libsub \
	libsub_inline \
	libiconv \
	libh264enc \
	libmp4_muxer \
	libjpgenc \
	libcedarx_rtsp
else #L2
LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxplayer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxcomponents.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxdemuxers.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxstream.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxrender.a	\
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_cedarm.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libsub.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libsub_inline.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libiconv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libh264enc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libmp4_muxer.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libjpgenc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarx_rtsp.a
	
endif

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarv_osal.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libvecore.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcedarxalloc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_asf.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_avi.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_flv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_mkv.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_mov.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_mpg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_rmvb.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_ts.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_pmp.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdemux_idxsub.a


ifneq ($(CEDARX_DEBUG_LEVEL),L0)
#LOCAL_LDFLAGS += \
#	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libm3u.a
endif

#LOCAL_LDFLAGS += \
#	$(OUT)/obj/STATIC_LIBRARIES/libft2_intermediates/libft2.a \
#	$(OUT)/obj/STATIC_LIBRARIES/libz_intermediates/libz.a

endif #end L0

#out/target/product/sun3i/obj/STATIC_LIBRARIES/libiconv_intermediates/libiconv.a

LOCAL_LDFLAGS += \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libac3_hw.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libdts_hw.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libwma.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libaac.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libmp3.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libatrc.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libcook.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libsipr.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libamr.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libape.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libogg.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libflac.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libwav.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libra.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libGetAudio_format.a \
	$(CEDARX_TOP)/../CedarAndroidLib/LIB_ICS_$(CEDARX_CHIP_VERSION)/libaacenc.a 


LOCAL_SHARED_LIBRARIES += libstagefright_foundation

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

LOCAL_MODULE:= libCedarX

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
