

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy_remote.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月16日 星期四 10时34分46秒
 *   修改日期：2022年02月13日 星期日 20时44分33秒
 *   描    述：
 *
 *================================================================*/
#include "channels_comm_proxy_remote.h"

#include "proxy.h"
#include "channels_comm_proxy.h"
#include "connect_state.h"
#include "command_status.h"
#include "can_command.h"
#include "can_data_task.h"
#include "can_data_task.h"
#include "power_manager.h"

//#define LOG_DISABLE
#include "log.h"

typedef struct {
	proxy_remote_heartbeat_t proxy_remote_heartbeat;
	proxy_local_heartbeat_t proxy_local_heartbeat;
	proxy_remote_stateless_t proxy_remote_stateless;
	proxy_local_stateless_t proxy_local_stateless;
	proxy_local_channel_login_t proxy_local_channel_login;
} channel_data_ctx_t;

typedef struct {
	connect_state_t connect_state;
	command_status_t *cmd_ctx;
	channel_data_ctx_t data_ctx;
	callback_item_t channel_power_module_assign_ready_cb;
} channels_comm_proxy_channel_ctx_t;

typedef struct {
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;
	uint32_t periodic_stamp;
	can_data_task_info_t *can_data_task_info;
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;

	channels_comm_proxy_channel_ctx_t *channels_comm_proxy_channel_ctx;
} channels_comm_proxy_ctx_t;

typedef int (*request_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index);
typedef int (*response_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index);
typedef int (*timeout_callback_t)(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index);

typedef struct {
	uint8_t cmd;
	uint8_t cmd_code;
	uint8_t broadcast;
	uint32_t request_period;
	request_callback_t request_callback;
	response_callback_t response_callback;
	timeout_callback_t timeout_callback;
} command_item_t;

static int request_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index)
{
	return -1;
}

static int response_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index)
{
	return -1;
}

static int timeout_callback_proxy_null(channels_info_t *channels_info, void *_command_item, uint8_t proxy_channel_index)
{
	return -1;
}

static command_item_t *channels_comm_proxy_command_table[] = {
};

static void channels_comm_proxy_set_connect_state(channels_comm_proxy_ctx_t *channels_comm_proxy_ctx, uint8_t proxy_channel_index, uint8_t state)
{
	channels_comm_proxy_channel_ctx_t *channel_ctx = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + proxy_channel_index;

	update_connect_state(&channel_ctx->connect_state, state);
}

uint8_t channels_comm_proxy_get_connect_state(channels_info_t *channels_info, uint8_t proxy_channel_index)
{
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	channels_comm_proxy_channel_ctx_t *channel_ctx = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + proxy_channel_index;

	return get_connect_state(&channel_ctx->connect_state);
}

uint32_t channels_comm_proxy_get_connect_stamp(channels_info_t *channels_info, uint8_t proxy_channel_index)
{
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	channels_comm_proxy_channel_ctx_t *channel_ctx = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + proxy_channel_index;

	return get_connect_stamp(&channel_ctx->connect_state);
}

#define RESPONSE_TIMEOUT 200

static void channels_comm_proxy_request_periodic(channels_info_t *channels_info)
{
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	uint32_t ticks = osKernelSysTick();
	int i;
	int j;

	if(ticks_duration(ticks, channels_comm_proxy_ctx->periodic_stamp) < 50) {
		return;
	}

	channels_comm_proxy_ctx->periodic_stamp = ticks;

	for(j = 0; j < proxy_channel_number; j++) {
		proxy_channel_item_t *proxy_channel_item = get_proxy_channel_item_by_proxy_channel_index(&channels_config->proxy_channel_info, j);
		channels_comm_proxy_channel_ctx_t *channels_comm_proxy_channel_ctx = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + j;

		uint8_t connect_timeout;

		if(ticks_duration(ticks, channels_comm_proxy_get_connect_stamp(channels_info, j)) >= (10 * 1000)) {
			connect_timeout = 1;
		} else {
			connect_timeout = 0;
		}

		if(get_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
			set_fault(channel_info->faults, CHANNEL_FAULT_CONNECT_TIMEOUT, connect_timeout);

			if(connect_timeout != 0) {
				debug("channel %d(%d) timeout, ticks:%d, update stamp:%d",
				      proxy_channel_item->channel_id,
				      j,
				      ticks,
				      channels_comm_proxy_get_connect_stamp(channels_com_info, j));
				start_stop_channel(channels_info, channel_info->channel_id, CHANNEL_EVENT_TYPE_STOP_CHANNEL);
			}
		}

		for(i = 0; i < ARRAY_SIZE(channels_comm_proxy_command_table); i++) {
			command_item_t *item = channels_comm_proxy_command_table[i];
			command_status_t *cmd_ctx = channels_comm_proxy_channel_ctx->cmd_ctx + item->cmd;

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					channels_comm_proxy_set_connect_state(channels_comm_proxy_ctx, j, 0);
					debug("channel %d(%d), cmd %d(%s), index %d, timeout, connect state:%d",
					      proxy_channel_item->channel_id,
					      j,
					      item->cmd,
					      get_channels_comm_proxy_command_des(item->cmd),
					      cmd_ctx->index,
					      channels_comm_proxy_get_connect_state(channels_info, j));

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
				      get_channels_comm_proxy_command_des(item->cmd),
				      cmd_ctx->index,
				      channels_comm_proxy_get_connect_state(channels_info, j));
			}
		}
	}
}

