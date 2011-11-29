/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#ifndef CDX_PlayerAPI_H_
#define CDX_PlayerAPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    CDX_CMD_NOP           =  0, // interface test message
    CDX_CMD_GET_VERSION       ,
    CDX_SET_DATASOURCE_URL    ,
    CDX_SET_DATASOURCE_FD	  ,
    CDX_CMD_GET_POSITION	  ,
    CDX_CMD_GET_DURATION	  ,

    CDX_CMD_PREPARE			  ,
    CDX_CMD_START			  ,
    CDX_CMD_TAG_START		  ,
    CDX_CMD_STOP 			  ,
    CDX_CMD_PAUSE			  ,
    CDX_CMD_SEEK			  ,

    CDX_CMD_PREPARE_ASYNC     , //12
    CDX_CMD_START_ASYNC  	  ,
    CDX_CMD_TAG_START_ASYNC	  ,
    CDX_CMD_STOP_ASYNC		  ,
    CDX_CMD_PAUSE_ASYNC		  ,
    CDX_CMD_SEEK_ASYNC        ,

    CDX_CMD_RESUME			  ,
    CDX_CMD_RESET			  ,
    CDX_CMD_GETSTATE		  ,
    CDX_CMD_REGISTER_CALLBACK ,
    CDX_CMD_GET_MEDIAINFO     ,
    CDX_CMD_SWITCH_AUDIO      ,
    CDX_CMD_SWITCH_SUBTITLE   ,
    CDX_CMD_SELECT_AUDIO_OUT  ,
    CDX_CMD_CAPTURE_THUMBNAIL ,
    CDX_CMD_CLOSE_CAPTURE     ,
    CDX_CMD_QUIT_RETRIVER     ,
    CDX_CMD_GET_METADATA 	  ,
    CDX_CMD_SET_STREAM_TYPE   ,
    CDX_CMD_GET_STREAM_TYPE   ,
    CDX_CMD_SET_VOLUME        ,

	CDX_CMD_SEND_BUF		 ,
	CDX_CMD_SET_SAVE_FILE	 ,
	CDX_CMD_SET_VIDEO_INFO	 ,
	CDX_CMD_SET_AUDIO_INFO	 ,
	CDX_CMD_SET_REC_MODE	 ,
	CDX_CMD_SET_PREVIEW_INFO ,
	CDX_CMD_GET_FILE_SIZE	 ,

    CDX_CMD_GETSUBCOUNT,
    CDX_CMD_GETSUBLIST,
    CDX_CMD_GETCURSUB,
    CDX_CMD_SWITCHSUB,
    CDX_CMD_SETSUBGATE,
    CDX_CMD_GETSUBGATE,
    CDX_CMD_SETSUBCOLOR,
    CDX_CMD_GETSUBCOLOR,
    CDX_CMD_SETSUBFRAMECOLOR,
    CDX_CMD_GETSUBFRAMECOLOR,
    CDX_CMD_SETSUBFONTSIZE,
    CDX_CMD_GETSUBFONTSIZE,
    CDX_CMD_SETSUBCHARSET,
    CDX_CMD_GETSUBCHARSET,
    CDX_CMD_SETSUBPOSITION,
    CDX_CMD_GETSUBPOSITION,
    CDX_CMD_SETSUBDELAY,
    CDX_CMD_GETSUBDELAY,
    CDX_CMD_GETTRACKCOUNT,
    CDX_CMD_GETTRACKLIST,
    CDX_CMD_GETCURTRACK,
    CDX_CMD_SWITCHTRACK,

	CDX_CMD_SET_DECODER_SOURCE_3D_FORMAT,
	CDX_CMD_SET_DISPLAY_MODE,
	CDX_CMD_SET_3D_DISPLAY_FORMAT,
	CDX_CMD_SET_ANAGLAGH_TYPE,
	CDX_CMD_GET_ALL_DISPLAY_MODE,
	CDX_CMD_SET_CALLBACK,
	CDX_CMD_DATA_READY,
	CDX_CMD_IS_REQUESING_DATA,
	
	CDX_CMD_REGISTER_DEMUXER = 0x800,
	CDX_CMD_SELECT_DEMUXER   ,
}CEDARX_COMMAND_TYPE;

typedef enum CEDARX_EVENT_TYPE{
	CDX_EVENT_NONE                 = 0,
    CDX_EVENT_PREPARED             = 1,
    CDX_EVENT_PLAYBACK_COMPLETE    = 2,
    CDX_EVENT_SEEK_COMPLETE        = 4,
    CDX_EVENT_FATAL_ERROR          = 8,
    CDX_MEDIA_INFO_BUFFERING_START = 16,
    CDX_MEDIA_INFO_BUFFERING_END   = 32,
    CDX_MEDIA_BUFFERING_UPDATE     = 64,
    CDX_MEDIA_INFO_SRC_3D_MODE,

	CDX_EVENT_VIDEORENDERINIT    = 65536,
	CDX_EVENT_VIDEORENDERDATA	  ,
	CDX_EVENT_VIDEORENDERGETDISPID,
	CDX_EVENT_AUDIORENDERINIT     ,
	CDX_EVENT_AUDIORENDEREXIT     ,
	CDX_EVENT_AUDIORENDERDATA     ,
	CDX_EVENT_AUDIORENDERGETSPACE ,
	CDX_EVENT_AUDIORENDERGETDELAY ,
}CEDARX_EVENT_TYPE;

