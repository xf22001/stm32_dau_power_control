

/*================================================================
 *
 *
 *   文件名称：relay_boards_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 14时29分27秒
 *   修改日期：2021年10月09日 星期六 11时31分34秒
 *   描    述：
 *
 *================================================================*/
#include "relay_boards_communication.h"

#include <string.h>
#include <stdlib.h>

#include "object_class.h"
#include "channels.h"

#include "relay_board_command.h"
#include "can_data_task.h"

//#define LOG_NONE
#include "log.h"

typedef struct {
	uint32_t src_id : 8;//0xff
	uint32_t dst_id : 8;
	uint32_t unused : 8;
	uint32_t flag : 5;//0x12
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
	uint32_t flag : 5;//0x12
	uint32_t unused1 : 3;
} com_can_rx_id_t;

typedef union {
	com_can_rx_id_t s;
	uint32_t v;
} u_com_can_rx_id_t;


#define RESPONSE_TIMEOUT 3000

static char *get_relay_board_cmd_des(relay_board_cmd_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT);
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG);
			add_des_case(RELAY_BOARD_CMD_RELAY_BOARD_STATUS);

		default: {
		}
		break;
	}

	return des;
}

typedef struct {
	relay_board_heartbeat_t relay_board_heartbeat;
	relay_boards_heartbeat_t relay_boards_heartbeat;
	relay_board_config_t relay_board_config;
	relay_board_status_t relay_board_status;
} data_ctx_t;

static object_class_t *relay_boards_com_class = NULL;

typedef int (*request_callback_t)(relay_boards_com_info_t *relay_boards_com_info);
typedef int (*response_callback_t)(relay_boards_com_info_t *relay_boards_com_info);

typedef struct {
	uint8_t cmd;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
} command_item_t;

static uint8_t tx_get_id(relay_boards_com_info_t *relay_boards_com_info)
{
	u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&relay_boards_com_info->can_tx_msg.ExtId;
	return u_com_can_tx_id->s.dst_id;
}

static uint8_t rx_get_id(relay_boards_com_info_t *relay_boards_com_info)
{
	u_com_can_rx_id_t *u_com_can_rx_id = (u_com_can_rx_id_t *)&relay_boards_com_info->can_rx_msg->ExtId;
	return u_com_can_rx_id->s.dst_id;
}

static uint32_t cmd_ctx_offset(uint8_t relay_board_id, uint8_t cmd)
{
	return RELAY_BOARD_CMD_TOTAL * relay_board_id + cmd;
}

//准备请求数据 发
static int prepare_tx_request(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;

	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_prepare_tx_request(cmd_ctx, can_com_cmd_common, data, data_size);

	return ret;
}

//请求数据后,处理响应响应 收
static int process_rx_response(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	uint8_t relay_board_id = rx_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_rx_msg->Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_process_rx_response(cmd_ctx, can_com_cmd_response, data_size);

	return ret;
}

//处理请求后，准备响应数据 发
static int prepare_tx_response(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;

	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_prepare_tx_response(cmd_ctx, can_com_cmd_response, data_size);

	return ret;
}

//处理请求数据 收
static int process_rx_request(relay_boards_com_info_t *relay_boards_com_info, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	uint8_t relay_board_id = rx_get_id(relay_boards_com_info);
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, cmd);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_rx_msg->Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = can_com_process_rx_request(cmd_ctx, can_com_cmd_common, data, data_size);

	return ret;
}

static int request_relay_board_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;

	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	//debug("relay_board_id:%d", relay_board_id);

	ret = prepare_tx_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT, sizeof(relay_board_heartbeat_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		int i;

		for(i = 0; i < sizeof(relay_board_heartbeat_t); i++) {
			if(data_ctx->relay_board_heartbeat.buffer[i] != i) {
				debug("relay_board %d relay_board_heartbeat data[%d] == %d",
				      relay_board_id,
				      i,
				      data_ctx->relay_board_heartbeat.buffer[i]);
				break;
			}
		}

		//_hexdump("relay_board_heartbeat",
		//         (const char *)&data_ctx->relay_board_heartbeat,
		//         sizeof(relay_board_heartbeat_t));
	}

	return ret;
}

