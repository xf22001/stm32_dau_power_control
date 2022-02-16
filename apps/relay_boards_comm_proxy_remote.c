

/*================================================================
 *
 *
 *   文件名称：relay_boards_comm_proxy_remote.c
 *   创 建 者：肖飞
 *   创建日期：2022年02月15日 星期二 08时45分38秒
 *   修改日期：2022年02月16日 星期三 14时49分29秒
 *   描    述：
 *
 *================================================================*/
#include "relay_boards_comm_proxy_remote.h"

#include "proxy.h"
#include "relay_boards_comm_proxy.h"
#include "connect_state.h"
#include "command_status.h"
#include "can_command.h"
#include "can_data_task.h"
#include "can_data_task.h"
#include "power_manager.h"

#define LOG_DISABLE
#include "log.h"

typedef struct {
	relay_board_heartbeat_t relay_board_heartbeat;
	relay_boards_heartbeat_t relay_boards_heartbeat;
	relay_board_config_t relay_board_config;
	relay_board_status_t relay_board_status;
} relay_board_data_ctx_t;

typedef struct {
	connect_state_t connect_state;
	command_status_t *cmd_ctx;
	relay_board_data_ctx_t data_ctx;
} relay_boards_comm_proxy_relay_board_ctx_t;

typedef struct {
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;
	uint32_t periodic_stamp;
	can_data_task_info_t *can_data_task_info;
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;

	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx;
} relay_boards_comm_proxy_ctx_t;

typedef int (*request_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id);
typedef int (*response_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id);
typedef int (*timeout_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id);

typedef struct {
	uint8_t cmd;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
	timeout_callback_t timeout_callback;
} command_item_t;

static int request_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	return 0;
}

static int response_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	return 0;
}

static int timeout_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	return 0;
}

static int request_relay_board_heartbeat(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;

	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_comm_proxy_ctx->can_tx_msg.Data;

	//debug("relay_board_id:%d", relay_board_id);

	ret = can_com_prepare_tx_response(cmd_ctx,
	                                  can_com_cmd_response,
	                                  sizeof(data_ctx->relay_board_heartbeat));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		int i;

		for(i = 0; i < sizeof(data_ctx->relay_board_heartbeat); i++) {
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
		//         sizeof(data_ctx->relay_board_heartbeat));
	}

	return ret;
}

static int response_relay_board_heartbeat(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_rx_msg->Data;

	ret = can_com_process_rx_request(cmd_ctx,
	                                 can_com_cmd_common,
	                                 (uint8_t *)data_ctx->relay_board_heartbeat.buffer,
	                                 sizeof(data_ctx->relay_board_heartbeat));

	return ret;
}

static command_item_t command_item_relay_board_heartbeat = {
	.cmd = relay_boards_comm_proxy_command_enum(RELAY_BOARD_HEARTBEAT),
	.request_period = 0,
	.request_callback = request_relay_board_heartbeat,
	.response_callback = response_relay_board_heartbeat,
	.timeout_callback = timeout_callback_proxy_null,
};

static int request_relay_boards_heartbeat(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_tx_msg.Data;

	if(cmd_ctx->index == 0) {
	}

	ret = can_com_prepare_tx_request(cmd_ctx,
	                                 can_com_cmd_common,
	                                 (uint8_t *)&data_ctx->relay_boards_heartbeat,
	                                 sizeof(data_ctx->relay_boards_heartbeat));

	return ret;
}

static int response_relay_boards_heartbeat(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_comm_proxy_ctx->can_rx_msg->Data;

	ret = can_com_process_rx_response(cmd_ctx,
	                                  can_com_cmd_response,
	                                  sizeof(data_ctx->relay_boards_heartbeat));

	if(cmd_ctx->state == COMMAND_STATE_IDLE) {
	}

	return ret;
}

static command_item_t command_item_relay_boards_heartbeat = {
	.cmd = relay_boards_comm_proxy_command_enum(RELAY_BOARDS_HEARTBEAT),
	.request_period = 500,
	.request_callback = request_relay_boards_heartbeat,
	.response_callback = response_relay_boards_heartbeat,
	.timeout_callback = timeout_callback_proxy_null,
};

