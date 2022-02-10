

/*================================================================
 *
 *
 *   文件名称：display_cache.h
 *   创 建 者：肖飞
 *   创建日期：2021年07月17日 星期六 09时42分47秒
 *   修改日期：2021年07月28日 星期三 10时17分30秒
 *   描    述：
 *
 *================================================================*/
#ifndef _DISPLAY_CACHE_H
#define _DISPLAY_CACHE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

#pragma pack(push, 1)

typedef struct {
	uint8_t sync;
} display_cache_app_t;

typedef struct {
	uint8_t sync;
} display_cache_channels_t;

typedef struct {
	uint8_t sync;
} display_cache_channel_t;

#pragma pack(pop)

#endif //_DISPLAY_CACHE_H
