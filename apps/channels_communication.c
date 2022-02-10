

/*================================================================
 *
 *
 *   文件名称：channels_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分07秒
 :   修改日期：2021年12月10日 星期五 11时18分56秒
 *   描    述：
 *
 *================================================================*/
#include "channels_communication.h"
#include <string.h>
#include <stdlib.h>

#include "object_class.h"
#include "channels.h"

#include "channel_command.h"
#include "can_data_task.h"
#include "power_modules.h"

#define LOG_DISABLE
#include "log.h"

typedef struct {
	uint32_t src_id : 8;//0xff
	uint32_t dst_id : 8;
	uint32_t unused : 8;
	uint32_t flag : 5;//0x15
	uint32_t unused1 : 3;
} com_can_tx_id_t;

typedef union {
	com_can_tx_id_t s;
	uint32_t v;
} u_com_can_tx_id_t;

typedef struct {
	uint32_t dst_id : 8;
	uint32_t src_id : 8;//0xff
	uint32_t unused : 8;
	uint32_t flag : 5;//0x15
	uint32_t unused1 : 3;
} com_can_rx_id_t;

typedef union {
	com_can_rx_id_t s;
	uint32_t v;
} u_com_can_rx_id_t;

typedef struct {
	uint8_t cmd;
} cmd_request_t;

typedef struct {
	uint8_t cmd;
} cmd_response_t;

#define RESPONSE_TIMEOUT 200

static char *get_channel_cmd_des(channel_cmd_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(CHANNEL_CMD_CHANNEL_HEARTBEAT);
			add_des_case(CHANNEL_CMD_CHANNEL_REQUIRE);
			add_des_case(CHANNEL_CMD_CHANNEL_START);
			add_des_case(CHANNEL_CMD_CHANNEL_STOP);
			add_des_case(CHANNEL_CMD_CHANNEL_OUTPUT);
			add_des_case(CHANNEL_CMD_PDU_STATUS);
			add_des_case(CHANNEL_CMD_MODULES_READY);
			add_des_case(CHANNEL_CMD_MODULES_STATUS);
			add_des_case(CHANNEL_CMD_INPUT_VOLTAGE);

		default: {
		}
		break;
	}

	return des;
}

static char *get_module_assign_info_des(module_assign_info_t module_assign_info)
{
	char *des = "unknow";

	switch(module_assign_info) {
			add_des_case(MODULE_ASSIGN_INFO_READY);
			add_des_case(MODULE_ASSIGN_INFO_RETRY);

		default: {
		}
		break;
	}

	return des;
}


typedef struct {
	channel_heartbeat_t channel_heartbeat;
	module_assign_info_t module_assign_info;
} data_ctx_t;

static object_class_t *channels_com_class = NULL;

typedef int (*request_callback_t)(channels_com_info_t *channels_com_info);
typedef int (*response_callback_t)(channels_com_info_t *channels_com_info);

typedef struct {
	uint8_t cmd;
	uint8_t cmd_code;
	uint8_t broadcast;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
} command_item_t;

static uint8_t tx_get_id(channels_com_info_t *channels_com_info)
{
	u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;
	return u_com_can_tx_id->s.dst_id - 1;
}

static uint8_t rx_get_id(channels_com_info_t *channels_com_info)
{
	u_com_can_rx_id_t *u_com_can_rx_id = (u_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;
	return u_com_can_rx_id->s.dst_id - 1;
}

static uint32_t cmd_ctx_offset(uint8_t channel_id, uint8_t cmd)
{
	return CHANNEL_CMD_TOTAL * channel_id + cmd;
}

//准备请求数据 发
static int prepare_tx_request(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;

	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, cmd);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	ret = can_com_prepare_tx_request(cmd_ctx, can_com_cmd_common, data, data_size);

	return ret;
}

//请求数据后,处理响应响应 收
static int process_rx_response(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, cmd);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channels_com_info->can_rx_msg->Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	ret = can_com_process_rx_response(cmd_ctx, can_com_cmd_response, data_size);

	return ret;
}

