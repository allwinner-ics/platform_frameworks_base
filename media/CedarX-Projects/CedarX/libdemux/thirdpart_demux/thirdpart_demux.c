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
#define LOG_NDEBUG 0
#define LOG_TAG "thirdpart_demux"
#include <CDX_Debug.h>

#include <CDX_Common.h>
#include "cedarx_demux.h"
#include <cedarx_stream.h>

#define CDX_MUSIC_DEMUX_BUFFER_SIZE (1024*4)

struct cdx_stream_info *p_thirdpart_stream;

int thirdpart_demux_open(CedarXMediainfo *pMediaInfo, CedarXDataSourceDesc *datasrc_desc)
{
	p_thirdpart_stream = create_stream_handle(datasrc_desc);
	if(!p_thirdpart_stream)
		return CDX_ERROR;

	pMediaInfo->nHasAudio = 1;
	pMediaInfo->nHasSubtitle = 0;
	pMediaInfo->nHasVideo = 0;
	pMediaInfo->nStreamNum = 1;
	pMediaInfo->nFileSize = p_thirdpart_stream->getsize(p_thirdpart_stream);

	//printf("datasrc_desc->demux_type: %d\n",datasrc_desc->demux_type);

	switch(datasrc_desc->demux_type & 0xffff){
    case CDX_MEDIA_FILE_FMT_AAC:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_MPEG_AAC_LC;
		break;
    case CDX_MEDIA_FILE_FMT_AC3:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_AC3;
		break;
    case CDX_MEDIA_FILE_FMT_AMR:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_AMR;
		break;
	case CDX_MEDIA_FILE_FMT_APE:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_APE;
		break;
    case CDX_MEDIA_FILE_FMT_ATRC:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_ATRC;
		break;
    case CDX_MEDIA_FILE_FMT_DTS:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_DTS;
		break;
    case CDX_MEDIA_FILE_FMT_FLAC:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_FLAC;
		break;
    case CDX_MEDIA_FILE_FMT_MP3:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_MP3;
		break;
	case CDX_MEDIA_FILE_FMT_OGG:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_OGG;
		break;
    case CDX_MEDIA_FILE_FMT_WAV:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_PCM;
		break;
    case CDX_MEDIA_FILE_FMT_RA:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_RA;
		break;
	default:
		pMediaInfo->AudStrmList[0].codec_type = CDX_AUDIO_UNKNOWN;
		break;
	}

	return CDX_OK;
}

void thirdpart_demux_close()
{
	destory_stream_handle(p_thirdpart_stream);
}

int  thirdpart_demux_prefetch(CedarXPacket *cdx_pkt)
{
	cdx_pkt->pkt_length = CDX_MUSIC_DEMUX_BUFFER_SIZE;
	cdx_pkt->pkt_type = CDX_PacketAudio;
	cdx_pkt->pkt_pts = -1;

	return CDX_OK;
}

int thirdpart_demux_read(CedarXPacket *cdx_pkt)
{
	//static int seek_times = 0, thirdpart_read_times = 0;
	CDX_S32 read_length;
    //add for seek ,maybe to a function
	//printf("thirdpart read %d\n",thirdpart_read_times++);
    if(cdx_pkt->curr_offset !=cdx_pkt->file_offset)
    {
      //printf("----thirdpart seek offset: %lld-----\n",cdx_pkt->curr_offset - cdx_pkt->file_offset);
      //printf("******jump curr_offset=%lld,file_offset=%lld seek_times:%d\n",cdx_pkt->curr_offset,cdx_pkt->file_offset,seek_times++);
      if(cdx_seek(p_thirdpart_stream,cdx_pkt->curr_offset - cdx_pkt->file_offset, SEEK_CUR)){
    	  return CDX_ERROR;
      }
      cdx_pkt->file_offset = cdx_pkt->curr_offset ;
    }

	if (cdx_pkt->pkt_length <= cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_size0) {
		read_length = cdx_read(cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_buf0, 1,
				cdx_pkt->pkt_length, p_thirdpart_stream);
	} else {
		read_length = cdx_read(cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_buf0, 1,
				cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_size0, p_thirdpart_stream);
		read_length += cdx_read(cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_buf1, 1,
				cdx_pkt->pkt_length - cdx_pkt->pkt_info.epdk_read_pkt_info.pkt_size0,
				p_thirdpart_stream);
	}

	cdx_pkt->pkt_length = read_length;
    cdx_pkt->file_offset += read_length;
	if(read_length == 0){
		return CDX_ERROR;
	}

	return CDX_OK;
}

void thirdpart_demux_seek(CDX_S64 abs_seek_secs, int flags)
{
	return;
}

int  thirdpart_demux_control(int cmd, int cmd_sub, void *arg)
{
	return CDX_OK;
}

CedarXDemuxerAPI cdx_dmxs_thirdpart_0 = {
  .name = "thirdpart_dmx",
  .subname = "",
  .shortdesc = "all thirdpart demuxer packing",

  .open = thirdpart_demux_open,
  .close = thirdpart_demux_close,
  .prefetch = thirdpart_demux_prefetch,
  .read = thirdpart_demux_read,
  .seek = thirdpart_demux_seek,
  .control = thirdpart_demux_control,
};

CedarXDemuxerAPI cdx_dmxs_thirdpart_1 = {
  .name = "thirdpart_dmx",
  .subname = "",
  .shortdesc = "all thirdpart demuxer packing",

  .open = thirdpart_demux_open,
  .close = thirdpart_demux_close,
  .prefetch = thirdpart_demux_prefetch,
  .read = thirdpart_demux_read,
  .seek = thirdpart_demux_seek,
  .control = thirdpart_demux_control,
};

CedarXDemuxerAPI cdx_dmxs_thirdpart_2 = {
  .name = "thirdpart_dmx",
  .subname = "",
  .shortdesc = "all thirdpart demuxer packing",

  .open = thirdpart_demux_open,
  .close = thirdpart_demux_close,
  .prefetch = thirdpart_demux_prefetch,
  .read = thirdpart_demux_read,
  .seek = thirdpart_demux_seek,
  .control = thirdpart_demux_control,
};

CedarXDemuxerAPI cdx_dmxs_thirdpart_3 = {
  .name = "thirdpart_dmx",
  .subname = "",
  .shortdesc = "all thirdpart demuxer packing",

  .open = thirdpart_demux_open,
  .close = thirdpart_demux_close,
  .prefetch = thirdpart_demux_prefetch,
  .read = thirdpart_demux_read,
  .seek = thirdpart_demux_seek,
  .control = thirdpart_demux_control,
};
