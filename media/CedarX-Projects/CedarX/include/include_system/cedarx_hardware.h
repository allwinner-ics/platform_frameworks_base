#ifndef CEDARX_HARDWARE_H
#define CEDARX_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum CEDARX_HARDWARE_MODE
{
	CEDARX_HARDWARE_MODE_UNKOWN = 0,
	CEDARX_HARDWARE_MODE_MUSIC,
	//add other's here and don't touch below define
	CEDARX_HARDWARE_MODE_VIDEO,
	CEDARX_HARDWARE_MODE_VIDEO_THUMB,

}CEDARX_HARDWARE_MODE;

extern int cedarx_hardware_init(int mode);
extern int cedarx_hardware_exit(int mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