//处理请求后，准备响应数据 发
static int prepare_tx_response(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;

	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, cmd);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	ret = can_com_prepare_tx_response(cmd_ctx, can_com_cmd_response, data_size);

	return ret;
}

//处理请求数据 收
static int process_rx_request(channels_com_info_t *channels_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, cmd);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_com_info->can_rx_msg->Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	ret = can_com_process_rx_request(cmd_ctx, can_com_cmd_common, data, data_size);

	return ret;
}

static int request_channel_heartbeat(channels_com_info_t *channels_com_info)
{
	int ret = -1;

	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t channel_number = channels_com_info->channel_number;
	data_ctx_t *data_ctx = (data_ctx_t *)channels_com_info->channels_com_data_ctx + channel_id;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	//debug("channel_id:%d", channel_id);

	ret = prepare_tx_response(channels_com_info, CHANNEL_CMD_CHANNEL_HEARTBEAT, sizeof(channel_heartbeat_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		int i;

		for(i = 0; i < sizeof(channel_heartbeat_t); i++) {
			if(data_ctx->channel_heartbeat.buffer[i] != i) {
				debug("channel %d channel_heartbeat data[%d] == %d",
				      channel_id,
				      i,
				      data_ctx->channel_heartbeat.buffer[i]);
				break;
			}
		}

		//_hexdump("channel_heartbeat",
		//         (const char *)&data_ctx->channel_heartbeat,
		//         sizeof(channel_heartbeat_t));
	}

	return ret;
}

static int response_channel_heartbeat(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)channels_com_info->channels_com_data_ctx + channel_id;

	if(data_ctx == NULL) {
		return ret;
	}

	ret = process_rx_request(channels_com_info,
	                         CHANNEL_CMD_CHANNEL_HEARTBEAT,
	                         (uint8_t *)&data_ctx->channel_heartbeat,
	                         sizeof(channel_heartbeat_t));

	return ret;
}

static command_item_t command_item_channel_heartbeat = {
	.cmd = CHANNEL_CMD_CHANNEL_HEARTBEAT,
	.cmd_code = CHANNEL_CMD_CODE_CHANNEL_HEARTBEAT,
	.broadcast = 0,
	.request_period = 0,
	.request_callback = request_channel_heartbeat,
	.response_callback = response_channel_heartbeat,
};

