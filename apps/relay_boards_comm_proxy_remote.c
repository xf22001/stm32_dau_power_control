

/*================================================================
 *
 *
 *   文件名称：relay_boards_comm_proxy_remote.c
 *   创 建 者：肖飞
 *   创建日期：2022年02月15日 星期二 08时45分38秒
 *   修改日期：2022年02月15日 星期二 17时31分15秒
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
#include "channel.h"

//#define LOG_DISABLE
#include "log.h"

typedef struct {
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

static command_item_t *relay_boards_comm_proxy_command_table[] = {
};

static void relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx, uint8_t relay_board_id, uint8_t state)
{
	relay_boards_comm_proxy_relay_board_ctx_t *channel_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	update_connect_state(&channel_ctx->connect_state, state);
}

uint8_t relay_boards_comm_proxy_get_connect_state(channels_info_t *channels_info, uint8_t relay_board_id)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	relay_boards_comm_proxy_relay_board_ctx_t *channel_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	return get_connect_state(&channel_ctx->connect_state);
}

uint32_t relay_boards_comm_proxy_get_connect_stamp(channels_info_t *channels_info, uint8_t relay_board_id)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	relay_boards_comm_proxy_relay_board_ctx_t *channel_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + relay_board_id;

	return get_connect_stamp(&channel_ctx->connect_state);
}

#define RESPONSE_TIMEOUT 200

static void relay_boards_comm_proxy_request_periodic(channels_info_t *channels_info)
{
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)channels_info->relay_boards_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	uint32_t ticks = osKernelSysTick();
	int i;
	int j;
	int k;

	if(ticks_duration(ticks, relay_boards_comm_proxy_ctx->periodic_stamp) < 50) {
		return;
	}

	relay_boards_comm_proxy_ctx->periodic_stamp = ticks;

	for(j = 0; j < proxy_channel_number; j++) {
		relay_boards_comm_proxy_relay_board_ctx_t *relay_boards_comm_proxy_relay_board_ctx = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + j;

		uint8_t connect_timeout;

		if(ticks_duration(ticks, relay_boards_comm_proxy_get_connect_stamp(channels_info, j)) >= (3 * 1000)) {
			connect_timeout = 1;
		} else {
			connect_timeout = 0;
		}

		for(k = 0; k < channels_info->channel_number; k++) {
			if() {
			}
		}

		if(get_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
			set_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT, connect_timeout);

			if(connect_timeout != 0) {
				debug("channel %d(%d) timeout, ticks:%d, update stamp:%d",
				      proxy_channel_item->channel_id,
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
					debug("channel %d(%d), cmd %d(%s), index %d timeout, ticks:%d, send_stamp:%d, connect state:%d",
					      proxy_channel_item->channel_id,
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

				debug("channel %d(%d), cmd %d(%s), index %d start",
				      proxy_channel_item->channel_id,
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
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	int i;
	int j;
	int ret;

	for(j = 0; j < proxy_channel_number; j++) {
		proxy_channel_item_t *proxy_channel_item = get_proxy_channel_item_by_proxy_channel_index(&channels_config->proxy_channel_info, j);
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
			u_com_can_id->s.type = PROXY_TYPE_CHANNEL;
			u_com_can_id->s.flag = PROXY_FLAG_CHANNEL;
			u_com_can_id->s.src_id = PROXY_ADDR_REMOTE_CHANNEL;
			u_com_can_id->s.dst_id = proxy_channel_item->channel_id + 1;

			relay_boards_comm_proxy_ctx->can_tx_msg.IDE = CAN_ID_EXT;
			relay_boards_comm_proxy_ctx->can_tx_msg.RTR = CAN_RTR_DATA;
			relay_boards_comm_proxy_ctx->can_tx_msg.DLC = 8;

			memset(relay_boards_comm_proxy_ctx->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(channels_info, item, j);

			//debug("request channel %d(%d), cmd %d(%s), index:%d",
			//      proxy_channel_item->channel_id,
			//      j,
			//      item->cmd,
			//      get_relay_boards_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			if(ret != 0) {
				debug("process channel %d(%d), cmd %d(%s), index %d error!",
				      proxy_channel_item->channel_id,
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

				debug("send channel %d(%d), cmd %d(%s), index:%d error!",
				      proxy_channel_item->channel_id,
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
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	int i;
	u_com_can_id_t *u_com_can_id;
	uint8_t channel_id;
	proxy_channel_item_t *proxy_channel_item;
	int ret;

	relay_boards_comm_proxy_ctx->can_rx_msg = can_rx_msg;

	u_com_can_id = (u_com_can_id_t *)&can_rx_msg->ExtId;

	if(u_com_can_id->s.type != PROXY_TYPE_CHANNEL) {
		//debug("response type:%02x!", u_com_can_id->s.type);
		return;
	}

	if(u_com_can_id->s.flag != PROXY_FLAG_CHANNEL) {
		//debug("response flag:%02x!", u_com_can_id->s.flag);
		return;
	}

	if(u_com_can_id->s.dst_id != PROXY_ADDR_REMOTE_CHANNEL) {
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

	channel_id = u_com_can_id->s.src_id - 1;

	if(channel_id >= channels_info->channel_number) {
		debug("channel_id:%d!", channel_id);
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

	proxy_channel_item = get_proxy_channel_item_by_channel_id(&channels_config->proxy_channel_info, channel_id);

	if(proxy_channel_item == NULL) {
		return;
	}

	if(proxy_channel_item->relay_board_id >= proxy_channel_number) {
		debug("relay_board_id:%d!", proxy_channel_item->relay_board_id);
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
			//debug("response channel %d(%d), cmd %d(%s), index:%d",
			//      proxy_channel_item->channel_id,
			//      proxy_channel_item->relay_board_id,
			//      item->cmd,
			//      get_relay_boards_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			relay_boards_comm_proxy_set_connect_state(relay_boards_comm_proxy_ctx, proxy_channel_item->relay_board_id, 1);

			ret = item->response_callback(channels_info, item, proxy_channel_item->relay_board_id);

			if(ret != 0) {//收到响应
				debug("process response channel %d(%d), cmd %d(%s), index:%d error!",
				      proxy_channel_item->channel_id,
				      proxy_channel_item->relay_board_id,
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
	relay_boards_comm_proxy_ctx_t *relay_boards_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	int i;
	power_manager_settings_t *power_manager_settings = &channels_info->channels_settings.power_manager_settings
	        uint8_t relay_board_number = 0;

	OS_ASSERT(channels_info->relay_boards_comm_proxy_ctx == NULL);

	relay_boards_comm_proxy_ctx = (relay_boards_comm_proxy_ctx_t *)os_calloc(1, sizeof(relay_boards_comm_proxy_ctx_t));
	OS_ASSERT(relay_boards_comm_proxy_ctx != NULL);

	for(i = 0; i < power_manager_settings->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];
		relay_board_number += power_manager_group_settings->channel_number * power_manager_group_settings->relay_board_number_per_channel;
	}

	channels_info->relay_board_number = relay_board_number;

	relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx = (relay_boards_comm_proxy_relay_board_ctx_t *)os_calloc(relay_board_number, sizeof(relay_boards_comm_proxy_relay_board_ctx_t));
	OS_ASSERT(relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx != NULL);

	for(i = 0; i < proxy_channel_number; i++) {
		int j;
		relay_boards_comm_proxy_relay_board_ctx_t *item = relay_boards_comm_proxy_ctx->relay_boards_comm_proxy_relay_board_ctx + i;
		//relay_board_data_ctx_t *channel_data_ctx = &item->data_ctx;
		proxy_channel_item_t *proxy_channel_item = get_proxy_channel_item_by_proxy_channel_index(&channels_config->proxy_channel_info, i);

		item->cmd_ctx = (command_status_t *)os_calloc(CHANNELS_COMM_PROXY_COMMAND_SIZE, sizeof(command_status_t));
		OS_ASSERT(item->cmd_ctx != NULL);

		for(j = 0; j < CHANNELS_COMM_PROXY_COMMAND_SIZE; j++) {
			item->cmd_ctx[j].available = 1;
		}
	}

	relay_boards_comm_proxy_ctx->can_data_task_info = get_or_alloc_can_data_task_info(channels_config->proxy_channel_info.hcan);
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
