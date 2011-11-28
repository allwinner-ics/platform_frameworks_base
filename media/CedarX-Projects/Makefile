include Config-Linux.mk

all:
ifeq ($(CEDARX_DEBUG_LEVEL),L0)
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mov
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mkv
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_rmvb
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mpg
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_ts
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_asf
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_avi
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_flv
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_pmp
	make -C ${CEDARX_TOP}/../CedarVideoLib/cedarv_osal
	make -C ${CEDARX_TOP}/../CedarVideoLib/allocator
	make -C ${CEDARX_TOP}/../CedarVideoLib/libcedarv/libve
	make -C ${CEDARX_TOP}/../CedarVideoLib/libcedarv
endif
ifneq ($(CEDARX_DEBUG_LEVEL),L2)
	make -C ${CEDARX_TOP}/components
	make -C ${CEDARX_TOP}/libmuxer/mp4_muxer
	make -C ${CEDARX_TOP}/libcodecs/h264encLib
	make -C ${CEDARX_TOP}/libcodecs/jpgencLib
	make -C ${CEDARX_TOP}/libexternal/CedarM/build-dir
	make -C ${CEDARX_TOP}/libsub/sub_inline
	make -C ${CEDARX_TOP}/libsub/sub_external
	make -C ${CEDARX_TOP}/libutil/playlist/m3u
	make -C ${CEDARX_TOP}/libdemux
	make -C ${CEDARX_TOP}/libstream
	make -C ${CEDARX_TOP}/librender
	make -C ${CEDARX_TOP}/framework
	make -C ${CEDARX_TOP}/capture 
endif	
	make -C ${CEDARX_TOP}/libdemux/thirdpart_demux
	make -C ${CEDARX_TOP}/tests
	make -C ${CEDARX_TOP}/tests/recorder

clean: 
ifeq ($(CEDARX_DEBUG_LEVEL),L0)
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mov clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mkv clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_rmvb clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_mpg clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_asf clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_ts  clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_avi clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_flv clean
	make -C $(CEDARX_DEMUXLIB_PATH)/psr_pmp clean
	make -C ${CEDARX_TOP}/../CedarVideoLib/cedarv_osal clean
	make -C ${CEDARX_TOP}/../CedarVideoLib/allocator clean
	make -C ${CEDARX_TOP}/../CedarVideoLib/libcedarv/libve clean
	make -C ${CEDARX_TOP}/../CedarVideoLib/libcedarv/ clean
endif
ifneq ($(CEDARX_DEBUG_LEVEL),L2)
	make -C ${CEDARX_TOP}/components clean
	make -C ${CEDARX_TOP}/libmuxer/mp4_muxer clean
	make -C ${CEDARX_TOP}/libcodecs/h264encLib clean
	make -C ${CEDARX_TOP}/libcodecs/jpgencLib clean
	make -C ${CEDARX_TOP}/libexternal/CedarM/build-dir clean
	make -C ${CEDARX_TOP}/libsub/sub_inline clean
	make -C ${CEDARX_TOP}/libsub/sub_external clean
	make -C ${CEDARX_TOP}/libutil/playlist/m3u clean
	make -C ${CEDARX_TOP}/libdemux clean
	make -C ${CEDARX_TOP}/libstream clean
	make -C ${CEDARX_TOP}/librender clean
	make -C ${CEDARX_TOP}/framework clean
	make -C ${CEDARX_TOP}/capture clean
endif
	make -C ${CEDARX_TOP}/libdemux/thirdpart_demux clean
	make -C ${CEDARX_TOP}/tests clean
	make -C ${CEDARX_TOP}/tests/recorder clean

install:
	@echo install

