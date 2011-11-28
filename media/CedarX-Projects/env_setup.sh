################ CedarX Configuration #######################

export CC=arm-none-linux-gnueabi-gcc
export AR=arm-none-linux-gnueabi-ar

PWD=`pwd`

export CEDARX_CHIP_VERSION=F23
export CEDARX_TOP=${PWD}/CedarX
export CEDARX_BUILD_STATIC_LIB=1
export CEDARX_DEMUXLIB_PATH=${CEDARX_TOP}/../CedarDemuxLib
export CEDARM_TOP=${CEDARX_TOP}/libexternal/CedarM
export CEDARX_ENABLE_MEMWATCH=0


