

/*================================================================
 *   
 *   
 *   文件名称：proxy.h
 *   创 建 者：肖飞
 *   创建日期：2021年08月31日 星期二 15时10分27秒
 *   修改日期：2022年01月07日 星期五 14时02分44秒
 *   描    述：
 *
 *================================================================*/
#ifndef _PROXY_H
#define _PROXY_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	PROXY_TYPE_NONE = 0,
	PROXY_TYPE_CHANNEL,
	PROXY_TYPE_MULTI_CHARGER,
} proxy_type_t;

#define PROXY_ADDR_REMOTE 0xff
#define PROXY_FLAG 0x12

typedef struct {
	uint32_t src_id : 8;
	uint32_t dst_id : 8;
	uint32_t type : 8;//proxy_type_t
	uint32_t flag : 5;//PROXY_FLAG
	uint32_t unused : 3;
} com_can_id_t;

typedef union {
	com_can_id_t s;
	uint32_t v;
} u_com_can_id_t;

#endif //_PROXY_H
