

/*================================================================
 *   
 *   
 *   文件名称：proxy.h
 *   创 建 者：肖飞
 *   创建日期：2021年08月31日 星期二 15时10分27秒
 *   修改日期：2022年02月16日 星期三 11时17分37秒
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
	PROXY_ADDR_REMOTE_CHANNEL = 0xff,
	PROXY_ADDR_REMOTE_RELAY_BOARD = 0xff,
} proxy_addr_remote_t;

typedef enum {
	PROXY_TYPE_CHANNEL = 0x00,
	PROXY_TYPE_RELAY_BOARD = 0x00,
} proxy_type_t;

typedef enum {
	PROXY_FLAG_CHANNEL = 0x15,
	PROXY_FLAG_RELAY_BOARD = 0x12,
} proxy_flag_t;

typedef struct {
	uint32_t src_id : 8;
	uint32_t dst_id : 8;
	uint32_t type : 8;//proxy_type_t
	uint32_t flag : 5;//proxy_flag_t
	uint32_t unused : 3;
} com_can_id_t;

typedef union {
	com_can_id_t s;
	uint32_t v;
} u_com_can_id_t;

#endif //_PROXY_H