static int request_channel_require(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_REQUIRE);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_response->cmd = CHANNEL_CMD_CODE_CHANNEL_REQUIRE;

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static const char *get_channel_require_work_state_des(channel_require_work_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_WORK_STATE_IDLE);
			add_des_case(CHANNEL_WORK_STATE_PREPARE_START);
			add_des_case(CHANNEL_WORK_STATE_START);
			add_des_case(CHANNEL_WORK_STATE_CHARGE);
			add_des_case(CHANNEL_WORK_STATE_STOP);

		default: {
		}
		break;
	}

	return des;
}
static int response_channel_require(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_REQUIRE);
	uint8_t channel_number = channels_com_info->channel_number;
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	channel_require_t *channel_require = (channel_require_t *)channels_com_info->can_rx_msg->Data;
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	uint32_t ticks = osKernelSysTick();

	if(channel_id >= channel_number) {
		return ret;
	}

	channel_info->status.require_output_stamp = ticks;

	channel_info->status.require_output_voltage = get_u16_from_u8_lh(
	            channel_require->require_voltage_l,
	            channel_require->require_voltage_h);
	channel_info->status.require_output_current = get_u16_from_u8_lh(
	            channel_require->require_current_l,
	            channel_require->require_current_h);
	//debug("channel_id:%d require_output_voltage:%d, require_output_current:%d",
	//      channel_id,
	//      channel_info->status.require_output_voltage,
	//      channel_info->status.require_output_current);

	if(channel_info->status.require_work_state != channel_require->require_state) {
		debug("channel_id:%d require_state %s -> %s",
		      channel_id,
		      get_channel_require_work_state_des(channel_info->status.require_work_state),
		      get_channel_require_work_state_des(channel_require->require_state));

		channel_info->status.require_work_state = channel_require->require_state;

		switch(channel_require->require_state) {
			//case CHANNEL_WORK_STATE_IDLE:
			case CHANNEL_WORK_STATE_STOP: {//停机
				channels_com_info->cmd_ctx[cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_READY)].state = COMMAND_STATE_IDLE;
				start_stop_channel(channels_info, channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
			}
			break;

			default: {
			}
			break;
		}
	}

	if(get_fault(channel_info->faults, CHANNEL_FAULT_FAULT) != channel_require->fault_stop) {
		set_fault(channel_info->faults, CHANNEL_FAULT_FAULT, channel_require->fault_stop);

		debug("channel:%d update fault:%d", channel_info->channel_id, channel_require->fault_stop);

		if(channel_require->fault_stop != 0) {
			start_stop_channel(channels_info, channel_info->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
		}
	}

	cmd_ctx->state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static command_item_t command_item_channel_require = {
	.cmd = CHANNEL_CMD_CHANNEL_REQUIRE,
	.cmd_code = CHANNEL_CMD_CODE_CHANNEL_REQUIRE,
	.broadcast = 0,
	.request_period = 0,
	.request_callback = request_channel_require,
	.response_callback = response_channel_require,
};

static int request_channel_start(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_START);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_response->cmd = CHANNEL_CMD_CODE_CHANNEL_START;

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_channel_start(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_START);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	channels_info_t *channels_info = channels_com_info->channels_info;

	start_stop_channel(channels_info, channel_id, CHANNEL_EVENT_TYPE_START_CHANNEL);

	cmd_ctx->state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static command_item_t command_item_channel_start = {
	.cmd = CHANNEL_CMD_CHANNEL_START,
	.cmd_code = CHANNEL_CMD_CODE_CHANNEL_START,
	.broadcast = 0,
	.request_period = 0,
	.request_callback = request_channel_start,
	.response_callback = response_channel_start,
};

static int request_channel_output(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_OUTPUT);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_response->cmd = CHANNEL_CMD_CODE_CHANNEL_OUTPUT;

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_channel_output(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_CHANNEL_OUTPUT);
	uint8_t channel_number = channels_com_info->channel_number;
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	channel_output_t *channel_output = (channel_output_t *)channels_com_info->can_rx_msg->Data;
	channels_info_t *channels_info = channels_com_info->channels_info;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	uint32_t ticks = osKernelSysTick();

	if(channel_id >= channel_number) {
		return ret;
	}

	channel_info->status.charge_output_stamp = ticks;
	channel_info->status.charge_output_voltage = get_u16_from_u8_lh(
	            channel_output->output_voltage_l,
	            channel_output->output_voltage_h);
	channel_info->status.charge_output_current = get_u16_from_u8_lh(
	            channel_output->output_current_l,
	            channel_output->output_current_h);

	//debug("channel_id %d charge_output_voltage:%d, charge_output_current:%d",
	//      channel_id,
	//      channel_info->status.charge_output_voltage,
	//      channel_info->status.charge_output_current);

	cmd_ctx->state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static command_item_t command_item_channel_output = {
	.cmd = CHANNEL_CMD_CHANNEL_OUTPUT,
	.cmd_code = CHANNEL_CMD_CODE_CHANNEL_OUTPUT,
	.broadcast = 0,
	.request_period = 0,
	.request_callback = request_channel_output,
	.response_callback = response_channel_output,
};

typedef struct {
	uint8_t cmd;
	uint8_t fault_stop;
	uint8_t pdu_idle;
} cmd_request_pdu_status_t;

static uint8_t pdu_channels_idle(channels_info_t *channels_info)
{
	int i;
	uint8_t ret = 1;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(channel_info->status.state != CHANNEL_STATE_IDLE) {
			ret = 0;
			break;
		}
	}

	return ret;
}

