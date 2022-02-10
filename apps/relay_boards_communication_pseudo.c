

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_communication_pseudo.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月14日 星期四 12时44分27秒
 *   修改日期：2021年06月09日 星期三 14时44分34秒
 *   描    述：
 *
 *================================================================*/
#include "relay_boards_communication.h"

#include <string.h>

#include "object_class.h"
#include "channels.h"

#include "relay_board_command.h"
#include "can_data_task.h"

//#define LOG_NONE
#include "log.h"

static object_class_t *relay_boards_com_class = NULL;

static uint32_t cmd_ctx_offset(uint8_t relay_board_id, uint8_t cmd)
{
	return RELAY_BOARD_CMD_TOTAL * relay_board_id + cmd;
}

int relay_boards_com_update_config(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id, uint8_t config)
{
	int ret = 0;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;
	channels_info_t *channels_info = (channels_info_t *)relay_boards_com_info->channels_info;
	relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + relay_board_id;

	if(relay_board_id >= relay_board_number) {
		return ret;
	}

	relay_board_item_info->remote_config = config;

	debug("relay_board_id:%d, config:%02x, remote_config:%02x", relay_board_id, config, relay_board_item_info->remote_config);

	return ret;
}

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

void relay_boards_com_request(relay_boards_com_info_t *relay_boards_com_info)
{
	int i;
	uint8_t relay_board_number = relay_boards_com_info->relay_board_number;

	for(i = 0; i < relay_board_number; i++) {
		relay_boards_com_set_connect_state(relay_boards_com_info, i, 1);
	}
}

int relay_boards_com_response(relay_boards_com_info_t *relay_boards_com_info, can_rx_msg_t *can_rx_msg)
{
	int ret = 0;
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
		//relay_boards_com_info->cmd_ctx[cmd_ctx_offset(i, RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT)].available = 1;
	}

	relay_boards_com_info->relay_boards_com_data_ctx = NULL;

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