static void channels_comm_proxy_request(channels_info_t *channels_info, can_info_t *can_info)
{
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	int i;
	int j;
	int ret;

	for(j = 0; j < proxy_channel_number; j++) {
		proxy_channel_item_t *proxy_channel_item = get_proxy_channel_item_by_proxy_channel_index(&channels_config->proxy_channel_info, j);
		channels_comm_proxy_channel_ctx_t *channels_comm_proxy_channel_ctx = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + j;

		for(i = 0; i < ARRAY_SIZE(channels_comm_proxy_command_table); i++) {
			uint32_t ticks = osKernelSysTick();
			command_item_t *item = channels_comm_proxy_command_table[i];
			command_status_t *cmd_ctx = channels_comm_proxy_channel_ctx->cmd_ctx + item->cmd;
			can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_comm_proxy_ctx->can_tx_msg.Data;
			u_com_can_id_t *u_com_can_id = (u_com_can_id_t *)&channels_comm_proxy_ctx->can_tx_msg.ExtId;

			channels_comm_proxy_request_periodic(channels_info);

			if(cmd_ctx->available == 0) {
				continue;
			}

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_com_can_id->v = 0;
			u_com_can_id->s.type = PROXY_TYPE_CHANNEL;
			u_com_can_id->s.flag = PROXY_FLAG;
			u_com_can_id->s.src_id = PROXY_ADDR_REMOTE;
			u_com_can_id->s.dst_id = proxy_channel_item->channel_id;

			channels_comm_proxy_ctx->can_tx_msg.IDE = CAN_ID_EXT;
			channels_comm_proxy_ctx->can_tx_msg.RTR = CAN_RTR_DATA;
			channels_comm_proxy_ctx->can_tx_msg.DLC = 8;

			memset(channels_comm_proxy_ctx->can_tx_msg.Data, 0, 8);

			can_com_cmd_common->cmd = item->cmd;

			ret = item->request_callback(channels_info, item, j);

			//debug("request channel %d(%d), cmd %d(%s), index:%d",
			//      proxy_channel_item->channel_id,
			//      j,
			//      item->cmd,
			//      get_channels_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			if(ret != 0) {
				debug("process channel %d(%d), cmd %d(%s), index:%d error!",
				      proxy_channel_item->channel_id,
				      j,
				      item->cmd,
				      get_channels_comm_proxy_command_des(item->cmd),
				      can_com_cmd_common->index);
				continue;
			}

			cmd_ctx->send_stamp = ticks;
			cmd_ctx->send_error = 0;
			ret = can_tx_data(can_info, &channels_comm_proxy_ctx->can_tx_msg, 10);

			if(ret != 0) {//发送失败
				cmd_ctx->state = COMMAND_STATE_REQUEST;
				cmd_ctx->send_error = 1;

				debug("send channel %d(%d), cmd %d(%s), index:%d error!",
				      proxy_channel_item->channel_id,
				      j,
				      item->cmd,
				      get_channels_comm_proxy_command_des(item->cmd),
				      can_com_cmd_common->index);
				channels_comm_proxy_set_connect_state(channels_comm_proxy_ctx, j, 0);
			}
		}
	}
}