typedef enum CEDARX_STATES{
	CDX_STATE_UNKOWN   = 0,
	CDX_STATE_IDEL        ,
	CDX_STATE_PAUSE       ,
	CDX_STATE_EXCUTING    ,
	CDX_STATE_SEEKING     ,
	CDX_STATE_PREPARING   ,
	CDX_STATE_STOPPED	  ,
}CEDARX_STATES;

typedef enum CEDARX_AUDIO_OUT_TYPE{
	CDX_AUDIO_OUT_UNKOWN = 0,
	CDX_AUDIO_OUT_DAC       ,
	CDX_AUDIO_OUT_I2S       ,
}CEDARX_AUDIO_OUT_TYPE;

typedef enum CEDARX_STREAMTYPE{
	CEDARX_STREAM_NETWORK,
	CEDARX_STREAM_LOCALFILE,
	CEDARX_STREAM_EXTERNAL_BUFFER,
}CEDARX_STREAMTYPE;

typedef enum CEDARX_THIRDPART_DEMUXER_TYPE{
	CEDARX_THIRDPART_DEMUXER_0 = 0x70000001,
	CEDARX_THIRDPART_DEMUXER_1,
	CEDARX_THIRDPART_DEMUXER_2,
	CEDARX_THIRDPART_DEMUXER_3,
}CEDARX_THIRDPART_DEMUXER_TYPE;

#define CEDARX_MAX_AUDIO_STREAM_NUM    16
#define CEDARX_MAX_VIDEO_STREAM_NUM     1
#define CEDARX_MAX_SUBTITLE_STREAM_NUM 16

typedef struct CedarXExternFdDesc{
	int		fd;   //SetDataSource FD
	long long offset;
	long long length;
}CedarXExternFdDesc;

typedef enum {
	VIDEO_THUMB_UNKOWN = 0,
	VIDEO_THUMB_JPEG,
	VIDEO_THUMB_YUVPLANNER,
	VIDEO_THUMB_RGB565,
}VIDEOTHUMBNAILFORMAT;

typedef struct VideoThumbnailInfo{
	int format; //0: JPEG STREAM  1: YUV RAW STREAM 2:RGB565
	int capture_time; //
	int require_width;
	int require_height;
	void *thumb_stream_address;
	int thumb_stream_size;
	int capture_result; //0:fail 1:ok
}VideoThumbnailInfo;

typedef enum {
	RECORDER_MODE_AUDIO		= 1	,		// only audio recorder
	RECORDER_MODE_VIDEO		= 2	,		// only video recorder
	RECORDER_MODE_CAMERA	= 3 ,		// audio and video recorder
}RECORDER_MODE;

typedef enum CEDARX_MEDIA_TYPE{
	CEDARX_MEDIATYPE_NORMAL = 0 ,
	CEDARX_MEDIATYPE_RAWMUSIC   ,
	CEDARX_MEDIATYPE_3D_VIDEO   ,
}CEDARX_MEDIA_TYPE;

typedef struct CedarXMediaInformations
{
    unsigned char mVideoStreamCount;
    unsigned char mAudioStreamCount;
    unsigned char mSubtitleStreamCount;
    unsigned int  mDuration;
    unsigned int  mFlags; //CanSeek etc.
    unsigned int  media_type;
    unsigned int  media_subtype_3d;
    unsigned int  source_3d_mode;

    struct CedarXAudioInfo {
    	char    cMIMEType[16];
        int     mChannels;
        int     mSampleRate;
        int     mAvgBitrate;
    }mAudioInfo[CEDARX_MAX_AUDIO_STREAM_NUM];

    struct CedarXVideoInfo {
    	char    cMIMEType[16];
        int 	mFrameWidth;
        int 	mFrameHeight;
    }mVideoInfo[CEDARX_MAX_VIDEO_STREAM_NUM];

    struct CedarXSubtitleInfo {
    	char    cMIMEType[16];
        char    mLang[32];
    }mSubtitleInfo[CEDARX_MAX_SUBTITLE_STREAM_NUM];

} CedarXMediaInformations;

int CDXPlayer_Create(void **inst);
void CDXPlayer_Destroy(void *player);
int CDXRetriever_Create(void **inst);
void CDXRetriever_Destroy(void *retriever);

typedef struct CDXPlayer
{
	void *context;
	int  (*control)(void *player, int cmd, unsigned int para0, unsigned int para1);
}CDXPlayer;

typedef struct CDXRetriever
{
	void *context;
	int  (*control)(void *retriever, int cmd, unsigned int para0, unsigned int para1);
}CDXRetriever;

typedef int (*CedarXCallbackType)(void *cookie, CEDARX_EVENT_TYPE event, void *p_event_data);

typedef struct CedarXPlayerCallbackType{
	void *cookie;
	CedarXCallbackType callback;
}CedarXPlayerCallbackType;

// recorder
#ifdef __OS_ANDROID
int CDXRecorder_Init(void *);
#else
int CDXRecorder_Init(void);
#endif
int CDXRecorder_Exit();
int CDXRecorder_Control(int cmd, unsigned int para0, unsigned int para1);
typedef int (* reqdata_from_dram)(unsigned char *pbuf, unsigned int buflen);

#ifdef __cplusplus
}
#endif

#endif
/* File EOF */