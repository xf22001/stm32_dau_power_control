

/*================================================================
 *   
 *   
 *   文件名称：relay_board_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 11时11分49秒
 *   修改日期：2020年10月29日 星期四 08时46分55秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARD_COMMAND_H
#define _RELAY_BOARD_COMMAND_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT = 0,
	RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT,
	RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG,
	RELAY_BOARD_CMD_RELAY_BOARD_STATUS,
	RELAY_BOARD_CMD_TOTAL,
} relay_board_cmd_t;

#pragma pack(push, 1)

typedef struct {
	char buffer[64];
} relay_board_heartbeat_t;

typedef struct {
	//char buffer[64];
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

#endif //_RELAY_BOARD_COMMAND_H