static uint8_t pdu_fault(channel_info_t *channel_info)
{
	uint8_t fault = 0;
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	if(get_first_fault(channels_info->faults) != -1) {
		debug("");
		fault = 1;
	}

	if(get_first_fault(pdu_group_info->faults) != -1) {
		debug("");
		fault = 1;
	}

	return fault;
}

static int request_pdu_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_PDU_STATUS);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	cmd_request_pdu_status_t *cmd_request = (cmd_request_pdu_status_t *)channels_com_info->can_tx_msg.Data;
	channels_info_t *channels_info = channels_com_info->channels_info;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_request->cmd = CHANNEL_CMD_CODE_PDU_STATUS;
	cmd_request->fault_stop = pdu_fault(channel_info);
	cmd_request->pdu_idle = pdu_channels_idle(channels_info);

	cmd_ctx->state = COMMAND_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_pdu_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_PDU_STATUS);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static command_item_t command_item_pdu_status = {
	.cmd = CHANNEL_CMD_PDU_STATUS,
	.cmd_code = CHANNEL_CMD_CODE_PDU_STATUS,
	.broadcast = 1,
	.request_period = 50,
	.request_callback = request_pdu_status,
	.response_callback = response_pdu_status,
};

int channels_com_channel_module_assign_info(channels_com_info_t *channels_com_info, uint8_t channel_id, module_assign_info_t module_assign_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_READY);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	data_ctx_t *data_ctx = (data_ctx_t *)channels_com_info->channels_com_data_ctx + channel_id;

	if(channel_id >= channel_number) {
		return ret;
	}

	data_ctx->module_assign_info = module_assign_info;

	debug("channel_id:%d set module_assign_info %s",
	      channel_id,
	      get_module_assign_info_des(module_assign_info));
	cmd_ctx->state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_modules_ready(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_READY);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	data_ctx_t *data_ctx = (data_ctx_t *)channels_com_info->channels_com_data_ctx + channel_id;
	channel_module_assign_info_t *channel_module_assign_info = (channel_module_assign_info_t *)channels_com_info->can_tx_msg.Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	channel_module_assign_info->cmd = CHANNEL_CMD_CODE_MODULES_READY;
	channel_module_assign_info->info = data_ctx->module_assign_info;

	cmd_ctx->state = COMMAND_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_modules_ready(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_READY);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	//cmd_response_t *cmd_response = (cmd_response_t *)channels_com_info->can_rx_msg->Data;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static command_item_t command_item_modules_ready = {
	.cmd = CHANNEL_CMD_MODULES_READY,
	.cmd_code = CHANNEL_CMD_CODE_MODULES_READY,
	.broadcast = 0,
	.request_period = 0,
	.request_callback = request_modules_ready,
	.response_callback = response_modules_ready,
};

typedef struct {
	uint8_t cmd;
	uint8_t offset;
	uint8_t data[6];
} cmd_module_status_request_t;

static int request_modules_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_STATUS);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	cmd_module_status_request_t *cmd_request = (cmd_module_status_request_t *)channels_com_info->can_tx_msg.Data;
	uint8_t max_index = (sizeof(module_status_t) * channels_com_info->power_module_number + sizeof(cmd_request->data) - 1) / sizeof(cmd_request->data);
	int data_size;

	cmd_request->cmd = CHANNEL_CMD_CODE_MODULES_STATUS;

	if(channel_id >= channel_number) {
		return ret;
	}

	if(cmd_ctx->index >= max_index) {
		cmd_ctx->index = 0;
	}

	data_size = sizeof(module_status_t) * channels_com_info->power_module_number - cmd_ctx->index * sizeof(cmd_request->data);

	if(data_size > sizeof(cmd_request->data)) {
		data_size = sizeof(cmd_request->data);
	}

	cmd_request->offset = cmd_ctx->index;

	memcpy(cmd_request->data, (uint8_t *)channels_com_info->module_status + cmd_ctx->index * sizeof(cmd_request->data), data_size);

	cmd_ctx->index++;

	if(cmd_ctx->index >= max_index) {
		cmd_ctx->state = COMMAND_STATE_IDLE;
	}

	ret = 0;

	return ret;
}