static int response_relay_board_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	uint8_t relay_board_id = rx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;

	if(data_ctx == NULL) {
		return ret;
	}

	ret = process_rx_request(relay_boards_com_info,
	                         RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT,
	                         (uint8_t *)&data_ctx->relay_board_heartbeat,
	                         sizeof(relay_board_heartbeat_t));

	return ret;
}

static command_item_t command_item_relay_board_heartbeat = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT,
	.request_period = 0,
	.request_callback = request_relay_board_heartbeat,
	.response_callback = response_relay_board_heartbeat,
};

//static void update_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
//{
//	int i;
//
//	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
//	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
//
//	for(i = 0; i < sizeof(relay_boards_heartbeat_t); i++) {
//		data_ctx->relay_boards_heartbeat.buffer[i] = i;
//	}
//}

static int request_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	if(cmd_ctx->index == 0) {
		//update_relay_boards_heartbeat(relay_boards_com_info);
	}

	ret = prepare_tx_request(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, (uint8_t *)&data_ctx->relay_boards_heartbeat, sizeof(relay_boards_heartbeat_t));

	return ret;
}

static int response_relay_boards_heartbeat(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	//uint8_t relay_board_id = rx_get_id(relay_boards_com_info);
	//uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	//uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT);
	//command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	//command_status_t *cmd_ctx_next;
	//uint8_t next_relay_board_id = relay_board_id;

	//if(relay_board_id >= relay_board_number) {
	//	debug("");
	//	return ret;
	//}

	//if(next_relay_board_id + 1 ==  relay_board_number) {
	//	next_relay_board_id = 0;
	//} else {
	//	next_relay_board_id += 1;
	//}
	//
	//cmd_ctx_next = relay_boards_com_info->cmd_ctx + next_relay_board_id;

	ret = process_rx_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, sizeof(relay_boards_heartbeat_t));


	//if(cmd_ctx->state == COMMAND_STATE_IDLE) {
	//	cmd_ctx->available = 0;
	//	cmd_ctx_next->available = 1;

	//	debug("cmd %d(%s), disable relay_board %d, enable relay_board %d", RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT, get_relay_board_cmd_des(RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT), relay_board_id, next_relay_board_id);
	//}

	return ret;
}

static command_item_t command_item_relay_boards_heartbeat = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT,
	.request_period = 500,
	.request_callback = request_relay_boards_heartbeat,
	.response_callback = response_relay_boards_heartbeat,
};

int relay_boards_com_update_config(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id, uint8_t config)
{
	int ret = 0;
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
	uint8_t cmd_ctx_index = cmd_ctx_offset(relay_board_id, RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG);
	command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	relay_board_config_t *relay_board_config = &data_ctx->relay_board_config;
	channels_info_t *channels_info = (channels_info_t *)relay_boards_com_info->channels_info;
	relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + relay_board_id;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	debug("relay_board_id:%d, config:%02x", relay_board_id, config);

	relay_board_item_info->remote_config = ~config;

	relay_board_config->config = config;

	cmd_ctx->state = COMMAND_STATE_REQUEST;
	return ret;
}

static int request_relay_boards_config(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = prepare_tx_request(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG, (uint8_t *)&data_ctx->relay_board_config, sizeof(relay_board_config_t));

	return ret;
}

static int response_relay_boards_config(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;

	ret = process_rx_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG, sizeof(relay_board_config_t));

	return ret;
}

static command_item_t command_item_relay_boards_config = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG,
	.request_period = 0,
	.request_callback = request_relay_boards_config,
	.response_callback = response_relay_boards_config,
};

