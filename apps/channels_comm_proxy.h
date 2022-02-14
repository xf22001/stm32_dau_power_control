

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy.h
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时28分49秒
 *   修改日期：2022年02月14日 星期一 13时59分54秒
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

#pragma pack(push, 1)

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
	uint8_t cmd;
	uint8_t fault_stop;
	uint8_t pdu_idle;
} cmd_request_pdu_status_t;

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

#define channels_comm_proxy_command_enum(e) CHANNELS_COMM_PROXY_COMMAND_##e

typedef enum {
	channels_comm_proxy_command_enum(CHANNEL_REQUIRE) = 0,
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
	channels_comm_proxy_command_code_enum(CHANNEL_REQUIRE) = 0,
	channels_comm_proxy_command_code_enum(CHANNEL_START),
	channels_comm_proxy_command_code_enum(CHANNEL_STOP),
	channels_comm_proxy_command_code_enum(CHANNEL_OUTPUT) = 5,
	channels_comm_proxy_command_code_enum(DAU_STATUS) = 101,
	channels_comm_proxy_command_code_enum(MODULES_READY) = 102,
	channels_comm_proxy_command_code_enum(MODULES_STATUS) = 105,
	channels_comm_proxy_command_code_enum(INPUT_VOLTAGE) = 106,
} channels_comm_proxy_command_code_t;

char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd);
char *get_channel_require_work_state_des(channel_require_work_state_t state);

#endif //_CHANNELS_COMM_PROXY_H
