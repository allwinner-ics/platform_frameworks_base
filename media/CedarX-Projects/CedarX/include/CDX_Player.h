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

#ifndef CDX_Player_H_
#define CDX_Player_H_

#include <stdio.h>
#include <CDX_Types.h>
#include <CDX_Component.h>
#include <CDX_Common.h>
#include <CDX_PlayerAPI.h>
#include <cedarx_demux.h>
#include <GetAudio_format.h>
#include <CDX_Resource_Manager.h>
#include <tmessage.h>
#include <tsemaphore.h>

typedef struct CedarXPlayerContext{
	CedarXDataSourceDesc data_source_desc;
	CDX_S32 init_flags;
	CDX_S32 flags;
	CDX_S32 exit_flags;
	CDX_S32 exit_flags_backup;
	CDX_S32 eof_flags;
	CDX_S32 msg_queue_thread_exit;
	CDX_S32 playback_complete_flag;
	CDX_S32 position_invalid;
	CDX_S64 seek_position;
	CEDARX_STATES states;
	CEDARX_STATES transient_states;
	CEDARX_AUDIO_OUT_TYPE audio_out_type;
	CDX_S64 pause_position;
	CDX_S32 is_manual_pause;

	void *cdx_player;
	OMX_PTR pAppData;
	CedarXPlayerCallbackType callback_info;
	CedarXMediainfo cdx_detail_mediainfo;
	CedarXMediaInformations cdx_mediainfo;
	VideoThumbnailInfo vd_thumb_info;
	OMX_S32 file_fmt_type;
	OMX_S32 is_hardware_init;
	OMX_S32 fatal_error;
	audio_file_info_t audio_metadata;
	CEDARV_REQUEST_CONTEXT cedarv_req_ctx;

	OMX_S32 sub_enable;
	OMX_S32 curr_subtrack_idx;
	OMX_S32 sub_position;
	OMX_S32 sub_delay;
	OMX_S32 curr_audiotrack_idx;

	CDX_TUNNELLINKTYPE cdx_tunnels_link;

	pthread_mutex_t cdx_player_mutex;
	pthread_t thread_id;
	message_quene_t  msg_queue;
	tsem_t tsem_wait_message;
	int msg_id_processed;
	int msg_id_index;

	tsem_t tsem_demux_cmd;
	tsem_t tsem_video_decoder_cmd;
	tsem_t tsem_audio_decoder_cmd;
	tsem_t tsem_video_render_cmd;
	tsem_t tsem_audio_render_cmd;
	tsem_t tsem_clock_cmd;
	tsem_t tsem_subtitle_cmd;

	OMX_HANDLETYPE hnd_demuxer;
	OMX_HANDLETYPE hnd_video_decoder;
	OMX_HANDLETYPE hnd_audio_decoder;
	OMX_HANDLETYPE hnd_video_render;
	OMX_HANDLETYPE hnd_audio_render;
	OMX_HANDLETYPE hnd_clock;
	OMX_HANDLETYPE hnd_subtitle;

	cdx_3d_mode_e		original_3d_mode;
	cdx_3d_mode_e 		source_3d_mode;
	cdx_display_mode_e	display_mode;
	cdx_anaglagh_e		anaglagh_type;
	OMX_U32				anaglagh_enable;
	OMX_U32				display_3d_enable;

	OMX_S32 	cedarv_rotation;
	OMX_S32 	cedarv_max_width;
	OMX_S32 	cedarv_max_height;
	OMX_S32 	cedarv_output_yuv_mode;
}CedarXPlayerContext;

#include "CDX_PlayerAPI.h"

typedef struct CedarXMediaRetriverContext{
	int bIsCaptureInit;
	audio_file_info_t audio_metadata;
	CedarXDataSourceDesc data_source_desc;
	CEDARV_REQUEST_CONTEXT cedarv_req_ctx;
}CedarXMediaRetriverContext;

//enum CedarXPlayerMessage
//{
//	CDX_MESSAGE_NOP = 0,
//	CDX_MESSAGE_RESET,
//	CDX_MESSAGE_STOP,
//	CDX_MESSAGE_QUIT,
//	CDX_MESSAGE_PREPARE,
//	CDX_MESSAGE_SEEK,
//	CDX_MESSAGE_PAUSE,
//	CDX_MESSAGE_CONTROL_PAUSE,
//	CDX_MESSAGE_CONTROL_START,
//	CDX_MESSAGE_STREAM_EOF,
//};

#endif
/* File EOF */