int relay_boards_comm_update_config(channels_info_t *channels_info, uint8_t relay_board_id, uint8_t config)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + relay_boards_comm_proxy_command_enum(RELAY_BOARDS_CONFIG);
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	power_manager_info_t *power_manager_info = channels_info->power_manager_info;
	power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + relay_board_id;

	debug("relay_board_id:%d, config:%02x", relay_board_id, config);

	if(relay_board_id >= channels_info->relay_board_number) {
		return ret;
	}

	data_ctx->relay_board_config.config = config;
	power_manager_relay_board_info->remote_config = ~config;
	cmd_ctx->state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static int request_relay_boards_config(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_tx_msg.Data;

	ret = can_com_prepare_tx_request(cmd_ctx,
	                                 can_com_cmd_common,
	                                 (uint8_t *)&data_ctx->relay_board_config,
	                                 sizeof(data_ctx->relay_board_config));

	return ret;
}

static int response_relay_boards_config(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_comm_proxy_ctx->can_rx_msg->Data;

	ret = can_com_process_rx_response(cmd_ctx,
	                                  can_com_cmd_response,
	                                  sizeof(data_ctx->relay_board_config));

	if(cmd_ctx->state == COMMAND_STATE_IDLE) {
	}

	return ret;
}

static command_item_t command_item_relay_boards_config = {
	.cmd = relay_boards_comm_proxy_command_enum(RELAY_BOARDS_CONFIG),
	.request_period = 0,
	.request_callback = request_relay_boards_config,
	.response_callback = response_relay_boards_config,
};

static int request_relay_board_status(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_response_t *can_com_cmd_response = (can_com_cmd_response_t *)relay_boards_comm_proxy_ctx->can_tx_msg.Data;

	ret = can_com_prepare_tx_response(cmd_ctx,
	                                  can_com_cmd_response,
	                                  sizeof(data_ctx->relay_board_status));

	if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_DONE) {
		power_manager_info_t *power_manager_info = channels_info->power_manager_info;
		power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + relay_board_id;
		relay_board_status_t *relay_board_status = &data_ctx->relay_board_status;

		//debug("relay_board_id %d status: config %02x, temperature1:%d, temperature2:%d, fault:%d, over_temperature:%d",
		//      relay_board_id,
		//      relay_board_status->config,
		//      relay_board_status->temperature1,
		//      relay_board_status->temperature2,
		//      relay_board_status->fault.fault,
		//      relay_board_status->fault.over_temperature);

		power_manager_relay_board_info->temperature1 = relay_board_status->temperature1;
		power_manager_relay_board_info->temperature2 = relay_board_status->temperature2;

		if(power_manager_relay_board_info->remote_config != relay_board_status->config) {
			power_manager_relay_board_info->remote_config = relay_board_status->config;
			debug("relay_board_id %d update remote config %02x",
			      relay_board_id,
			      power_manager_relay_board_info->remote_config);

			if(power_manager_relay_board_info->config != power_manager_relay_board_info->remote_config) {
				debug("relay_board_id %d config %02x != remote config %02x",
				      relay_board_id,
				      power_manager_relay_board_info->config,
				      power_manager_relay_board_info->remote_config);
			}
		}

		if(get_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_FAULT) != relay_board_status->fault.fault) {
			set_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_FAULT, relay_board_status->fault.fault);

			if(relay_board_status->fault.fault != 0) {
				debug("relay_board_id %d fault:%d",
				      relay_board_id,
				      relay_board_status->fault.fault);
			}
		}

		if(get_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_OVER_TEMPERATURE) != relay_board_status->fault.over_temperature) {
			set_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_OVER_TEMPERATURE, relay_board_status->fault.over_temperature);

			if(relay_board_status->fault.over_temperature != 0) {
				debug("relay_board_id %d over_temperature:%d",
				      relay_board_id,
				      relay_board_status->fault.over_temperature);
			}
		}

	}

	return ret;
}