static int request_relay_board_status(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;

	uint8_t relay_board_id = tx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_com_info->can_tx_msg.Data;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	ret = prepare_tx_response(relay_boards_com_info, RELAY_BOARD_CMD_RELAY_BOARD_STATUS, sizeof(relay_board_status_t));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		channels_info_t *channels_info = (channels_info_t *)relay_boards_com_info->channels_info;
		relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + relay_board_id;
		relay_board_status_t *relay_board_status = &data_ctx->relay_board_status;

		//debug("relay_board_id %d status: config %02x, temperature1:%d, temperature2:%d, fault:%d, over_temperature:%d",
		//      relay_board_id,
		//      relay_board_status->config,
		//      relay_board_status->temperature1,
		//      relay_board_status->temperature2,
		//      relay_board_status->fault.fault,
		//      relay_board_status->fault.over_temperature);

		relay_board_item_info->temperature1 = relay_board_status->temperature1;
		relay_board_item_info->temperature2 = relay_board_status->temperature2;

		if(relay_board_item_info->remote_config != relay_board_status->config) {
			relay_board_item_info->remote_config = relay_board_status->config;
			debug("relay_board_id %d update remote config %02x",
			      relay_board_id,
			      relay_board_item_info->remote_config);

			if(relay_board_item_info->config != relay_board_item_info->remote_config) {
				debug("relay_board_id %d config %02x != remote config %02x",
				      relay_board_id,
				      relay_board_item_info->config,
				      relay_board_item_info->remote_config);
			}
		}

		if(get_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_FAULT) != relay_board_status->fault.fault) {
			channel_info_t *channel_info = channels_info->channel_info + relay_board_item_info->channel_id;
			pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;

			set_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_FAULT, relay_board_status->fault.fault);
			set_fault(pdu_group_info->faults, PDU_GROUP_FAULT_RELAY_BOARD, relay_board_status->fault.fault);

			if(relay_board_status->fault.fault != 0) {
				int i;
				debug("relay_board_id %d fault:%d",
				      relay_board_id,
				      relay_board_status->fault.fault);

				for(i = 0; i < channels_info->channel_number; i++) {
					channel_info_t *channel_info_item = channels_info->channel_info + i;

					if(pdu_group_info->pdu_group_id == channel_info_item->pdu_group_info->pdu_group_id) {
						start_stop_channel(channels_info, channel_info_item->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
					}
				}
			}
		}

		if(get_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_OVER_TEMPERATURE) != relay_board_status->fault.over_temperature) {
			channel_info_t *channel_info = channels_info->channel_info + relay_board_item_info->channel_id;

			set_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_OVER_TEMPERATURE, relay_board_status->fault.over_temperature);
			set_fault(channel_info->faults, CHANNEL_FAULT_RELAY_BOARD_OVER_TEMPERATURE, relay_board_status->fault.over_temperature);

			if(relay_board_status->fault.over_temperature != 0) {
				debug("relay_board_id %d over_temperature:%d",
				      relay_board_id,
				      relay_board_status->fault.over_temperature);
				start_stop_channel(channels_info, channel_info->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
			}
		}

	}

	return ret;
}

static int response_relay_board_status(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = -1;
	uint8_t relay_board_id = rx_get_id(relay_boards_com_info);
	data_ctx_t *data_ctx = (data_ctx_t *)relay_boards_com_info->relay_boards_com_data_ctx + relay_board_id;

	if(data_ctx == NULL) {
		return ret;
	}

	ret = process_rx_request(relay_boards_com_info,
	                         RELAY_BOARD_CMD_RELAY_BOARD_STATUS,
	                         (uint8_t *)&data_ctx->relay_board_status,
	                         sizeof(relay_board_status_t));

	return ret;
}

static command_item_t command_item_relay_board_status = {
	.cmd = RELAY_BOARD_CMD_RELAY_BOARD_STATUS,
	.request_period = 0,
	.request_callback = request_relay_board_status,
	.response_callback = response_relay_board_status,
};

static command_item_t *relay_boards_com_command_table[] = {
	&command_item_relay_board_heartbeat,
	&command_item_relay_boards_heartbeat,
	&command_item_relay_boards_config,
	&command_item_relay_board_status,
};

static void relay_boards_com_set_connect_state(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id, uint8_t state)
{
	connect_state_t *connect_state = relay_boards_com_info->connect_state + relay_board_id;

	update_connect_state(connect_state, state);
}

uint8_t relay_boards_com_get_connect_state(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id)
{
	connect_state_t *connect_state = relay_boards_com_info->connect_state + relay_board_id;

	return get_connect_state(connect_state);
}

static uint32_t relay_boards_com_get_connect_stamp(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id)
{
	connect_state_t *connect_state = relay_boards_com_info->connect_state + relay_board_id;

	return get_connect_stamp(connect_state);
}

