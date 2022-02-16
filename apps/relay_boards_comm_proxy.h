

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_comm_proxy.h
 *   创 建 者：肖飞
 *   创建日期：2022年02月15日 星期二 09时07分39秒
 *   修改日期：2022年02月16日 星期三 14时51分19秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARDS_COMM_PROXY_H
#define _RELAY_BOARDS_COMM_PROXY_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#define relay_boards_comm_proxy_command_enum(e) RELAY_BOARDS_COMM_PROXY_COMMAND_##e

typedef enum {
	relay_boards_comm_proxy_command_enum(RELAY_BOARD_HEARTBEAT) = 0,
	relay_boards_comm_proxy_command_enum(RELAY_BOARDS_HEARTBEAT),
	relay_boards_comm_proxy_command_enum(RELAY_BOARDS_CONFIG),
	relay_boards_comm_proxy_command_enum(RELAY_BOARD_STATUS),

	RELAY_BOARDS_COMM_PROXY_COMMAND_SIZE,
} relay_board_cmd_t;

#pragma pack(push, 1)

typedef struct {
	char buffer[1];
} relay_board_heartbeat_t;

typedef struct {
	char buffer[1];
} relay_boards_heartbeat_t;

typedef struct {
	uint8_t config;
} relay_board_config_t;

typedef struct {
	uint8_t fault : 1;
	uint8_t over_temperature : 1;
} relay_board_fault_t;

typedef struct {
	uint8_t config;
	int16_t temperature1;
	int16_t temperature2;
	relay_board_fault_t fault;//relay_board_fault_t
} relay_board_status_t;

#pragma pack(pop)

char *get_relay_boards_comm_proxy_command_des(relay_board_cmd_t cmd);

#endif //_RELAY_BOARDS_COMM_PROXY_H