static int response_relay_board_status(channels_info_t *channels_info, void *_command_item, uint8_t relay_board_id)
{
	int ret = -1;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	command_item_t *item = (command_item_t *)_command_item;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;
	command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
	relay_board_data_ctx_t *data_ctx = &relay_boards_comm_proxy_relay_board_ctx->data_ctx;
	can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_rx_msg->Data;

	ret = can_com_process_rx_request(cmd_ctx,
	                                 can_com_cmd_common,
	                                 (uint8_t *)&data_ctx->relay_board_status,
	                                 sizeof(data_ctx->relay_board_status));

	return ret;
}

static command_item_t command_item_relay_board_status = {
	.cmd = relay_boards_comm_proxy_command_enum(RELAY_BOARD_STATUS),
	.request_period = 0,
	.request_callback = request_relay_board_status,
	.response_callback = response_relay_board_status,
};

static command_item_t *relay_boards_comm_proxy_command_table[] = {
	&command_item_relay_board_heartbeat,
	&command_item_relay_boards_heartbeat,
	&command_item_relay_boards_config,
	&command_item_relay_board_status,
};

static void relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx, uint8_t relay_board_id, uint8_t state)
{
	relay_boards_comm_proxy_relay_board_ctx_t *relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	update_connect_state(&relay_board_ctx->connect_state, state);
}

uint8_t relay_boards_comm_proxy_get_connect_state(channels_info_t *channels_info, uint8_t relay_board_id)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	return get_connect_state(&relay_board_ctx->connect_state);
}

uint32_t relay_boards_comm_proxy_get_connect_stamp(channels_info_t *channels_info, uint8_t relay_board_id)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	relay_boards_comm_proxy_relay_board_ctx_t *relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	return get_connect_stamp(&relay_board_ctx->connect_state);
}

#define RESPONSE_TIMEOUT 200

static void relay_boards_comm_proxy_request_periodic(channels_info_t *channels_info)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	power_manager_info_t *power_manager_info = channels_info->power_manager_info;
	uint32_t ticks = osKernelSysTick();
	int i;
	int j;

	if(ticks_duration(ticks, relay_boards_comm_proxy_ctx->periodic_stamp) < 50) {
		return;
	}

	relay_boards_comm_proxy_ctx->periodic_stamp = ticks;

	for(j = 0; j < channels_info->relay_board_number; j++) {
		relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + j;
		power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + j;

		uint8_t connect_timeout = 0;

		if(ticks_duration(ticks, relay_boards_comm_proxy_get_connect_stamp(channels_info, j)) >= (3 * 1000)) {
			connect_timeout = 1;
		}

		if(get_fault(power_manager_relay_board_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
			set_fault(power_manager_relay_board_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT, connect_timeout);

			if(connect_timeout != 0) {
				debug("relay board %d timeout, ticks:%d, update stamp:%d",
				      j,
				      ticks,
				      relay_boards_comm_proxy_get_connect_stamp(channels_info, j));
			}
		}

		for(i = 0; i < ARRAY_SIZE(relay_boards_comm_proxy_command_table); i++) {
			command_item_t *item = relay_boards_comm_proxy_command_table[i];
			command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx, j, 0);
					debug("relay board %d, cmd %d(%s), index %d timeout, ticks:%d, send_stamp:%d, connect state:%d",
					      j,
					      item->cmd,
					      get_relay_boards_comm_proxy_command_des(item->cmd),
					      cmd_ctx->index,
					      ticks,
					      cmd_ctx->send_stamp,
					      relay_boards_comm_proxy_get_connect_state(channels_info, j));

					cmd_ctx->state = COMMAND_STATE_IDLE;

					if(item->timeout_callback != NULL) {
						item->timeout_callback(channels_info, item, j);
					}
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

				debug("relay board %d, cmd %d(%s), index %d start",
				      j,
				      item->cmd,
				      get_relay_boards_comm_proxy_command_des(item->cmd),
				      cmd_ctx->index,
				      relay_boards_comm_proxy_get_connect_state(channels_info, j));
			}
		}
	}
}