static void channels_comm_proxy_response(channels_info_t *channels_info, can_rx_msg_t *can_rx_msg)
{
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;
	int i;
	u_com_can_id_t *u_com_can_id;
	uint8_t channel_id;
	proxy_channel_item_t *proxy_channel_item;
	int ret;

	channels_comm_proxy_ctx->can_rx_msg = can_rx_msg;

	u_com_can_id = (u_com_can_id_t *)&can_rx_msg->ExtId;

	if(u_com_can_id->s.type != PROXY_TYPE_CHANNEL) {
		//debug("response type:%02x!", u_com_can_id->s.type);
		return;
	}

	if(u_com_can_id->s.flag != PROXY_FLAG) {
		//debug("response flag:%02x!", u_com_can_id->s.flag);
		return;
	}

	if(u_com_can_id->s.dst_id != PROXY_ADDR_REMOTE) {
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

	channel_id = u_com_can_id->s.src_id;
	proxy_channel_item = get_proxy_channel_item_by_channel_id(&channels_config->proxy_channel_info, channel_id);

	if(proxy_channel_item == NULL) {
		return;
	}

	if(proxy_channel_item->proxy_channel_index >= proxy_channel_number) {
		debug("proxy_channel_index:%d!", proxy_channel_item->proxy_channel_index);
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

	for(i = 0; i < ARRAY_SIZE(channels_comm_proxy_command_table); i++) {
		command_item_t *item = channels_comm_proxy_command_table[i];
		can_com_cmd_common_t *can_com_cmd_common = (can_com_cmd_common_t *)channels_comm_proxy_ctx->can_rx_msg->Data;

		if(can_com_cmd_common->cmd == item->cmd) {
			//debug("response channel %d(%d), cmd %d(%s), index:%d",
			//      proxy_channel_item->channel_id,
			//      proxy_channel_item->proxy_channel_index,
			//      item->cmd,
			//      get_channels_comm_proxy_command_des(item->cmd),
			//      can_com_cmd_common->index);

			channels_comm_proxy_set_connect_state(channels_comm_proxy_ctx, proxy_channel_item->proxy_channel_index, 1);

			ret = item->response_callback(channels_info, item, proxy_channel_item->proxy_channel_index);

			if(ret != 0) {//收到响应
				debug("process response channel %d(%d), cmd %d(%s), index:%d error!",
				      proxy_channel_item->channel_id,
				      proxy_channel_item->proxy_channel_index,
				      item->cmd,
				      get_channels_comm_proxy_command_des(item->cmd),
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

	channels_comm_proxy_request(channels_info, can_data_task_info->can_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;

	can_rx_msg_t *can_rx_msg = can_get_msg(can_data_task_info->can_info);

	channels_comm_proxy_response(channels_info, can_rx_msg);
}

static int init_channels_config_proxy_channel_info(channels_config_t *channels_config)
{
	int ret = 0;
	int i;
	int j;

	channels_config->proxy_channel_info.proxy_channel_number = 0;

	OS_ASSERT(channels_config->proxy_channel_info.items == NULL);

	for(i = 0; i < channels_config->channel_number; i++) {
		channel_config_t *channel_config = channels_config->channel_config[i];

		if(channel_config->channel_type == CHANNEL_TYPE_PROXY_REMOTE) {
			channels_config->proxy_channel_info.proxy_channel_number++;
		}
	}

	if(channels_config->proxy_channel_info.proxy_channel_number == 0) {
		return ret;
	}

	channels_config->proxy_channel_info.items = (proxy_channel_item_t *)os_calloc(channels_config->proxy_channel_info.proxy_channel_number, sizeof(proxy_channel_item_t));
	OS_ASSERT(channels_config->proxy_channel_info.items != NULL);

	j = 0;

	for(i = 0; i < channels_config->channel_number; i++) {
		channel_config_t *channel_config = channels_config->channel_config[i];

		if(channel_config->channel_type == CHANNEL_TYPE_PROXY_REMOTE) {
			proxy_channel_item_t *item = channels_config->proxy_channel_info.items + j;
			item->proxy_channel_index = j;
			item->channel_id = i;

			debug("proxy remote channel %d index %d", item->channel_id, item->proxy_channel_index);

			j++;
		}
	}

	return ret;
}

static void channel_power_module_assign_ready(void *_proxy_channel_item, void *_channel_info)
{
	//proxy_channel_item_t *proxy_channel_item = (proxy_channel_item_t *)_proxy_channel_item;
	//channel_info_t *channel_info = (channel_info_t *)_channel_info;
	//channels_info_t *channels_info = (channels_info_t *)channel_info->channes_info;
	//channels_comm_proxy_ctx_t *channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)channels_info->channels_comm_proxy_ctx;
	//channels_comm_proxy_channel_ctx_t *item = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + proxy_channel_item->proxy_channel_index;
}

int start_channels_comm_proxy_remote(channels_info_t *channels_info)
{
	int ret = 0;
	channels_comm_proxy_ctx_t *channels_comm_proxy_ctx;
	channels_config_t *channels_config = channels_info->channels_config;
	uint8_t proxy_channel_number;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)channels_info->power_manager_info;
	int i;

	if(channels_info->channels_comm_proxy_ctx != NULL) {
		return ret;
	}

	init_channels_config_proxy_channel_info(channels_config);

	proxy_channel_number = channels_config->proxy_channel_info.proxy_channel_number;

	if(proxy_channel_number == 0) {
		return ret;
	}

	channels_comm_proxy_ctx = (channels_comm_proxy_ctx_t *)os_calloc(1, sizeof(channels_comm_proxy_ctx_t));
	OS_ASSERT(channels_comm_proxy_ctx != NULL);

	channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx = (channels_comm_proxy_channel_ctx_t *)os_calloc(proxy_channel_number, sizeof(channels_comm_proxy_channel_ctx_t));
	OS_ASSERT(channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx != NULL);

	for(i = 0; i < proxy_channel_number; i++) {
		int j;
		channels_comm_proxy_channel_ctx_t *item = channels_comm_proxy_ctx->channels_comm_proxy_channel_ctx + i;
		channel_data_ctx_t *channel_data_ctx = &item->data_ctx;
		proxy_channel_item_t *proxy_channel_item = get_proxy_channel_item_by_proxy_channel_index(&channels_config->proxy_channel_info, i);

		item->cmd_ctx = (command_status_t *)os_calloc(CHANNELS_COMM_PROXY_COMMAND_SIZE, sizeof(command_status_t));
		OS_ASSERT(item->cmd_ctx != NULL);

		item->cmd_ctx[channels_comm_proxy_command_enum(DAU_STATUS)].available = 1;
		item->cmd_ctx[channels_comm_proxy_command_enum(MODULES_STATUS)].available = 1;
		item->cmd_ctx[channels_comm_proxy_command_enum(INPUT_VOLTAGE)].available = 1;

		for(j = 0; j < sizeof(channel_data_ctx->proxy_remote_heartbeat); j++) {
			channel_data_ctx->proxy_remote_heartbeat.data[j] = j;
		}

		for(j = 0; j < sizeof(channel_data_ctx->proxy_remote_stateless); j++) {
			channel_data_ctx->proxy_remote_stateless.data[j] = j;
		}

		item->channel_power_module_assign_ready_cb.fn = channel_power_module_assign_ready;
		item->channel_power_module_assign_ready_cb.fn_ctx = proxy_channel_item;
		OS_ASSERT(register_callback(power_manager_info->power_manager_channel_module_assign_ready_chain, &item->channel_power_module_assign_ready_cb) == 0);
	}

	channels_comm_proxy_ctx->can_data_task_info = get_or_alloc_can_data_task_info(channels_config->proxy_channel_info.hcan);
	OS_ASSERT(channels_comm_proxy_ctx->can_data_task_info != NULL);

	channels_comm_proxy_ctx->can_data_request_cb.fn = can_data_request;
	channels_comm_proxy_ctx->can_data_request_cb.fn_ctx = channels_info;
	add_can_data_task_info_request_cb(channels_comm_proxy_ctx->can_data_task_info, &channels_comm_proxy_ctx->can_data_request_cb);
	channels_comm_proxy_ctx->can_data_response_cb.fn = can_data_response;
	channels_comm_proxy_ctx->can_data_response_cb.fn_ctx = channels_info;
	add_can_data_task_info_response_cb(channels_comm_proxy_ctx->can_data_task_info, &channels_comm_proxy_ctx->can_data_response_cb);

	channels_info->channels_comm_proxy_ctx = channels_comm_proxy_ctx;

	return ret;
}