static int response_modules_status(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_STATUS);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static command_item_t command_item_modules_status = {
	.cmd = CHANNEL_CMD_MODULES_STATUS,
	.cmd_code = CHANNEL_CMD_CODE_MODULES_STATUS,
	.broadcast = 1,
	.request_period = 2 * 1000,
	.request_callback = request_modules_status,
	.response_callback = response_modules_status,
};

static int request_input_voltage(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = tx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_INPUT_VOLTAGE);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
	input_voltage_info_t *input_voltage_info = (input_voltage_info_t *)channels_com_info->can_tx_msg.Data;
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	power_modules_info_t *power_modules_info = (power_modules_info_t *)channels_info->power_module_item_info[0].power_modules_info;
	power_module_info_t *power_module_info = &power_modules_info->power_module_info[0];

	if(channel_id >= channel_number) {
		return ret;
	}

	input_voltage_info->v_a_l = get_u8_l_from_u16(power_module_info->input_aline_voltage);
	input_voltage_info->v_a_h = get_u8_h_from_u16(power_module_info->input_aline_voltage);
	input_voltage_info->v_b_l = get_u8_l_from_u16(power_module_info->input_bline_voltage);
	input_voltage_info->v_b_h = get_u8_h_from_u16(power_module_info->input_bline_voltage);
	input_voltage_info->v_c_l = get_u8_l_from_u16(power_module_info->input_cline_voltage);
	input_voltage_info->v_c_h = get_u8_h_from_u16(power_module_info->input_cline_voltage);

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_input_voltage(channels_com_info_t *channels_com_info)
{
	int ret = -1;
	uint8_t channel_number = channels_com_info->channel_number;
	uint8_t channel_id = rx_get_id(channels_com_info);
	uint8_t cmd_ctx_index = cmd_ctx_offset(channel_id, CHANNEL_CMD_MODULES_STATUS);
	command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;

	if(channel_id >= channel_number) {
		return ret;
	}

	cmd_ctx->state = COMMAND_STATE_IDLE;

	ret = 0;

	return ret;
}

static command_item_t command_item_input_voltage = {
	.cmd = CHANNEL_CMD_INPUT_VOLTAGE,
	.cmd_code = CHANNEL_CMD_CODE_INPUT_VOLTAGE,
	.broadcast = 1,
	.request_period = 2 * 1000,
	.request_callback = request_input_voltage,
	.response_callback = response_input_voltage,
};

static command_item_t *channels_com_command_table[] = {
	&command_item_channel_heartbeat,
	&command_item_channel_require,
	&command_item_channel_start,
	&command_item_channel_output,
	&command_item_pdu_status,
	&command_item_modules_ready,
	&command_item_modules_status,
	&command_item_input_voltage,
};

static void channels_com_set_connect_state(channels_com_info_t *channels_com_info, uint8_t channel_id, uint8_t state)
{
	connect_state_t *connect_state = channels_com_info->connect_state + channel_id;

	update_connect_state(connect_state, state);
}

uint8_t channels_com_get_connect_state(channels_com_info_t *channels_com_info, uint8_t channel_id)
{
	connect_state_t *connect_state = channels_com_info->connect_state + channel_id;

	return get_connect_state(connect_state);
}

static uint32_t channels_com_get_connect_stamp(channels_com_info_t *channels_com_info, uint8_t channel_id)
{
	connect_state_t *connect_state = channels_com_info->connect_state + channel_id;

	return get_connect_stamp(connect_state);
}