static void relay_boards_comm_proxy_request(channels_info_t *channels_info, can_info_t *can_info)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	int i;
	int j;
	int ret;

	for(j = 0; j < channels_info->relay_board_number; j++) {
		relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + j;

		for(i = 0; i < ARRAY_SIZE(relay_boards_comm_proxy_command_table); i++) {
			uint32_t ticks = osKernelSysTick();
			command_item_t *item = relay_boards_comm_proxy_command_table[i];
			command_status_t *cmd_ctx = relay_boards_comm_proxy_relay_board_ctx->cmd_ctx + item->cmd;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_tx_msg.Data;
			u_com_can_id_t *u_com_can_id = (u_com_can_id_t *)&relay_boards_comm_proxy_ctx->can_tx_msg.ExtId;

			relay_boards_comm_proxy_request_periodic(channels_info);

			if(cmd_ctx->available == 0) {
				continue;
			}

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_com_can_id->v = 0;
			u_com_can_id->s.type = PROXY_TYPE_RELAY_BOARD;
			u_com_can_id->s.flag = PROXY_FLAG_RELAY_BOARD;
			u_com_can_id->s.src_id = PROXY_ADDR_REMOTE_RELAY_BOARD;
			u_com_can_id->s.dst_id = j + 1;

			relay_boards_comm_proxy_ctx->can_tx_msg.IDE = CAN_ID_EXT;
			relay_boards_comm_proxy_ctx->can_tx_msg.RTR = CAN_RTR_DATA;
			relay_boards_comm_proxy_ctx->can_tx_msg.DLC = 8;

			memset(relay_boards_comm_proxy_ctx->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(channels_info, item, j);

			//debug("request relay board %d, cmd %d(%s), index:%d",
			//      j,
			//      item->cmd,
			//      get_relay_boards_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			if(ret != 0) {
				debug("process relay board %d, cmd %d(%s), index %d error!",
				      j,
				      item->cmd,
				      get_relay_boards_comm_proxy_command_des(item->cmd),
				      can_com_cmd_common->index);
				continue;
			}

			cmd_ctx->send_stamp = ticks;
			cmd_ctx->send_error = 0;
			ret = can_tx_data(can_info, &relay_boards_comm_proxy_ctx->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				cmd_ctx->state = COMMAND_STATE_REQUEST;
				cmd_ctx->send_error = 1;

				debug("send relay board %d, cmd %d(%s), index:%d error!",
				      j,
				      item->cmd,
				      get_relay_boards_comm_proxy_command_des(item->cmd),
				      can_com_cmd_common->index);
				relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx, j, 0);
			}
		}
	}
}

static void relay_boards_comm_proxy_response(channels_info_t *channels_info, can_rx_msg_t *can_rx_msg)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	int i;
	u_com_can_id_t *u_com_can_id;
	uint8_t relay_board_id;
	int ret;

	relay_boards_comm_proxy_ctx->can_rx_msg = can_rx_msg;

	u_com_can_id = (u_com_can_id_t *)&can_rx_msg->ExtId;

	if(u_com_can_id->s.type != PROXY_TYPE_RELAY_BOARD) {
		//debug("response type:%02x!", u_com_can_id->s.type);
		return;
	}

	if(u_com_can_id->s.flag != PROXY_FLAG_RELAY_BOARD) {
		//debug("response flag:%02x!", u_com_can_id->s.flag);
		return;
	}

	if(u_com_can_id->s.dst_id != PROXY_ADDR_REMOTE_RELAY_BOARD) {
		//debug("response dst_id:%02x!", u_com_can_id->s.dst_id);
		//debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		//      can_rx_msg->ExtId,
		//      can_rx_msg->Data[0],
		//      can_rx_msg->Data[1],
		//      can_rx_msg->Data[2],
		//      can_rx_msg->Data[3],
		//      can_rx_msg->Data[4],
		//      can_rx_msg->Data[5],
		//      can_rx_msg->Data[6],
		//      can_rx_msg->Data[7]);
		return;
	}

	relay_board_id = u_com_can_id->s.src_id - 1;

	if(relay_board_id >= channels_info->relay_board_number) {
		debug("relay_board_id:%d!", relay_board_id);
		//debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
		//      can_rx_msg->ExtId,
		//      can_rx_msg->Data[0],
		//      can_rx_msg->Data[1],
		//      can_rx_msg->Data[2],
		//      can_rx_msg->Data[3],
		//      can_rx_msg->Data[4],
		//      can_rx_msg->Data[5],
		//      can_rx_msg->Data[6],
		//      can_rx_msg->Data[7]);
		return;
	}

	for(i = 0; i < ARRAY_SIZE(relay_boards_comm_proxy_command_table); i++) {
		command_item_t *item = relay_boards_comm_proxy_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)relay_boards_comm_proxy_ctx->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			//debug("response relay board %d, cmd %d(%s), index:%d",
			//      relay_board_id,
			//      item->cmd,
			//      get_relay_boards_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx, relay_board_id, 1);

			ret = item->response_callback(channels_info, item, relay_board_id);

			if(ret != 0) {//收到响应
				debug("process response relay board %d, cmd %d(%s), index:%d error!",
				      relay_board_id,
				      item->cmd,
				      get_relay_boards_comm_proxy_command_des(item->cmd),
				      can_com_cmd_common->index);
				debug("rx extid:0x%08x, data:%02x %02x %02x %02x %02x %02x %02x %02x\n",
				      can_rx_msg->ExtId,
				      can_rx_msg->Data[0],
				      can_rx_msg->Data[1],
				      can_rx_msg->Data[2],
				      can_rx_msg->Data[3],
				      can_rx_msg->Data[4],
				      can_rx_msg->Data[5],
				      can_rx_msg->Data[6],
				      can_rx_msg->Data[7]);
			}

			break;
		}
	}

	return;
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;

	relay_boards_comm_proxy_request(channels_info, can_data_task_info->can_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;

	can_rx_msg_t *can_rx_msg = can_get_msg(can_data_task_info->can_info);

	relay_boards_comm_proxy_response(channels_info, can_rx_msg);
}

