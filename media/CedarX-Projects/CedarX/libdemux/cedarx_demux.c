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

//#define LOG_NDEBUG 0
#define LOG_TAG "cedarx_demux"
#include <CDX_Debug.h>

#include "cedarx_demux.h"
#include "epdk_demux/epdk_demux.h"
#ifdef __CDX_ENABLE_SUBTITLE
#include "sub_dmx.h"
#endif 
#include "net_demux.h"
extern CedarXDemuxerAPI cdx_dmxs_music;
extern CedarXDemuxerAPI cdx_dmxs_idxsub;

CedarXDemuxerAPI* cedarx_demuxers[] =
{
	&cdx_dmxs_music,
	&cdx_dmxs_epdk,
#ifdef __CDX_ENABLE_NETWORK
	&cdx_dmxs_network,
#ifdef	__OS_ANDROID
	&cdx_dmxs_rtsp,
#endif
#endif
#ifdef __CDX_ENABLE_SUBTITLE
	&cdx_dmxs_idxsub,
#endif
	0
};

enum CEDARXDEMUXERTYPES{
	ID_CDX_DMXS_MUSIC = 0,
	ID_CDX_DMXS_EPDK,
#ifdef __CDX_ENABLE_NETWORK
	ID_CDX_DMXS_NETWORK,
#ifdef	__OS_ANDROID
	ID_CDX_DMXS_RTSP,
#endif
#endif
	ID_CDX_DMXS_IDXSUB
};

CedarXDemuxerAPI *cedarx_demuxer_handle;

#ifdef __OS_LINUX
extern CedarXDemuxerAPI cdx_dmxs_thirdpart_0;
extern CedarXDemuxerAPI cdx_dmxs_thirdpart_1;
extern CedarXDemuxerAPI cdx_dmxs_thirdpart_2;
extern CedarXDemuxerAPI cdx_dmxs_thirdpart_3;
#endif

CedarXDemuxerAPI *cedarx_demux_create(int demux_type)
{
#ifdef __OS_LINUX
	if(demux_type >= CEDARX_THIRDPART_DEMUXER_0) {
		switch(demux_type) {
		case CEDARX_THIRDPART_DEMUXER_0:
			cedarx_demuxer_handle = &cdx_dmxs_thirdpart_0;
			break;
		case CEDARX_THIRDPART_DEMUXER_1:
			cedarx_demuxer_handle = &cdx_dmxs_thirdpart_1;
			break;
		case CEDARX_THIRDPART_DEMUXER_2:
			cedarx_demuxer_handle = &cdx_dmxs_thirdpart_2;
			break;
		case CEDARX_THIRDPART_DEMUXER_3:
			cedarx_demuxer_handle = &cdx_dmxs_thirdpart_3;
			break;
		default:
			LOGE("Unknown 3rd demuxer!");
			break;
		}
	}
	else
#endif
	if(demux_type & CDX_MEDIA_FILE_FMT_AUDIO) {
		cedarx_demuxer_handle = cedarx_demuxers[ID_CDX_DMXS_MUSIC];
	}
	else if(demux_type & CDX_MEDIA_FILE_FMT_NETWORK) {
		cedarx_demuxer_handle = cedarx_demuxers[ID_CDX_DMXS_NETWORK];
	}
#ifdef	__OS_ANDROID
	else if(demux_type & CDX_MEDIA_FILE_FMT_NETWORK_RTSP){
		cedarx_demuxer_handle = cedarx_demuxers[ID_CDX_DMXS_RTSP];
	}
#endif
#ifdef __CDX_ENABLE_SUBTITLE
	else if(demux_type & CDX_MEDIA_FILE_FMT_IDXSUB){
		cedarx_demuxer_handle = cedarx_demuxers[ID_CDX_DMXS_IDXSUB];
	}
#endif
	else {
		cedarx_demuxer_handle = cedarx_demuxers[ID_CDX_DMXS_EPDK];
	}

	return cedarx_demuxer_handle;
}

