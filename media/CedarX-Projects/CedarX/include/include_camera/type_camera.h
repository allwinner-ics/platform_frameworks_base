
#ifndef __LIB__CAMERA__TYPE__H__
#define __LIB__CAMERA__TYPE__H__

typedef struct V4L2BUF_t
{
	unsigned int	addrPhyY;		// physical Y address of this frame
	int 			index;			// DQUE id number
	long long		timeStamp;		// time stamp of this frame
}V4L2BUF_t;

typedef struct PREVIEWINFO_t
{
	int left;
	int top;
	int height;			// preview height
	int width;			// preview width
}PREVIEWINFO_t;

typedef struct VIDEOINFO_t
{
	int src_height;
	int src_width;
	int height;			// camcorder video frame height
	int width;			// camcorder video frame width
	int frameRate;		// camcorder video frame rate
	int bitRate;		// camcorder video bitrate
	short profileIdc;
	short levelIdc;
}VIDEOINFO_t;

typedef struct AUDIOINFO_t
{
	int sampleRate;
	int channels;
	int bitRate;
	int bitsPerSample;
}AUDIOINFO_t;

typedef struct ENC_BUFFER_t
{
    int addrY;
	int addrCb;
	int addrCr;
	int width;
	int height;
}ENC_BUFFER_t;

typedef enum JPEG_COLOR_FORMAT
{
    JPEG_COLOR_YUV444,
    JPEG_COLOR_YUV422,
    JPEG_COLOR_YUV420,
    JPEG_COLOR_YUV411,
}JPEG_COLOR_FORMAT;

typedef struct JPEG_ENC_t
{
	int src_w;
	int src_h;
	int pic_w;
	int pic_h;
	int addrY;
	int addrC;
	int colorFormat;
	int quality;
	int rotate;
}JPEG_ENC_t;

#endif // __LIB__CAMERA__TYPE__H__