int start_relay_boards_comm_proxy_remote(channels_info_t *channels_info)
{
	int ret = 0;
	channels_config_t *channels_config = channels_info->channels_config;
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx;
	int i;

	if(channels_info->relay_board_number == 0) {
		return ret;
	}

	OS_ASSERT(channels_info->relay_boards_comm_proxy_ctx == NULL);

	relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)os_calloc(1, sizeof(relay_boards_comm_proxy_ctx_t));
	OS_ASSERT(relay_boards_comm_proxy_ctx != NULL);

	relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx = (relay_boards_comm_proxy_relay_board_ctx_t *)os_calloc(channels_info->relay_board_number, sizeof(relay_boards_comm_proxy_relay_board_ctx_t));
	OS_ASSERT(relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx != NULL);

	for(i = 0; i < channels_info->relay_board_number; i++) {
		int j;
		relay_boards_comm_proxy_relay_board_ctx_t *item = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + i;
		//relay_board_data_ctx_t *channel_data_ctx = &item->data_ctx;

		item->cmd_ctx = (command_status_t *)os_calloc(RELAY_BOARDS_COMM_PROXY_COMMAND_SIZE, sizeof(command_status_t));
		OS_ASSERT(item->cmd_ctx != NULL);

		for(j = 0; j < RELAY_BOARDS_COMM_PROXY_COMMAND_SIZE; j++) {
			item->cmd_ctx[j].available = 1;
		}
	}

	relay_boards_comm_proxy_ctx->can_data_task_info = get_or_alloc_can_data_task_info(channels_config->power_manager_config.hcan_relay_board);
	OS_ASSERT(relay_boards_comm_proxy_ctx->can_data_task_info != NULL);

	relay_boards_comm_proxy_ctx->can_data_request_cb.fn = can_data_request;
	relay_boards_comm_proxy_ctx->can_data_request_cb.fn_ctx = channels_info;
	add_can_data_task_info_request_cb(relay_boards_comm_proxy_ctx->can_data_task_info, &relay_boards_comm_proxy_ctx->can_data_request_cb);
	relay_boards_comm_proxy_ctx->can_data_response_cb.fn = can_data_response;
	relay_boards_comm_proxy_ctx->can_data_response_cb.fn_ctx = channels_info;
	add_can_data_task_info_response_cb(relay_boards_comm_proxy_ctx->can_data_task_info, &relay_boards_comm_proxy_ctx->can_data_response_cb);

	channels_info->relay_boards_comm_proxy_ctx = relay_boards_comm_proxy_ctx;

	return ret;
}
