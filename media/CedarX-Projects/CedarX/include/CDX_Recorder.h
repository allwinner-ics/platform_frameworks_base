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
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _CDX_RECORDER_H_
#define _CDX_RECORDER_H_

#include <stdio.h>
#include <tsemaphore.h>
#include <CDX_Types.h>
#include <CDX_Component.h>
#include <CDX_Common.h>
#include <CDX_PlayerAPI.h>

typedef struct CedarXRecorderContext{
	CDX_S32 init_flags;
	CDX_S32 flags;
	CEDARX_STATES states;

	void * cookie;
	OMX_PTR pAppData;
	RECORDER_MODE mode;

	tsem_t tsem_recorder_source_cmd;
	tsem_t tsem_video_encoder_cmd;
	tsem_t tsem_audio_encoder_cmd;
	tsem_t tsem_recorder_render_cmd;

	OMX_HANDLETYPE hnd_recorder_source;
	OMX_HANDLETYPE hnd_video_encoder;
	OMX_HANDLETYPE hnd_audio_encoder;
	OMX_HANDLETYPE hnd_recorder_render;
}CedarXRecorderContext;

#endif	// _CDX_RECORDER_H_

#ifdef __cplusplus
}
#endif /* __cplusplus */

