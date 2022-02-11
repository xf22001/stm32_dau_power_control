

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy.h
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时28分49秒
 *   修改日期：2022年01月07日 星期五 15时49分19秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_COMM_PROXY_H
#define _CHANNELS_COMM_PROXY_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channels.h"

#pragma pack(push, 1)

typedef struct {
	uint8_t data[128 * 6];
} proxy_remote_heartbeat_t;

typedef struct {
	uint8_t data[128 * 6];
} proxy_local_heartbeat_t;

typedef struct {
	uint8_t data[128 * 6];
} proxy_remote_stateless_t;

typedef struct {
	uint8_t data[128 * 6];
} proxy_local_stateless_t;

typedef struct {
	uint8_t unused;
} proxy_local_channel_login_t;

#pragma pack(pop)

typedef enum {
	CHANNELS_COMM_PROXY_COMMAND_PROXY_REMOTE_HEARTBEAT = 0,//远端代理心跳
	CHANNELS_COMM_PROXY_COMMAND_PROXY_LOCAL_HEARTBEAT,//本地代理心跳
	CHANNELS_COMM_PROXY_COMMAND_PROXY_REMOTE_STATELESS,//远端代理广播
	CHANNELS_COMM_PROXY_COMMAND_PROXY_LOCAL_STATELESS,//本地代理广播

	CHANNELS_COMM_PROXY_COMMAND_PROXY_LOCAL_CHANNEL_LOGIN,//本地上线

	CHANNELS_COMM_PROXY_COMMAND_SIZE,
} channels_comm_proxy_command_t;

char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd);

#endif //_CHANNELS_COMM_PROXY_H