static void relay_boards_com_request_periodic(relay_boards_com_info_t *relay_boards_com_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, relay_boards_com_info->periodic_stamp) < 50) {
		return;
	}

	relay_boards_com_info->periodic_stamp = ticks;

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

		for(j = 0; j < relay_board_number; j++) {
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
			channels_info_t *channels_info = (channels_info_t *)relay_boards_com_info->channels_info;
			relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + j;
			uint8_t connect_timeout = 0;

			if(ticks_duration(ticks, relay_boards_com_get_connect_stamp(relay_boards_com_info, j)) >= (10 * 1000)) {
				connect_timeout = 1;
			} else {
				connect_timeout = 0;
			}

			if(get_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
				int i;
				channel_info_t *channel_info = channels_info->channel_info + relay_board_item_info->channel_id;
				pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;

				set_fault(relay_board_item_info->faults, RELAY_BOARD_ITEM_FAULT_CONNECT_TIMEOUT, connect_timeout);
				set_fault(pdu_group_info->faults, PDU_GROUP_FAULT_RELAY_BOARD, connect_timeout);

				if(connect_timeout != 0) {
					for(i = 0; i < channels_info->channel_number; i++) {
						channel_info_t *channel_info_item = channels_info->channel_info + i;

						if(pdu_group_info->pdu_group_id == channel_info_item->pdu_group_info->pdu_group_id) {
							start_stop_channel(channels_info, channel_info_item->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
						}
					}
				}
			}

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					relay_boards_com_set_connect_state(relay_boards_com_info, j, 0);
					debug("cmd %d(%s), index %d, relay_board %d timeout, connect state:%d",
					      item->cmd,
					      get_relay_board_cmd_des(item->cmd),
					      cmd_ctx->index,
					      j,
					      relay_boards_com_get_connect_state(relay_boards_com_info, j));

					cmd_ctx->state = COMMAND_STATE_REQUEST;
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

				cmd_ctx->index = 0;
				cmd_ctx->state = COMMAND_STATE_REQUEST;
			}
		}

	}
}

