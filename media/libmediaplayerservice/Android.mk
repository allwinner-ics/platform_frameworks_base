LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    MediaRecorderClient.cpp     \
    MediaPlayerService.cpp      \
    MetadataRetrieverClient.cpp \
    TestPlayerStub.cpp          \
    MidiMetadataRetriever.cpp   \
    MidiFile.cpp                \
    CedarPlayer.cpp       		\
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp		\
    CedarAPlayerWrapper.cpp		\
    SimpleMediaFormatProbe.cpp

LOCAL_SHARED_LIBRARIES :=     		\
	libcutils             			\
	libutils              			\
	libbinder             			\
	libvorbisidec         			\
	libsonivox            			\
	libmedia              			\
	libcamera_client      			\
	libandroid_runtime    			\
	libCedarX           			\
	libCedarA           			\
	libstagefright        			\
	libstagefright_omx    			\
	libstagefright_foundation       \
	libgui                          \
	libdl

LOCAL_STATIC_LIBRARIES := \
        libstagefright_nuplayer                 \
        libstagefright_rtsp                     \

LOCAL_C_INCLUDES :=                                                 \
	$(JNI_H_INCLUDE)                                                \
	$(call include-path-for, graphics corecg)                       \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarXAndroid/IceCreamSandwich \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarX/include \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarX/include/include_audio \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarX/include/include_cedarv \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarA \
	$(TOP)/frameworks/base/media/CedarX-Projects/CedarA/include \
	$(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/frameworks/base/media/libstagefright/include             \
	$(TOP)/frameworks/base/media/libstagefright/rtsp                \
        $(TOP)/external/tremolo/Tremolo \

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

