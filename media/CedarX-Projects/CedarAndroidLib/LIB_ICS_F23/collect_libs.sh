#!/bin/sh

#SRCDIR0=~/Develope/android2.3/out/target/product/generic/obj/STATIC_LIBRARIES
#SRCDIR0=~/roller/trunk/android2.3/out/target/product/sun3i/obj/STATIC_LIBRARIES
SRCDIR0=~/workspace/android4.0.1/out/target/product/crane-evb/obj/STATIC_LIBRARIES

echo "--------------------------------------------------"
echo collect dir is: ${SRCDIR0}
echo "--------------------------------------------------"

cp $SRCDIR0/libdemux_asf_intermediates/libdemux_asf.a   ./
cp $SRCDIR0/libdemux_avi_intermediates/libdemux_avi.a   ./
cp $SRCDIR0/libdemux_ts_intermediates/libdemux_ts.a     ./
cp $SRCDIR0/libdemux_mkv_intermediates/libdemux_mkv.a   ./
cp $SRCDIR0/libdemux_mov_intermediates/libdemux_mov.a   ./
cp $SRCDIR0/libdemux_flv_intermediates/libdemux_flv.a   ./
cp $SRCDIR0/libdemux_mpg_intermediates/libdemux_mpg.a   ./
cp $SRCDIR0/libdemux_rmvb_intermediates/libdemux_rmvb.a ./
cp $SRCDIR0/libdemux_pmp_intermediates/libdemux_pmp.a ./
cp $SRCDIR0/libdemux_idxsub_intermediates/libdemux_idxsub.a ./
cp $SRCDIR0/libcedarxdemuxers_intermediates/libcedarxdemuxers.a ./
cp $SRCDIR0/libcedarxstream_intermediates/libcedarxstream.a ./
cp $SRCDIR0/libcedarxrender_intermediates/libcedarxrender.a ./
cp $SRCDIR0/libcedarxcomponents_intermediates/libcedarxcomponents.a ./
cp $SRCDIR0/libvecore_intermediates/libvecore.a         ./
cp $SRCDIR0/libcedarv_osal_intermediates/libcedarv_osal.a ./
cp $SRCDIR0/libcedarxalloc_intermediates/libcedarxalloc.a ./
cp $SRCDIR0/libcedarxplayer_intermediates/libcedarxplayer.a ./
cp $SRCDIR0/libcedarv_intermediates/libcedarv.a             ./
cp $SRCDIR0/libjpgenc_intermediates/libjpgenc.a             ./
cp $SRCDIR0/libdemux_cedarm_intermediates/libdemux_cedarm.a             ./
cp $SRCDIR0/libsub_intermediates/libsub.a             ./
cp $SRCDIR0/libsub_inline_intermediates/libsub_inline.a             ./
cp $SRCDIR0/libiconv_intermediates/libiconv.a ./
cp $SRCDIR0/libh264enc_intermediates/libh264enc.a ./
cp $SRCDIR0/libmp4_muxer_intermediates/libmp4_muxer.a ./
cp $SRCDIR0/libm3u_intermediates/libm3u.a ./
cp $SRCDIR0/libcedara_decoder_intermediates/libcedara_decoder.a ./
cp $SRCDIR0/libcedarx_rtsp_intermediates/libcedarx_rtsp.a ./
cp ~/workspace/android4.0.1/out/target/product/crane-evb/system/lib/libstagefright_soft_cedar_h264dec.so ./


arm-eabi-strip -g *.a

cp ~/workspace/android2.3.4/out/target/product/crane-evb/symbols/system/lib/libCedarX.so ./
current_time=`date +%Y%m%d%H%M`
mv libCedarX.so libCedarX-symbols-$current_time.so

