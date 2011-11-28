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
#define LOG_TAG "idxsubdemux"
#include <CDX_Debug.h>

#include "epdk_demux.h"
#include "cedarx_demux.h"

extern EPDKDemuxerAPI epdk_demux_idxsub;
EPDKDemuxerAPI *idxsubdmx;
void  *idxsubdemux_handle;

int idxsubdemux_open(CedarXMediainfo *pMediaInfo, CedarXDataSourceDesc *datasrc_desc)
{
	idxsubdmx = &epdk_demux_idxsub;

	if(idxsubdmx->open((void **)&idxsubdemux_handle,0,datasrc_desc) != CDX_OK)
		return CDX_ERROR;

	if(idxsubdmx->getmediainfo(idxsubdemux_handle, pMediaInfo) != CDX_OK)
		return CDX_ERROR;

	idxsubdmx->control(idxsubdemux_handle, CDX_DMX_CMD_MEDIAMODE_CONTRL, CDX_MEDIA_STATUS_PLAY, 0);

	return CDX_OK;
}

void idxsubdemux_close()
{
	idxsubdmx->close((void **)&idxsubdemux_handle);
}

int  idxsubdemux_prefetch(CedarXPacket *cdx_pkt)
{
	return idxsubdmx->prefetch((void*)idxsubdemux_handle,cdx_pkt);
}

int idxsubdemux_read(CedarXPacket *cdx_pkt)
{
	if(cdx_pkt != NULL && cdx_pkt->is_dummy_packet)
		return idxsubdmx->read_dummy((void*)idxsubdemux_handle, cdx_pkt);
	else
		return idxsubdmx->read((void*)idxsubdemux_handle, cdx_pkt);
}

void idxsubdemux_seek(CDX_S64 abs_seek_secs, int flags)
{
	return;
}

int  idxsubdemux_control(int cmd, int cmd_sub, void *arg)
{
	return idxsubdmx->control((void*)idxsubdemux_handle, cmd, cmd_sub,arg);
}

CedarXDemuxerAPI cdx_dmxs_idxsub = {
  .name = "idxsubdmx",
  .subname = "",
  .shortdesc = "all epdk demuxer packing",

  .open = idxsubdemux_open,
  .close = idxsubdemux_close,
  .prefetch = idxsubdemux_prefetch,
  .read = idxsubdemux_read,
  .seek = idxsubdemux_seek,
  .control = idxsubdemux_control,
};