static void channels_com_request_periodic(channels_com_info_t *channels_com_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channels_com_info->periodic_stamp) < 50) {
		return;
	}

	channels_com_info->periodic_stamp = ticks;

	for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
		command_item_t *item = channels_com_command_table[i];
		uint8_t channel_number = channels_com_info->channel_number;

		for(j = 0; j < channel_number; j++) {
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
			channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
			channel_info_t *channel_info = channels_info->channel_info + j;
			uint8_t connect_timeout;

			if(ticks_duration(ticks, channels_com_get_connect_stamp(channels_com_info, j)) >= (10 * 1000)) {
				connect_timeout = 1;
			} else {
				connect_timeout = 0;
			}

			if(get_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
				set_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT, connect_timeout);

				if(connect_timeout != 0) {
					debug("cmd %d(%s), index %d, channel %d timeout, ticks:%d, update stamp:%d",
					      item->cmd,
					      get_channel_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      ticks,
					      channels_com_get_connect_stamp(channels_com_info, j));
					start_stop_channel(channels_info, channel_info->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
				}
			}

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					channels_com_set_connect_state(channels_com_info, j, 0);
					debug("cmd %d(%s), index %d, channel %d timeout, connect state:%d",
					      item->cmd,
					      get_channel_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      channels_com_get_connect_state(channels_com_info, j));
					cmd_ctx->state = COMMAND_STATE_REQUEST;
				}
			}

			if(item->cmd == CHANNEL_CMD_CHANNEL_REQUIRE || item->cmd == CHANNEL_CMD_CHANNEL_OUTPUT) {
				if(ticks_duration(ticks, cmd_ctx->recv_stamp) >= 1000) {
					channels_com_set_connect_state(channels_com_info, j, 0);
					debug("cmd %d(%s), index %d, channel %d timeout, ticks:%d, recv_ticks:%d, connect state:%d",
					      item->cmd,
					      get_channel_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      ticks,
					      cmd_ctx->recv_stamp,
					      channels_com_get_connect_state(channels_com_info, j));
				}
			}

			if(item->request_period == 0) {
				continue;
			}

			if(cmd_ctx->available == 0) {
				continue;
			}

			if(cmd_ctx->state != COMMAND_STATE_IDLE) {
				continue;
			}

			if(ticks_duration(ticks, cmd_ctx->stamp) >= item->request_period) {
				cmd_ctx->stamp = ticks;

				//cmd_ctx->index = 0;
				cmd_ctx->state = COMMAND_STATE_REQUEST;
			}
		}

	}
}

