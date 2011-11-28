LIBPATH=${CEDARX_TOP}/output
BUILDPATH=${CEDARX_TOP}

include ${CEDARX_TOP}/../Config-Linux.mk

CFLAGS=-Wall 
CFLAGS+=-I${CEDARX_TOP}/include/include_platform/CHIP_${CEDARX_CHIP_VERSION}
CFLAGS+=-I${CEDARX_TOP}/include/include_platform/CHIP_${CEDARX_CHIP_VERSION}/disp  
CFLAGS+=-I${CEDARX_TOP}/include/include_platform/CHIP_${CEDARX_CHIP_VERSION}/sys

CFLAGS+=-I${CEDARX_TOP}/include -I${CEDARX_TOP}/framework -I${CEDARX_TOP}/libdemux/epdk_demux -I${CEDARX_TOP}/libdemux/ -I${CEDARX_TOP}/libdemux/cedar_demux -I${CEDARX_TOP}/libutil -I${CEDARX_TOP}/libstream
CFLAGS+=-I${CEDARX_TOP}/libcodecs/include -I${CEDARX_TOP}/libcodecs/libvdcedar -I${CEDARX_TOP}/librender -I${CEDARX_TOP}/osdep
CFLAGS+=-I${CEDARX_TOP}/include/include_cedarv/
CFLAGS+=-I${CEDARX_TOP}/libsub/include/

CFLAGS+=-O2 -D__OS_LINUX -D__CHIP_VERSION_$(CEDARX_CHIP_VERSION)

CFLAGS+=$(CEDARX_EXT_CFLAGS)

#CFLAGS+=-D__CDX_AC3DTS_HW_CODEC


LIBSRC = \
	libdemux/cedarx_demux.c \
	libdemux/epdk_demux/epdk_demux.c \
	libdemux/music_demux.c

ifeq ($(CEDARX_BUILD_STATIC_LIB),1)
LIB=$(LIBPATH)/libcedarxdemuxers.a
else
LIB=$(LIBPATH)/libcedarxdemuxers.so
CFLAGS+=-fPIC
endif


LIBOBJ=$(addprefix $(BUILDPATH)/, $(addsuffix .o, $(basename $(LIBSRC))))

all: $(LIB)

lib:$(LIB)

$(BUILDPATH)/%.o:%.c
	$(CC) $(CFLAGS) $(EXT_CFLAGS) -o $@ -c $<
	
ifeq ($(CEDARX_BUILD_STATIC_LIB),1)
$(LIB):$(LIBOBJ)
	$(AR) -rcs $@ $^
else
$(LIB):$(LIBOBJ)
	$(CC) -shared -o $@ $^
endif

clean:
	rm -f $(LIB) $(LIBOBJ)
