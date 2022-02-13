

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy.h
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时28分49秒
 *   修改日期：2022年02月13日 星期日 20时30分11秒
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

#define channels_comm_proxy_command_enum(e) CHANNELS_COMM_PROXY_COMMAND_##e

typedef enum {
	channels_comm_proxy_command_enum(CHANNEL_HEARTBEAT) = 0,
	channels_comm_proxy_command_enum(CHANNEL_REQUIRE),
	channels_comm_proxy_command_enum(CHANNEL_START),
	channels_comm_proxy_command_enum(CHANNEL_STOP),
	channels_comm_proxy_command_enum(CHANNEL_OUTPUT),
	channels_comm_proxy_command_enum(DAU_STATUS),
	channels_comm_proxy_command_enum(MODULES_READY),
	channels_comm_proxy_command_enum(MODULES_STATUS),
	channels_comm_proxy_command_enum(INPUT_VOLTAGE),

	CHANNELS_COMM_PROXY_COMMAND_SIZE,
} channels_comm_proxy_command_t;

#define channels_comm_proxy_command_code_enum(e) CHANNELS_COMM_PROXY_COMMAND_CODE_##e

typedef enum {
	channels_comm_proxy_command_code_enum(CHANNEL_HEARTBEAT) = 0,
	channels_comm_proxy_command_code_enum(CHANNEL_REQUIRE),
	channels_comm_proxy_command_code_enum(CHANNEL_START),
	channels_comm_proxy_command_code_enum(CHANNEL_STOP),
	channels_comm_proxy_command_code_enum(CHANNEL_OUTPUT) = 5,
	channels_comm_proxy_command_code_enum(DAU_STATUS) = 101,
	channels_comm_proxy_command_code_enum(MODULES_READY) = 102,
	channels_comm_proxy_command_code_enum(MODULES_STATUS) = 105,
	channels_comm_proxy_command_code_enum(INPUT_VOLTAGE) = 106,
} channels_comm_proxy_command_code_t;

char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd);

#endif //_CHANNELS_COMM_PROXY_H