static void channels_com_request(channels_com_info_t *channels_com_info)
{
	int ret = 0;
	int i;
	int j;

	for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
		command_item_t *item = channels_com_command_table[i];
		uint8_t channel_number = channels_com_info->channel_number;

		for(j = 0; j < channel_number; j++) {
			uint32_t ticks = osKernelSysTick();
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = channels_com_info->cmd_ctx + cmd_ctx_index;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_com_info->can_tx_msg.Data;
			u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&channels_com_info->can_tx_msg.ExtId;

			channels_com_request_periodic(channels_com_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			osDelay(5);

			u_com_can_tx_id->v = 0;
			u_com_can_tx_id->s.flag = 0x15;
			u_com_can_tx_id->s.src_id = 0xff;

			u_com_can_tx_id->s.dst_id = j + 1;

			channels_com_info->can_tx_msg.IDE = CAN_ID_EXT;
			channels_com_info->can_tx_msg.RTR = CAN_RTR_DATA;
			channels_com_info->can_tx_msg.DLC = 8;

			//debug("request cmd %d(%s), channel:%d, index:%d", item->cmd, get_channel_cmd_des(item->cmd), j, can_com_cmd_common->index);

			memset(channels_com_info->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd_code;

			ret = item->request_callback(channels_com_info);

			if(ret != 0) {
				debug("process request cmd %d(%s), index %d, channel %d error!", item->cmd, get_channel_cmd_des(item->cmd), cmd_ctx->index, j);
				continue;
			}

			cmd_ctx->send_stamp = ticks;

			if(item->broadcast != 0) {
				u_com_can_tx_id->s.dst_id = 0x00;
			}

			ret = can_tx_data(channels_com_info->can_info, &channels_com_info->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				cmd_ctx->state = COMMAND_STATE_REQUEST;

				debug("send request cmd %d(%s), channel %d error", item->cmd, get_channel_cmd_des(item->cmd), j);
				channels_com_set_connect_state(channels_com_info, j, 0);
			}
		}
	}
}

static int channels_com_response(channels_com_info_t *channels_com_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;

	u_com_can_rx_id_t *u_com_can_rx_id;
	uint8_t channel_id;
	uint8_t channel_number = channels_com_info->channel_number;
	uint32_t ticks = osKernelSysTick();

	channels_com_info->can_rx_msg = can_rx_msg;

	u_com_can_rx_id = (u_com_can_rx_id_t *)&channels_com_info->can_rx_msg->ExtId;

	if(u_com_can_rx_id->s.flag != 0x15) {
		//debug("response flag:%02x!", u_com_can_rx_id->s.flag);
		return ret;
	}

	if(u_com_can_rx_id->s.src_id != 0xff) {
		debug("response channels id:%02x!", u_com_can_rx_id->s.src_id);
		return ret;
	}

	channel_id = u_com_can_rx_id->s.dst_id - 1;

	if(channel_id >= channel_number) {
		debug("channel_id:%d!", channel_id);
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(channels_com_command_table); i++) {
		command_item_t *item = channels_com_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_com_info->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd_code) {
			//debug("response cmd %d(%s), channel:%d, index:%d", item->cmd, get_channel_cmd_des(item->cmd), channel_id, can_com_cmd_common->index);
			channels_com_info->cmd_ctx[cmd_ctx_offset(channel_id, item->cmd)].recv_stamp = ticks;

			channels_com_set_connect_state(channels_com_info, channel_id, 1);

			ret = item->response_callback(channels_com_info);

			if(ret != 0) {//收到响应
				debug("process response cmd %d(%s), channel %d error!", item->cmd, get_channel_cmd_des(item->cmd), channel_id);
			}

			ret = 0;
			break;
		}

	}

	return ret;
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	channels_com_info_t *channels_com_info = (channels_com_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	channels_com_request(channels_com_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	channels_com_info_t *channels_com_info = (channels_com_info_t *)fn_ctx;
	can_rx_msg_t *can_rx_msg = can_get_msg(channels_com_info->can_info);

	if(fn_ctx == NULL) {
		return;
	}

	channels_com_response(channels_com_info, can_rx_msg);
}

static void free_channels_com_info(channels_com_info_t *channels_com_info)
{
	if(channels_com_info == NULL) {
		return;
	}

	if(channels_com_info->cmd_ctx != NULL) {
		os_free(channels_com_info->cmd_ctx);
	}

	if(channels_com_info->channels_com_data_ctx != NULL) {
		os_free(channels_com_info->channels_com_data_ctx);
	}

	if(channels_com_info->connect_state != NULL) {
		os_free(channels_com_info->connect_state);
	}

	os_free(channels_com_info);
}

static int channels_com_info_set_channels_info(channels_com_info_t *channels_com_info, channels_info_t *channels_info)
{
	int ret = -1;
	can_info_t *can_info;
	command_status_t *can_com_cmd_ctx;
	data_ctx_t *channels_com_data_ctx;
	module_status_t *module_status;
	connect_state_t *connect_state;
	uint8_t channel_number;
	uint8_t power_module_number;
	int i;
	can_data_task_info_t *can_data_task_info;

	channels_com_info->channels_info = channels_info;

	channel_number = channels_info->channels_config->channel_number;

	channels_com_info->channel_number = channel_number;

	if(channel_number == 0) {
		debug("");
		return ret;
	}

	debug("channel_number:%d", channel_number);

	power_module_number = channels_info->channels_config->power_module_config.power_module_number;

	channels_com_info->power_module_number = power_module_number;

	if(power_module_number == 0) {
		debug("");
		return ret;
	}

	debug("power_module_number:%d", power_module_number);

	can_com_cmd_ctx = (command_status_t *)os_alloc(sizeof(command_status_t) * CHANNEL_CMD_TOTAL * channel_number);

	if(can_com_cmd_ctx == NULL) {
		return ret;
	}

	memset(can_com_cmd_ctx, 0, sizeof(command_status_t) * CHANNEL_CMD_TOTAL * channel_number);

	channels_com_info->cmd_ctx = can_com_cmd_ctx;

	for(i = 0; i < channel_number; i++) {
		if(i == 0) {
			channels_com_info->cmd_ctx[cmd_ctx_offset(i, CHANNEL_CMD_PDU_STATUS)].available = 1;
			channels_com_info->cmd_ctx[cmd_ctx_offset(i, CHANNEL_CMD_MODULES_STATUS)].available = 1;
			channels_com_info->cmd_ctx[cmd_ctx_offset(i, CHANNEL_CMD_INPUT_VOLTAGE)].available = 1;
		}
	}

	channels_com_data_ctx = (data_ctx_t *)os_alloc(sizeof(data_ctx_t) * channel_number);

	if(channels_com_data_ctx == NULL) {
		return ret;
	}

	memset(channels_com_data_ctx, 0, sizeof(data_ctx_t) * channel_number);

	channels_com_info->channels_com_data_ctx = channels_com_data_ctx;

	connect_state = (connect_state_t *)os_alloc(sizeof(connect_state_t) * channel_number);

	if(connect_state == NULL) {
		return ret;
	}

	memset(connect_state, 0, sizeof(connect_state_t) * channel_number);

	channels_com_info->connect_state = connect_state;


	module_status = (module_status_t *)os_alloc(sizeof(module_status_t) * power_module_number);

	if(module_status == NULL) {
		return ret;
	}

	memset(module_status, 0, sizeof(module_status_t) * power_module_number);

	channels_com_info->module_status = module_status;

	can_info = get_or_alloc_can_info(channels_info->channels_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	channels_com_info->can_info = can_info;

	can_data_task_info = get_or_alloc_can_data_task_info(channels_com_info->can_info->hcan);

	if(can_data_task_info == NULL) {
		app_panic();
	}

	channels_com_info->can_data_request_cb.fn = can_data_request;
	channels_com_info->can_data_request_cb.fn_ctx = channels_com_info;
	add_can_data_task_info_request_cb(can_data_task_info, &channels_com_info->can_data_request_cb);

	channels_com_info->can_data_response_cb.fn = can_data_response;
	channels_com_info->can_data_response_cb.fn_ctx = channels_com_info;
	add_can_data_task_info_response_cb(can_data_task_info, &channels_com_info->can_data_response_cb);

	ret = 0;
	return ret;
}

static channels_com_info_t *alloc_channels_com_info(channels_info_t *channels_info)
{
	channels_com_info_t *channels_com_info = NULL;

	channels_com_info = (channels_com_info_t *)os_alloc(sizeof(channels_com_info_t));

	if(channels_com_info == NULL) {
		return channels_com_info;
	}

	memset(channels_com_info, 0, sizeof(channels_com_info_t));

	if(channels_com_info_set_channels_info(channels_com_info, channels_info) != 0) {
		goto failed;
	}

	return channels_com_info;
failed:

	free_channels_com_info(channels_com_info);

	channels_com_info = NULL;

	return channels_com_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channels_com_info_t *channels_com_info = (channels_com_info_t *)o;
	channels_info_t *channels_info = (channels_info_t *)ctx;

	if(channels_com_info->channels_info == channels_info) {
		ret = 0;
	}

	return ret;
}

channels_com_info_t *get_or_alloc_channels_com_info(void *ctx)
{
	channels_com_info_t *channels_com_info = NULL;

	os_enter_critical();

	if(channels_com_class == NULL) {
		channels_com_class = object_class_alloc();
	}

	os_leave_critical();

	channels_com_info = (channels_com_info_t *)object_class_get_or_alloc_object(channels_com_class, object_filter, ctx, (object_alloc_t)alloc_channels_com_info, (object_free_t)free_channels_com_info);

	return channels_com_info;
}