static void relay_boards_com_request(relay_boards_com_info_t *relay_boards_com_info)
{
	int ret = 0;
	int i;
	int j;

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

		for(j = 0; j < relay_board_number; j++) {
			uint32_t ticks = osKernelSysTick();
			uint8_t cmd_ctx_index = cmd_ctx_offset(j, item->cmd);
			command_status_t *cmd_ctx = relay_boards_com_info->cmd_ctx + cmd_ctx_index;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_tx_msg.Data;
			u_com_can_tx_id_t *u_com_can_tx_id = (u_com_can_tx_id_t *)&relay_boards_com_info->can_tx_msg.ExtId;

			relay_boards_com_request_periodic(relay_boards_com_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_com_can_tx_id->v = 0;
			u_com_can_tx_id->s.flag = 0x12;
			u_com_can_tx_id->s.src_id = 0xff;
			u_com_can_tx_id->s.dst_id = j;

			relay_boards_com_info->can_tx_msg.IDE = CAN_ID_EXT;
			relay_boards_com_info->can_tx_msg.RTR = CAN_RTR_DATA;
			relay_boards_com_info->can_tx_msg.DLC = 8;

			//debug("request cmd %d(%s), relay_board:%d, index:%d", item->cmd, get_relay_board_cmd_des(item->cmd), j, can_com_cmd_common->index);

			memset(relay_boards_com_info->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(relay_boards_com_info);

			if(ret != 0) {
				debug("process request cmd %d(%s), index %d, relay_board %d error!", item->cmd, get_relay_board_cmd_des(item->cmd), cmd_ctx->index, j);
				continue;
			}

			cmd_ctx->send_stamp = ticks;
			ret = can_tx_data(relay_boards_com_info->can_info, &relay_boards_com_info->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				cmd_ctx->state = COMMAND_STATE_REQUEST;

				debug("send request cmd %d(%s), index %d, relay_board %d error!", item->cmd, get_relay_board_cmd_des(item->cmd), cmd_ctx->index, j);
				relay_boards_com_set_connect_state(relay_boards_com_info, j, 0);
			}

			osDelay(5);
		}
	}
}

static int relay_boards_com_response(relay_boards_com_info_t *relay_boards_com_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;

	u_com_can_rx_id_t *u_com_can_rx_id;
	uint8_t relay_board_id;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	relay_boards_com_info->can_rx_msg = can_rx_msg;

	u_com_can_rx_id = (u_com_can_rx_id_t *)&relay_boards_com_info->can_rx_msg->ExtId;

	if(u_com_can_rx_id->s.flag != 0x12) {
		//debug("response flag:%02x!", u_com_can_rx_id->s.flag);
		return ret;
	}

	if(u_com_can_rx_id->s.src_id != 0xff) {
		debug("response relay_boards id:%02x!", u_com_can_rx_id->s.src_id);
		debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		      relay_boards_com_info->can_rx_msg->ExtId,
		      relay_boards_com_info->can_rx_msg->Data[0],
		      relay_boards_com_info->can_rx_msg->Data[1],
		      relay_boards_com_info->can_rx_msg->Data[2],
		      relay_boards_com_info->can_rx_msg->Data[3],
		      relay_boards_com_info->can_rx_msg->Data[4],
		      relay_boards_com_info->can_rx_msg->Data[5],
		      relay_boards_com_info->can_rx_msg->Data[6],
		      relay_boards_com_info->can_rx_msg->Data[7]);
		return ret;
	}

	relay_board_id = u_com_can_rx_id->s.dst_id;

	if(relay_board_id >= relay_board_number) {
		debug("relay_board_id:%d!", relay_board_id);
		debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		      relay_boards_com_info->can_rx_msg->ExtId,
		      relay_boards_com_info->can_rx_msg->Data[0],
		      relay_boards_com_info->can_rx_msg->Data[1],
		      relay_boards_com_info->can_rx_msg->Data[2],
		      relay_boards_com_info->can_rx_msg->Data[3],
		      relay_boards_com_info->can_rx_msg->Data[4],
		      relay_boards_com_info->can_rx_msg->Data[5],
		      relay_boards_com_info->can_rx_msg->Data[6],
		      relay_boards_com_info->can_rx_msg->Data[7]);
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(relay_boards_com_command_table); i++) {
		command_item_t *item = relay_boards_com_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_com_info->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			//debug("response cmd %d(%s), relay_board:%d, index:%d", item->cmd, get_relay_board_cmd_des(item->cmd), relay_board_id, can_com_cmd_common->index);

			relay_boards_com_set_connect_state(relay_boards_com_info, relay_board_id, 1);

			ret = item->response_callback(relay_boards_com_info);

			if(ret != 0) {//收到响应
				debug("process response cmd %d(%s), relay_board %d error!", item->cmd, get_relay_board_cmd_des(item->cmd), relay_board_id);
				debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
				      relay_boards_com_info->can_rx_msg->ExtId,
				      relay_boards_com_info->can_rx_msg->Data[0],
				      relay_boards_com_info->can_rx_msg->Data[1],
				      relay_boards_com_info->can_rx_msg->Data[2],
				      relay_boards_com_info->can_rx_msg->Data[3],
				      relay_boards_com_info->can_rx_msg->Data[4],
				      relay_boards_com_info->can_rx_msg->Data[5],
				      relay_boards_com_info->can_rx_msg->Data[6],
				      relay_boards_com_info->can_rx_msg->Data[7]);
			}

			ret = 0;
			break;
		}

	}

	return ret;
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	relay_boards_com_request(relay_boards_com_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)fn_ctx;
	can_rx_msg_t *can_rx_msg = can_get_msg(relay_boards_com_info->can_info);

	if(fn_ctx == NULL) {
		return;
	}

	relay_boards_com_response(relay_boards_com_info, can_rx_msg);
}

static void free_relay_boards_com_info(relay_boards_com_info_t *relay_boards_com_info)
{
	if(relay_boards_com_info == NULL) {
		return;
	}

	if(relay_boards_com_info->cmd_ctx != NULL) {
		os_free(relay_boards_com_info->cmd_ctx);
	}

	if(relay_boards_com_info->relay_boards_com_data_ctx != NULL) {
		os_free(relay_boards_com_info->relay_boards_com_data_ctx);
	}

	if(relay_boards_com_info->connect_state != NULL) {
		os_free(relay_boards_com_info->connect_state);
	}

	os_free(relay_boards_com_info);
}

