

/*================================================================
 *
 *
 *   文件名称：channels_communication_pseudo.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月14日 星期四 12时45分55秒
 *   修改日期：2021年06月09日 星期三 14时41分10秒
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

//#define LOG_NONE
#include "log.h"

static object_class_t *channels_com_class = NULL;

static uint32_t cmd_ctx_offset(uint8_t channel_id, uint8_t cmd)
{
	return CHANNEL_CMD_TOTAL * channel_id + cmd;
}

int channels_com_channel_module_assign_info(channels_com_info_t *channels_com_info, uint8_t channel_id, module_assign_info_t module_assign_info)
{
	int ret = 0;
	return ret;
}


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

void channels_com_request(channels_com_info_t *channels_com_info)
{
	int i;
	int j;
	uint8_t channel_number = channels_com_info->channel_number;
	channels_info_t *channels_info = (channels_info_t *)channels_com_info->channels_info;
	uint8_t power_module_item_number = channels_info->power_module_item_number;
	uint32_t ticks = osKernelSysTick();

	for(i = 0; i < channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		uint16_t charge_output_voltage = 0;//通道输出电压0.1v
		uint16_t charge_output_current = 0;//通道输出电流0.1a -400

		channels_com_set_connect_state(channels_com_info, i, 1);

		if(ticks_duration(ticks, channel_info->status.require_output_stamp) > 500) {
			channel_info->status.require_output_stamp = ticks;
		}

		for(j = 0; j < power_module_item_number; j++) {
			power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + j;
			struct list_head *head = &power_module_group_info->power_module_item_list;
			power_module_item_info_t *power_module_item_info;

			if(power_module_group_info->channel_info != channel_info) {
				continue;
			}

			list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
				charge_output_voltage = power_module_item_info->status.module_output_voltage;
				charge_output_current += power_module_item_info->status.module_output_current;
			}
		}

		channel_info->status.charge_output_voltage = charge_output_voltage;
		channel_info->status.charge_output_current = charge_output_current;
		channel_info->status.charge_output_stamp = ticks;
	}
}

int channels_com_response(channels_com_info_t *channels_com_info, can_rx_msg_t *can_rx_msg)
{
	int ret = 0;

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

	power_module_number = channels_info->channels_config->power_module_config.channels_power_module_number;

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
		}
	}

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

