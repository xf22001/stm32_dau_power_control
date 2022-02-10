

/*================================================================
 *
 *
 *   文件名称：channel_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月26日 星期二 08时50分38秒
 *   修改日期：2021年09月09日 星期四 11时16分06秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMMAND_H
#define _CHANNEL_COMMAND_H
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
	CHANNEL_CMD_CHANNEL_HEARTBEAT = 0,
	CHANNEL_CMD_CHANNEL_REQUIRE,
	CHANNEL_CMD_CHANNEL_START,
	CHANNEL_CMD_CHANNEL_STOP,
	CHANNEL_CMD_CHANNEL_OUTPUT,
	CHANNEL_CMD_PDU_STATUS,
	CHANNEL_CMD_MODULES_READY,
	CHANNEL_CMD_MODULES_STATUS,
	CHANNEL_CMD_INPUT_VOLTAGE,
	CHANNEL_CMD_TOTAL,
} channel_cmd_t;

typedef enum {
	CHANNEL_CMD_CODE_CHANNEL_HEARTBEAT = 0,
	CHANNEL_CMD_CODE_CHANNEL_REQUIRE,
	CHANNEL_CMD_CODE_CHANNEL_START,
	CHANNEL_CMD_CODE_CHANNEL_STOP,
	CHANNEL_CMD_CODE_CHANNEL_OUTPUT = 5,
	CHANNEL_CMD_CODE_PDU_STATUS = 101,
	CHANNEL_CMD_CODE_MODULES_READY = 102,
	CHANNEL_CMD_CODE_MODULES_STATUS = 105,
	CHANNEL_CMD_CODE_INPUT_VOLTAGE = 106,
	CHANNEL_CMD_CODE_TOTAL,
} channel_cmd_code_t;

#pragma pack(push, 1)

typedef struct {
	char buffer[64];
} channel_heartbeat_t;

typedef enum {
	MODULE_ASSIGN_INFO_READY = 0,
	MODULE_ASSIGN_INFO_RETRY,
} module_assign_info_t;

typedef enum {
	CHANNEL_WORK_STATE_IDLE = 0,
	CHANNEL_WORK_STATE_PREPARE_START,//准备启动
	CHANNEL_WORK_STATE_START,//启动中
	CHANNEL_WORK_STATE_CHARGE,//充电
	CHANNEL_WORK_STATE_STOP,//结束
} channel_require_work_state_t;

typedef struct {
	uint8_t cmd;
	uint8_t info;//module_assign_info_t
} channel_module_assign_info_t;

typedef struct {
	uint8_t cmd;
	uint8_t require_voltage_l;//0.1v
	uint8_t require_voltage_h;//0.1v
	uint8_t require_current_l;//0.1a
	uint8_t require_current_h;//0.1a
	uint8_t require_state;//channel_require_work_state_t
	uint8_t fault_stop;
} channel_require_t;

typedef struct {
	uint8_t cmd;
	uint8_t relay_fb_state;//0断开，1闭合
} channel_require_code_t;

typedef struct {
	uint8_t cmd;
	uint8_t output_voltage_l;//0.1v
	uint8_t output_voltage_h;//0.1v
	uint8_t output_current_l;//0.1a
	uint8_t output_current_h;//0.1a
} channel_output_t;

typedef struct {
	uint8_t setting_voltage_l;//0.1v
	uint8_t setting_voltage_h;//0.1v
	uint8_t setting_current_l;//0.1a
	uint8_t setting_current_h;//0.1a
	uint8_t output_voltage_l;//0.1v
	uint8_t output_voltage_h;//0.1v
	uint8_t output_current_l;//0.1a
	uint8_t output_current_h;//0.1a
	uint8_t module_state_l;
	uint8_t module_state_h;
	uint8_t connect_state_l;
	uint8_t connect_state_h;
} module_status_t;

typedef struct {
	uint8_t cmd;
	uint8_t v_a_l;
	uint8_t v_a_h;
	uint8_t v_b_l;
	uint8_t v_b_h;
	uint8_t v_c_l;
	uint8_t v_c_h;
} input_voltage_info_t;

#pragma pack(pop)

#endif //_CHANNEL_COMMAND_H