static int relay_boards_com_info_set_channels_info(relay_boards_com_info_t *relay_boards_com_info, channels_info_t *channels_info)
{
	int ret = -1;
	can_info_t *can_info;
	command_status_t *can_com_cmd_ctx;
	data_ctx_t *relay_boards_com_data_ctx;
	connect_state_t *connect_state;
	uint8_t relay_board_number;
	int i;
	can_data_task_info_t *can_data_task_info;

	relay_boards_com_info->channels_info = channels_info;

	relay_board_number = channels_info->channels_config->relay_board_number;

	relay_boards_com_info->relay_board_number = relay_board_number;

	if(relay_board_number == 0) {
		debug("");
		return ret;
	}

	debug("relay_board_number:%d", relay_board_number);

	can_com_cmd_ctx = (command_status_t *)os_alloc(sizeof(command_status_t) * RELAY_BOARD_CMD_TOTAL * relay_board_number);

	if(can_com_cmd_ctx == NULL) {
		return ret;
	}

	memset(can_com_cmd_ctx, 0, sizeof(command_status_t) * RELAY_BOARD_CMD_TOTAL * relay_board_number);

	relay_boards_com_info->cmd_ctx = can_com_cmd_ctx;

	for(i = 0; i < relay_board_number; i++) {
		relay_boards_com_info->cmd_ctx[cmd_ctx_offset(i, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT)].available = 1;
	}

	for(i = 0; i < relay_board_number; i++) {
		relay_boards_com_info->cmd_ctx[cmd_ctx_offset(i, RELAY_BOARD_CMD_RELAY_BOARDS_CONFIG)].state = COMMAND_STATE_REQUEST;
	}

	relay_boards_com_data_ctx = (data_ctx_t *)os_alloc(sizeof(data_ctx_t) * relay_board_number);

	if(relay_boards_com_data_ctx == NULL) {
		return ret;
	}

	memset(relay_boards_com_data_ctx, 0, sizeof(data_ctx_t) * relay_board_number);

	relay_boards_com_info->relay_boards_com_data_ctx = relay_boards_com_data_ctx;

	connect_state = (connect_state_t *)os_alloc(sizeof(connect_state_t) * relay_board_number);

	if(connect_state == NULL) {
		return ret;
	}

	memset(connect_state, 0, sizeof(connect_state_t) * relay_board_number);

	relay_boards_com_info->connect_state = connect_state;

	can_info = get_or_alloc_can_info(channels_info->channels_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	relay_boards_com_info->can_info = can_info;

	can_data_task_info = get_or_alloc_can_data_task_info(relay_boards_com_info->can_info->hcan);

	if(can_data_task_info == NULL) {
		app_panic();
	}

	relay_boards_com_info->can_data_request_cb.fn = can_data_request;
	relay_boards_com_info->can_data_request_cb.fn_ctx = relay_boards_com_info;
	add_can_data_task_info_request_cb(can_data_task_info, &relay_boards_com_info->can_data_request_cb);

	relay_boards_com_info->can_data_response_cb.fn = can_data_response;
	relay_boards_com_info->can_data_response_cb.fn_ctx = relay_boards_com_info;
	add_can_data_task_info_response_cb(can_data_task_info, &relay_boards_com_info->can_data_response_cb);

	ret = 0;
	return ret;
}

static relay_boards_com_info_t *alloc_relay_boards_com_info(channels_info_t *channels_info)
{
	relay_boards_com_info_t *relay_boards_com_info = NULL;

	relay_boards_com_info = (relay_boards_com_info_t *)os_alloc(sizeof(relay_boards_com_info_t));

	if(relay_boards_com_info == NULL) {
		return relay_boards_com_info;
	}

	memset(relay_boards_com_info, 0, sizeof(relay_boards_com_info_t));

	if(relay_boards_com_info_set_channels_info(relay_boards_com_info, channels_info) != 0) {
		goto failed;
	}

	return relay_boards_com_info;
failed:

	free_relay_boards_com_info(relay_boards_com_info);

	relay_boards_com_info = NULL;

	return relay_boards_com_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)o;
	channels_info_t *channels_info = (channels_info_t *)ctx;

	if(relay_boards_com_info->channels_info == channels_info) {
		ret = 0;
	}

	return ret;
}

relay_boards_com_info_t *get_or_alloc_relay_boards_com_info(void *ctx)
{
	relay_boards_com_info_t *relay_boards_com_info = NULL;

	os_enter_critical();

	if(relay_boards_com_class == NULL) {
		relay_boards_com_class = object_class_alloc();
	}

	os_leave_critical();

	relay_boards_com_info = (relay_boards_com_info_t *)object_class_get_or_alloc_object(relay_boards_com_class, object_filter, ctx, (object_alloc_t)alloc_relay_boards_com_info, (object_free_t)free_relay_boards_com_info);

	return relay_boards_com_info;
}

