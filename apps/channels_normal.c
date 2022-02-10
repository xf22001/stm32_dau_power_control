

/*================================================================
 *
 *
 *   文件名称：channels.c
 *   创 建 者：肖飞
 *   创建日期：2020年06月18日 星期四 09时23分30秒
 *   修改日期：2022年02月10日 星期四 11时41分53秒
 *   描    述：
 *
 *================================================================*/
#include "channels.h"

#include <string.h>
#include <stdlib.h>

#include "os_utils.h"
#include "object_class.h"
#include "power_modules.h"
#include "channels_communication.h"
#include "relay_boards_communication.h"
#include "channel_command.h"
#include "modbus_data_value.h"
#include "modbus_addr_handler.h"
#include "config_layout.h"
#include "display.h"

#include "log.h"

typedef union {
	power_module_status_t s;
	uint16_t v;
} u_power_module_status_t;

static object_class_t *channels_class = NULL;

char *get_channel_state_des(channel_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_STATE_IDLE);
			add_des_case(CHANNEL_STATE_PREPARE_START);
			add_des_case(CHANNEL_STATE_START);
			add_des_case(CHANNEL_STATE_RUNNING);
			add_des_case(CHANNEL_STATE_PREPARE_STOP);
			add_des_case(CHANNEL_STATE_STOP);
			add_des_case(CHANNEL_STATE_RELAY_CHECK);

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_event_type_des(channel_event_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNEL_EVENT_TYPE_UNKNOW);
			add_des_case(CHANNEL_EVENT_TYPE_START_CHANNEL);
			add_des_case(CHANNEL_EVENT_TYPE_STOP_CHANNEL);
			add_des_case(CHANNEL_EVENT_TYPE_RELAY_CHECK);

		default: {
		}
		break;
	}

	return des;
}

char *get_channels_event_type_des(channels_event_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNELS_EVENT_CHANNEL_UNKNOW);
			add_des_case(CHANNELS_EVENT_CHANNEL_EVENT);

		default: {
		}
		break;
	}

	return des;
}

char *get_channels_change_state_des(channels_change_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNELS_CHANGE_STATE_IDLE);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_FREE);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_ASSIGN);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_READY_SYNC);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG);
			add_des_case(CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC);

		default: {
		}
		break;
	}

	return des;
}

char *get_power_module_policy_des(power_module_policy_t policy)
{
	char *des = "unknow";

	switch(policy) {
			add_des_case(POWER_MODULE_POLICY_AVERAGE);
			add_des_case(POWER_MODULE_POLICY_PRIORITY);

		default: {
		}
		break;
	}

	return des;
}

int set_fault(bitmap_t *faults, int fault, uint8_t v)
{
	return set_bitmap_value(faults, fault, v);
}

int get_fault(bitmap_t *faults, int fault)
{
	return get_bitmap_value(faults, fault);
}

int get_first_fault(bitmap_t *faults)
{
	int fault = get_first_value_index(faults, 1);

	if(fault >= faults->size) {
		fault = -1;
	}

	return fault;
}

static void print_power_module_item_info(const uint8_t pad, power_module_item_info_t *power_module_item_info)
{
	//u_power_module_status_t u_power_module_status;
	char *_pad = (char *)os_calloc(1, pad + 1);

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%smodule_id:%d\n", _pad, power_module_item_info->module_id);
	_printf("%s\tstate:%s\n", _pad, get_power_module_item_state_des(power_module_item_info->status.state));
	_printf("%s\trequire_output_voltage:%d\n", _pad, power_module_item_info->status.require_output_voltage);
	_printf("%s\trequire_output_current:%d\n", _pad, power_module_item_info->status.require_output_current);
	_printf("%s\tsetting_output_voltage:%d\n", _pad, power_module_item_info->status.setting_output_voltage);
	_printf("%s\tsetting_output_current:%d\n", _pad, power_module_item_info->status.setting_output_current);
	_printf("%s\tsmooth_setting_output_current:%d\n", _pad, power_module_item_info->status.smooth_setting_output_current);
	_printf("%s\trequire_relay_check_voltage:%d\n", _pad, power_module_item_info->status.require_relay_check_voltage);
	_printf("%s\trequire_relay_check_current:%d\n", _pad, power_module_item_info->status.require_relay_check_current);
	_printf("%s\tmodule_output_voltage:%d\n", _pad, power_module_item_info->status.module_output_voltage);
	_printf("%s\tmodule_output_current:%d\n", _pad, power_module_item_info->status.module_output_current);
	//_printf("%s\tconnect_state:%d\n", _pad, power_module_item_info->status.connect_state);
	//u_power_module_status.v = power_module_item_info->status.module_status;
	//_printf("%s\tpoweroff:%d\n", _pad, u_power_module_status.s.poweroff);
	//_printf("%s\tfault:%d\n", _pad, u_power_module_status.s.fault);
	//_printf("%s\toutput_state:%d\n", _pad, u_power_module_status.s.output_state);
	//_printf("%s\tfan_state:%d\n", _pad, u_power_module_status.s.fan_state);
	//_printf("%s\tinput_overvoltage:%d\n", _pad, u_power_module_status.s.input_overvoltage);
	//_printf("%s\tinput_lowvoltage:%d\n", _pad, u_power_module_status.s.input_lowvoltage);
	//_printf("%s\toutput_overvoltage:%d\n", _pad, u_power_module_status.s.output_overvoltage);
	//_printf("%s\toutput_lowvoltage:%d\n", _pad, u_power_module_status.s.output_lowvoltage);
	//_printf("%s\tprotect_overcurrent:%d\n", _pad, u_power_module_status.s.protect_overcurrent);
	//_printf("%s\tprotect_overtemperature:%d\n", _pad, u_power_module_status.s.protect_overtemperature);
	//_printf("%s\tsetting_poweroff:%d\n", _pad, u_power_module_status.s.setting_poweroff);

	os_free(_pad);
}

static void print_power_module_group_info(const uint8_t pad, power_module_group_info_t *power_module_group_info)
{
	char *_pad = (char *)os_calloc(1, pad + 1);
	struct list_head *head = &power_module_group_info->power_module_item_list;
	power_module_item_info_t *power_module_item_info;

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%spdu_group_id:%d\n", _pad, power_module_group_info->pdu_group_info->pdu_group_id);
	_printf("%sgroup_id:%d\n", _pad, power_module_group_info->group_id);
	list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
		print_power_module_item_info(pad + 1, power_module_item_info);
	}

	os_free(_pad);
}

static void print_relay_board_item_info(const uint8_t pad, relay_board_item_info_t *relay_board_item_info)
{
	char *_pad = (char *)os_calloc(1, pad + 1);

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%sboard_id:%d\n", _pad, relay_board_item_info->board_id);
	_printf("%s\tchannel_id:%d\n", _pad, relay_board_item_info->channel_id);
	_printf("%s\toffset:%d\n", _pad, relay_board_item_info->offset);
	_printf("%s\tnumber:%d\n", _pad, relay_board_item_info->number);
	_printf("%s\tconfig:0b%08x\n", _pad, u8_bin(relay_board_item_info->config));
	_printf("%s\tremote_config:0b%08x\n", _pad, u8_bin(relay_board_item_info->remote_config));
	//_printf("%s\ttemperature1:%d\n", _pad, relay_board_item_info->temperature1);
	//_printf("%s\ttemperature2:%d\n", _pad, relay_board_item_info->temperature2);
	//_printf("%s\tconnect_state:%d\n", _pad, relay_board_item_info->connect_state);

	os_free(_pad);
}

static void print_channel_info(const uint8_t pad, channel_info_t *channel_info)
{
	char *_pad = (char *)os_calloc(1, pad + 1);

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%schannel_id:%d\n", _pad, channel_info->channel_id);
	_printf("%s\tstate:%s\n", _pad, get_channel_state_des(channel_info->status.state));
	_printf("%s\trelay_board_number:%d\n", _pad, channel_info->relay_board_number);
	_printf("%s\trequire_output_voltage:%d\n", _pad, channel_info->status.require_output_voltage);
	_printf("%s\trequire_output_current:%d\n", _pad, channel_info->status.require_output_current);
	_printf("%s\trequire_work_state:%d\n", _pad, channel_info->status.require_work_state);
	_printf("%s\treassign:%d\n", _pad, channel_info->status.reassign);
	_printf("%s\treassign_module_number:%d\n", _pad, channel_info->status.reassign_module_group_number);
	_printf("%s\tcharge_output_voltage:%d\n", _pad, channel_info->status.charge_output_voltage);
	_printf("%s\tcharge_output_current:%d\n", _pad, channel_info->status.charge_output_current);
	//_printf("%s\tconnect_state:%d\n", _pad, channel_info->status.connect_state);

	os_free(_pad);
}

static int try_to_start_channel(channel_info_t *channel_info)
{
	int ret = -1;

	if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_START) {
		return ret;
	}

	channel_info->channel_request_state = CHANNEL_REQUEST_STATE_START;

	switch(channel_info->status.state) {
		case CHANNEL_STATE_IDLE: {
			debug("channel_id %d start", channel_info->channel_id);
			channel_info->status.module_ready_notify = 1;

			ret = 0;
		}
		break;

		case CHANNEL_STATE_PREPARE_STOP:
		case CHANNEL_STATE_STOP: {
			channels_info_t *channels_info = channel_info->channels_info;
			channels_com_info_t *channels_com_info = (channels_com_info_t *)channels_info->channels_com_info;

			debug("channel_id %d start with state %s, retry!",
			      channel_info->channel_id,
			      get_channel_state_des(channel_info->status.state));
			channels_com_channel_module_assign_info(channels_com_info, channel_info->channel_id, MODULE_ASSIGN_INFO_RETRY);
		}
		break;

		default: {
			debug("channel_id %d start with state %s",
			      channel_info->channel_id,
			      get_channel_state_des(channel_info->status.state));
		}
		break;
	}

	return ret;
}

static int try_to_stop_channel(channel_info_t *channel_info)
{
	int ret = -1;

	if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_STOP) {
		return ret;
	}

	channel_info->channel_request_state = CHANNEL_REQUEST_STATE_STOP;

	if(channel_info->status.state != CHANNEL_STATE_IDLE) {
		debug("channel_id %d stop", channel_info->channel_id);

		ret = 0;
	} else {
		debug("channel_id %d stop with state %s",
		      channel_info->channel_id,
		      get_channel_state_des(channel_info->status.state));
	}

	return ret;
}

static int try_to_relay_check(channel_info_t *channel_info)
{
	int ret = -1;

	channel_info->channel_request_state = CHANNEL_REQUEST_STATE_RELAY_CHECK;

	if(channel_info->status.state == CHANNEL_STATE_IDLE) {
		debug("channel_id %d start relay check", channel_info->channel_id);

		ret = 0;
	} else {
		debug("channel_id %d start relay check with state %s",
		      channel_info->channel_id,
		      get_channel_state_des(channel_info->status.state));
	}

	return ret;
}

static void default_handle_channel_event(void *_channel_info, void *_channels_event)
{
	uint8_t match = 0;

	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;
	channel_event_t *channel_event = channels_event->event;

	if(channels_event->type != CHANNELS_EVENT_CHANNEL_EVENT) {
		return;
	}

	if(channel_event == NULL) {
		return;
	}

	if(channel_event->channel_id == channel_info->channel_id) {
		match = 1;
	} else if(channel_event->channel_id == 0xff) {
		match = 1;
	}

	if(match == 0) {
		return;
	}

	switch(channel_event->type) {
		case CHANNEL_EVENT_TYPE_START_CHANNEL: {
			//debug("channel_id %d process event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

			if(try_to_start_channel(channel_info) != 0) {
			}
		}
		break;

		case CHANNEL_EVENT_TYPE_STOP_CHANNEL: {
			if(channel_info->status.state != CHANNEL_STATE_IDLE) {
				//debug("channel_id %d process event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

				if(try_to_stop_channel(channel_info) != 0) {
				}
			}
		}
		break;

		case CHANNEL_EVENT_TYPE_RELAY_CHECK: {
			if(try_to_relay_check(channel_info) != 0) {
			}
		}

		default: {
		}
		break;
	}
}

static uint16_t get_single_module_max_output_current(channels_settings_t *channels_settings)
{
	if(channels_settings->module_max_output_current == 0) {
		return 60 * 10;
	}

	return channels_settings->module_max_output_current;
}

#define CHANNEL_INFO_PREPARE_START_TIMEOUT (30 * 1000)
#define CHANNEL_INFO_PREPARE_STOP_TIMEOUT (30 * 1000)

static void module_voltage_current_correction(channels_settings_t *channels_settings, uint16_t *voltage, uint16_t *current)
{
	if(*voltage == 0) {
		*current = 0;
		return;
	}

	if(*current == 0) {
		*voltage = 0;
		return;
	}

	if(*voltage > channels_settings->module_max_output_voltage) {
		*voltage = channels_settings->module_max_output_voltage;
	}

	if(*voltage < channels_settings->module_min_output_voltage) {
		*voltage = channels_settings->module_min_output_voltage;
	}

	if(*current > channels_settings->module_max_output_current) {
		*current = channels_settings->module_max_output_current;
	}

	if(*current < channels_settings->module_min_output_current) {
		*current = channels_settings->module_min_output_current;
	}
}

static void module_power_limit_correction(channels_settings_t *channels_settings, uint16_t *voltage, uint16_t *current)
{
	uint32_t power = *voltage * *current;

	if(channels_settings->module_max_output_power != 0) {
		uint32_t max_power = channels_settings->module_max_output_power * 100;

		if(power > max_power) {
			uint16_t limit_current = max_power / *voltage;

			if(limit_current < *current) {
				*current = limit_current;
			}
		}
	}
}

static void handle_channel_state(channel_info_t *channel_info)
{
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;
	uint8_t channel_id = channel_info->channel_id;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channel_info->status.connect_state_stamp) >= 1 * 1000) {
		channels_info_t *channels_info = channel_info->channels_info;
		channels_com_info_t *channels_com_info = (channels_com_info_t *)channels_info->channels_com_info;
		channel_info->status.connect_state = channels_com_get_connect_state(channels_com_info, channel_id);
	}

	switch(channel_info->status.state) {
		case CHANNEL_STATE_IDLE: {
			if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_START) {//开机
				channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
				pdu_config_t *pdu_config = &channels_info->channels_settings.pdu_config;

				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;

				if(pdu_config->policy == POWER_MODULE_POLICY_PRIORITY) {
					channel_info->status.reassign = 1;//使能二次重分配
					channel_info->status.reassign_module_group_number = 1;
				} else {
					channel_info->status.reassign = 0;
					channel_info->status.reassign_module_group_number = 0;
				}

				list_move_tail(&channel_info->list, &pdu_group_info->channel_active_list);
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE;//模块组重分配

				channel_info->channel_state_change_stamp = ticks;
				channel_info->status.state = CHANNEL_STATE_PREPARE_START;
				debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
			} else if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_RELAY_CHECK) {
				channels_info_t *channels_info = channel_info->channels_info;
				relay_check_info_t *relay_check_info = &channels_info->relay_check_info;

				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;

				relay_check_info->relay_check_request_state = RELAY_CHECK_REQUEST_STATE_START;
				channel_info->status.state = CHANNEL_STATE_RELAY_CHECK;
				debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
			}
		}
		break;

		case CHANNEL_STATE_PREPARE_START: {
			if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_STOP) {//关机
				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;

				list_move_tail(&channel_info->list, &pdu_group_info->channel_deactive_list);
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组

				channel_info->channel_state_change_stamp = ticks;
				channel_info->status.state = CHANNEL_STATE_PREPARE_STOP;
				debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
			} else {
				if(list_contain(&channel_info->list, &pdu_group_info->channel_active_list) != 0) {//无法启动，停止启动过程
					channel_info->status.state = CHANNEL_STATE_IDLE;
					debug("channel_id %d start failed! channel_id %d to state %s failed!", channel_id, channel_id, get_channel_state_des(channel_info->status.state));
				} else {
					if(pdu_group_info->channels_change_state != CHANNELS_CHANGE_STATE_IDLE) {//配置没有同步完成
						if(ticks_duration(ticks, channel_info->channel_state_change_stamp) >= CHANNEL_INFO_PREPARE_START_TIMEOUT) {//超时,报警?
							debug("channels_change_state %s! channel_id %d in state %s!",
							      get_channels_change_state_des(pdu_group_info->channels_change_state),
							      channel_id,
							      get_channel_state_des(channel_info->status.state));

							list_move_tail(&channel_info->list, &pdu_group_info->channel_deactive_list);
							pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组

							channel_info->channel_state_change_stamp = ticks;
							channel_info->status.state = CHANNEL_STATE_PREPARE_STOP;
							debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
						}
					} else {
						channel_info->status.state = CHANNEL_STATE_START;
						debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
					}
				}
			}
		}
		break;

		case CHANNEL_STATE_START: {
			channel_info->status.state = CHANNEL_STATE_RUNNING;
			debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
		}
		break;

		case CHANNEL_STATE_RUNNING: {
			if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_STOP) {//关机
				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;

				list_move_tail(&channel_info->list, &pdu_group_info->channel_deactive_list);
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组

				channel_info->channel_state_change_stamp = ticks;
				channel_info->status.state = CHANNEL_STATE_PREPARE_STOP;
				debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
			} else {
				channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
				pdu_config_t *pdu_config = &channels_info->channels_settings.pdu_config;
				channels_settings_t *channels_settings = &channels_info->channels_settings;

				if(pdu_config->policy == POWER_MODULE_POLICY_PRIORITY) {
					if(channel_info->status.require_work_state == CHANNEL_WORK_STATE_CHARGE) {
						uint16_t single_module_max_output_current = get_single_module_max_output_current(channels_settings);
						uint16_t require_output_current = channel_info->status.require_output_current;
						uint16_t require_output_voltage = channel_info->status.require_output_voltage;

						module_voltage_current_correction(channels_settings, &require_output_voltage, &single_module_max_output_current);

						//debug("channel_id %d require_output_voltage:%d", channel_id, require_output_voltage);
						//debug("channel_id %d single_module_max_output_current:%d", channel_id, single_module_max_output_current);

						if(channel_info->status.charge_output_voltage != 0) {
							require_output_voltage = channel_info->status.charge_output_voltage;
							//debug("channel_id %d require_output_voltage:%d", channel_id, require_output_voltage);
						}

						module_power_limit_correction(channels_settings, &require_output_voltage, &single_module_max_output_current);

						//debug("channel_id %d single_module_max_output_current:%d", channel_id, single_module_max_output_current);

						if(single_module_max_output_current != 0) {
							pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;
							pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[pdu_group_info->pdu_group_id];
							uint16_t module_group_output_current = single_module_max_output_current * pdu_group_config->power_module_number_per_power_module_group;

							if(channel_info->status.reassign == 1) {
								channel_info->status.reassign = 0;
								channel_info->status.reassign_module_group_number = (require_output_current + (module_group_output_current - 1)) / module_group_output_current;
								debug("channel_id %d require_output_current:%d", channel_id, require_output_current);
								debug("channel_id %d module_group_output_current:%d", channel_id, module_group_output_current);


								debug("channel_id %d reassign_module_group_number:%d", channel_id, channel_info->status.reassign_module_group_number);
								pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE;//模块组重分配
							} else {
								if(pdu_group_info->channels_change_state == CHANNELS_CHANGE_STATE_IDLE) {
									if(channel_info->status.reassign_module_group_number != ((require_output_current + (module_group_output_current - 1)) / module_group_output_current)) {
										debug("request channel_id %d reassign!!!!!!!!!", channel_id);
										channel_info->status.reassign = 1;
									}
								}
							}

						} else {
							debug("channel_id %d require_output_current:%d, require_output_voltage:%d", channel_info->channel_id, require_output_current, require_output_voltage);
						}
					}
				}
			}
		}
		break;

		case CHANNEL_STATE_PREPARE_STOP: {
			if(pdu_group_info->channels_change_state != CHANNELS_CHANGE_STATE_IDLE) {//配置没有同步完成
				//if(ticks_duration(ticks, channel_info->channel_state_change_stamp) >= CHANNEL_INFO_PREPARE_STOP_TIMEOUT) {//超时,报警?
				//	debug("channels_change_state %s! channel_id %d in state %s!", get_channels_change_state_des(pdu_group_info->channels_change_state), channel_id, get_channel_state_des(channel_info->status.state));
				//}
			} else {
				channel_info->status.state = CHANNEL_STATE_STOP;
				debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
			}

		}
		break;

		case CHANNEL_STATE_STOP: {
			channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;

			list_move_tail(&channel_info->list, &pdu_group_info->channel_idle_list);

			channel_info->status.state = CHANNEL_STATE_IDLE;
			debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
		}
		break;

		case CHANNEL_STATE_RELAY_CHECK: {//自检中
			channels_info_t *channels_info = channel_info->channels_info;
			relay_check_info_t *relay_check_info = &channels_info->relay_check_info;

			if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_STOP) {//关机
				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;
				relay_check_info->relay_check_request_state = RELAY_CHECK_REQUEST_STATE_STOP;
			} else {
				if(relay_check_info->relay_check_state == RELAY_CHECK_STATE_NONE) {//自检操作完成
					channel_info->status.state = CHANNEL_STATE_IDLE;
					debug("channel_id %d to state %s", channel_id, get_channel_state_des(channel_info->status.state));
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_power_module_group_info(channel_info_t *channel_info, uint16_t module_require_voltage, uint16_t module_group_require_current)
{
	power_module_group_info_t *power_module_group_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	struct list_head *head = &channel_info->power_module_group_list;
	list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		uint16_t module_require_current;
		power_module_item_info_t *power_module_item_info;
		uint8_t power_module_number_per_power_module_group = list_size(head1);

		if(power_module_number_per_power_module_group == 0) {
			debug("");
			continue;
		}

		module_require_current = module_group_require_current / power_module_number_per_power_module_group;

		if((module_require_current == 0) && (channel_info->status.require_output_current != 0)) {
			module_require_current = channels_settings->module_min_output_current;
			//debug("module_require_current:%d", module_require_current);
		}

		module_voltage_current_correction(channels_settings, &module_require_voltage, &module_require_current);
		module_power_limit_correction(channels_settings, &channel_info->status.charge_output_voltage, &module_require_current);

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.require_output_voltage = module_require_voltage;
			power_module_item_info->status.require_output_current = module_require_current;
			//debug("setting module_id:%d, require_output_voltage:%d, require_output_current:%d",
			//      power_module_item_info->module_id,
			//      power_module_item_info->status.require_output_voltage,
			//      power_module_item_info->status.require_output_current);
		}
	}
}

static void channel_power_limit_correction(uint32_t channel_max_output_power, uint16_t *voltage, uint16_t *current)
{
	uint32_t power = *voltage * *current;

	uint32_t max_power = channel_max_output_power * 100;

	if(power > max_power) {
		*current = max_power / *voltage;
	}
}

static void handle_channel_power_module_group_info(channel_info_t *channel_info)
{
	struct list_head *head;
	uint8_t module_group_size;
	uint16_t module_require_voltage;
	uint16_t module_group_require_current;
	uint16_t charge_output_voltage = channel_info->status.charge_output_voltage;
	uint16_t require_output_current = channel_info->status.require_output_current;

	head = &channel_info->power_module_group_list;

	module_group_size = list_size(head);

	if(module_group_size == 0) {
		return;
	}

	//if(channel_info->status.require_output_stamp == channel_info->status.module_sync_stamp) {
	//	return;
	//}

	//channel_info->status.module_sync_stamp = channel_info->status.require_output_stamp;

	//debug("setting channel_id %d, require_output_voltage:%d, require_output_current:%d",
	//      channel_info->channel_id,
	//      channel_info->status.require_output_voltage,
	//      channel_info->status.require_output_current);

	module_require_voltage = channel_info->status.require_output_voltage;
	channel_power_limit_correction(channel_info->channel_max_output_power, &charge_output_voltage, &require_output_current);
	//debug("channel %d channel_max_output_power:%d, charge_output_voltage:%d, require_output_current:%d",
	//      channel_info->channel_id,
	//      channel_info->channel_max_output_power,
	//      charge_output_voltage,
	//      require_output_current);
	module_group_require_current = require_output_current / module_group_size;

	handle_power_module_group_info(channel_info, module_require_voltage, module_group_require_current);
}

static void default_handle_channel_periodic(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;

	handle_channel_power_module_group_info(channel_info);
	handle_channel_state(channel_info);
}

void free_channels_info(channels_info_t *channels_info)
{
	app_panic();
}

static uint8_t get_power_module_group_number_per_pdu_group_config(pdu_group_config_t *pdu_group_config)
{
	uint8_t power_module_group_number = 0;
	int i;

	//当前组任意一个通道每个开关板上接的模块组数累加，就是当前组的模块组数
	for(i = 0; i < pdu_group_config->relay_board_number_per_channel; i++) {
		power_module_group_number += pdu_group_config->power_module_group_number_per_relay_board[i];
	}

	return power_module_group_number;
}

static uint8_t get_power_module_group_number(pdu_config_t *pdu_config)
{
	uint8_t power_module_group_number = 0;
	int i;

	for(i = 0; i < pdu_config->pdu_group_number; i++) {
		pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[i];
		power_module_group_number += get_power_module_group_number_per_pdu_group_config(pdu_group_config);
	}

	return power_module_group_number;
}

static uint8_t get_power_module_item_number_per_pdu_group_config(pdu_group_config_t *pdu_group_config)
{
	uint8_t power_module_number = 0;
	int i;

	//当前组任意一个通道每个开关板上接的模块组内的模块数数累加，就是当前组的模块数
	for(i = 0; i < pdu_group_config->relay_board_number_per_channel; i++) {
		power_module_number += pdu_group_config->power_module_group_number_per_relay_board[i] * pdu_group_config->power_module_number_per_power_module_group;
	}

	return power_module_number;
}

static uint8_t get_power_module_item_number(pdu_config_t *pdu_config)
{
	uint8_t power_module_number = 0;
	int i;

	for(i = 0; i < pdu_config->pdu_group_number; i++) {
		pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[i];
		power_module_number += get_power_module_item_number_per_pdu_group_config(pdu_group_config);
	}

	return power_module_number;
}

static uint8_t get_channel_number(pdu_config_t *pdu_config)
{
	uint8_t channel_number = 0;
	int i;

	for(i = 0; i < pdu_config->pdu_group_number; i++) {
		pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[i];
		channel_number += pdu_group_config->channel_number;
	}

	return channel_number;
}

static uint8_t get_relay_board_item_number(pdu_config_t *pdu_config)
{
	uint8_t relay_board_item_number = 0;
	int i;

	for(i = 0; i < pdu_config->pdu_group_number; i++) {
		pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[i];

		relay_board_item_number += pdu_group_config->channel_number * pdu_group_config->relay_board_number_per_channel;
	}

	return relay_board_item_number;
}

static int channels_info_load_config(channels_info_t *channels_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.storage_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return load_config_item(channels_info->storage_info, "dau", (uint8_t *)&channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

static int channels_info_save_config(channels_info_t *channels_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.storage_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return save_config_item(channels_info->storage_info, "dau", (uint8_t *)&channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

static void restore_channels_settings(channels_settings_t *channels_settings)
{
	pdu_config_t *pdu_config = &channels_settings->pdu_config;
	pdu_group_config_t *pdu_group_config;

	memset(channels_settings, 0, sizeof(channels_settings_t));

	//pdu_config->policy = POWER_MODULE_POLICY_AVERAGE;
	pdu_config->policy = POWER_MODULE_POLICY_PRIORITY;

	pdu_config->pdu_group_number = 2;

	pdu_group_config = &pdu_config->pdu_group_config[0];
	pdu_group_config->channel_number = 5;
	pdu_group_config->relay_board_number_per_channel = 1;
	pdu_group_config->power_module_group_number_per_relay_board[0] = 5;
	pdu_group_config->power_module_group_number_per_relay_board[1] = 0;
	pdu_group_config->power_module_number_per_power_module_group = 2;

	pdu_group_config = &pdu_config->pdu_group_config[1];
	pdu_group_config->channel_number = 5;
	pdu_group_config->relay_board_number_per_channel = 1;
	pdu_group_config->power_module_group_number_per_relay_board[0] = 5;
	pdu_group_config->power_module_group_number_per_relay_board[1] = 0;
	pdu_group_config->power_module_number_per_power_module_group = 2;

	channels_settings->power_module_settings.power_module_type = POWER_MODULE_TYPE_WINLINE;
	channels_settings->power_module_settings.rate_current = 21;
	channels_settings->module_max_output_voltage = 10000;
	channels_settings->module_min_output_voltage = 2000;
	channels_settings->module_max_output_current = 1000;
	channels_settings->module_min_output_current = 1;
	channels_settings->module_max_output_power = 30000;
	channels_settings->channels_max_output_power = 400000;
}


static void init_channels_settings(channels_info_t *channels_info)
{
	int ret = -1;
	channels_settings_t *channels_settings = &channels_info->channels_settings;

	ret = channels_info_load_config(channels_info);

	if(ret == 0) {
		if(app_get_reset_config() != 0) {
			ret = -1;
		}
	}

	if(ret != 0) {
		debug("load config failed! restore config...");
		restore_channels_settings(channels_settings);
		channels_info_save_config(channels_info);
	} else {
		debug("load config successfully!");
	}

	load_channels_display_cache(channels_info);
}

static void channel_info_deactive_power_module_group(channel_info_t *channel_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;

	head = &channel_info->power_module_group_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_deactive_list);
	}
}

static void free_power_module_group_for_stop_channel(pdu_group_info_t *pdu_group_info)
{
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_deactive_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		channel_info_deactive_power_module_group(channel_info);
	}
}

static void channel_info_deactive_unneeded_power_module_group_priority(channel_info_t *channel_info)//POWER_MODULE_POLICY_PRIORITY
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;
	uint8_t module_group_size;
	uint8_t power_module_group_to_remove_count = 0;

	head = &channel_info->power_module_group_list;
	module_group_size = list_size(head);

	debug("channel_id %d reassign_module_group_number:%d", channel_info->channel_id, channel_info->status.reassign_module_group_number);

	if(module_group_size <= channel_info->status.reassign_module_group_number) {
		return;
	}

	power_module_group_to_remove_count = module_group_size - channel_info->status.reassign_module_group_number;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_deactive_list);
		power_module_group_to_remove_count--;

		if(power_module_group_to_remove_count == 0) {
			break;
		}
	}
}

static void free_power_module_group_for_active_channel_priority(pdu_group_info_t *pdu_group_info)//POWER_MODULE_POLICY_PRIORITY
{
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		channel_info_deactive_unneeded_power_module_group_priority(channel_info);
	}
}

static uint8_t get_disable_power_group_count(pdu_group_info_t *pdu_group_info)
{
	uint8_t count = 0;

	count = list_size(&pdu_group_info->power_module_group_disable_list);

	return count;
}

static uint8_t get_available_power_group_count(pdu_group_info_t *pdu_group_info)
{
	uint8_t count = 0;

	//计算空闲的模块组数
	count += list_size(&pdu_group_info->power_module_group_idle_list);

	//计算将要被回收的模块组数
	count += list_size(&pdu_group_info->power_module_group_deactive_list);

	return count;
}

static uint8_t get_more_power_group_count_need(pdu_group_info_t *pdu_group_info)
{
	uint8_t count = 0;
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		if(list_size(&channel_info->power_module_group_list) == 0) {
			count++;
		}
	}
	return count;
}

//要启动的通道数
static uint8_t get_start_channel_count(pdu_group_info_t *pdu_group_info)
{
	uint8_t count = 0;
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		if(channel_info->status.state == CHANNEL_STATE_PREPARE_START) {
			count++;
		}
	}
	return count;
}

static void cancel_last_start_channel(pdu_group_info_t *pdu_group_info)
{
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	channel_info = list_last_entry(head, channel_info_t, list);

	list_move_tail(&channel_info->list, &pdu_group_info->channel_idle_list);
}

static void channel_free_power_module_group(channel_info_t *channel_info, uint8_t channel_free_power_module_group_count)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;

	if(channel_free_power_module_group_count == 0) {
		return;
	}

	head = &channel_info->power_module_group_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_deactive_list);
		channel_free_power_module_group_count--;

		if(channel_free_power_module_group_count == 0) {
			return;
		}
	}
}

static void active_channel_list_remove_power_module_group(pdu_group_info_t *pdu_group_info, uint8_t more_power_module_group_count_need, uint8_t keep_count)
{
	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	if(more_power_module_group_count_need == 0) {
		return;
	}

	list_for_each_entry_reverse(channel_info, head, channel_info_t, list) {
		uint8_t power_module_group_count;
		uint8_t channel_free_power_module_group_count;

		power_module_group_count = list_size(&channel_info->power_module_group_list);

		if(power_module_group_count <= keep_count) {//没有可以剔除的模块组
			continue;
		}

		channel_free_power_module_group_count = power_module_group_count - keep_count;

		if(channel_free_power_module_group_count > more_power_module_group_count_need) {
			channel_free_power_module_group_count = more_power_module_group_count_need;
		}

		if(channel_free_power_module_group_count > 0) {
			channel_free_power_module_group(channel_info, channel_free_power_module_group_count);
			more_power_module_group_count_need -= channel_free_power_module_group_count;
		}

		if(more_power_module_group_count_need == 0) {
			break;
		}
	}

	if(more_power_module_group_count_need != 0) {
		debug("bug!!! more_power_module_group_count_need:%d", more_power_module_group_count_need);
	}
}

static int pdu_group_info_free_power_module_group_average(pdu_group_info_t *pdu_group_info)
{
	int ret = -1;

	//充电中的枪数
	uint8_t active_channel_count;
	//要启动的枪数
	uint8_t start_channel_count;
	//所有正常模块组数
	uint8_t all_enable_power_module_group_count;
	//平均每个通道可以用的模块组数
	uint8_t average_power_module_group_per_channel;
	//可用的模块组数
	uint8_t available_power_module_group_count;
	//需要从现在充电的枪中剔除多少个模块组
	uint8_t more_power_module_group_count_need;

	//释放所有即将停止充电的枪关联的模块组
	free_power_module_group_for_stop_channel(pdu_group_info);

	//获取需要充电的枪数
	active_channel_count = list_size(&pdu_group_info->channel_active_list);
	debug("active_channel_count:%d", active_channel_count);

	if(active_channel_count == 0) {//如果没有枪需要充电，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	//获取启动充电的枪数
	start_channel_count = get_start_channel_count(pdu_group_info);
	debug("start_channel_count:%d", start_channel_count);

	if(start_channel_count == 0) {//如果没有要启动的枪，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	//获取所有可用模块组数
	all_enable_power_module_group_count = pdu_group_info->power_module_group_number - get_disable_power_group_count(pdu_group_info);
	debug("all_enable_power_module_group_count:%d", all_enable_power_module_group_count);

	if(all_enable_power_module_group_count == 0) {
		debug("bug!!!");
		return ret;
	}

	//计算平均每个需要充电的枪使用的模块组数
	average_power_module_group_per_channel = all_enable_power_module_group_count / active_channel_count;
	debug("average_power_module_group_per_channel:%d", average_power_module_group_per_channel);

	if(average_power_module_group_per_channel == 0) {//模块组不够分了，停掉最后一个要启动的通道
		cancel_last_start_channel(pdu_group_info);
		return ret;
	}

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(pdu_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	//计算至少需要从正在充电枪中释放多少个模块组
	more_power_module_group_count_need = 0;

	if(start_channel_count * average_power_module_group_per_channel > available_power_module_group_count) {
		more_power_module_group_count_need = start_channel_count * average_power_module_group_per_channel - available_power_module_group_count;
	} else {//不需要再释放模块组，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	debug("more_power_module_group_count_need:%d", more_power_module_group_count_need);

	//从正在充电的枪中释放模块组
	active_channel_list_remove_power_module_group(pdu_group_info, more_power_module_group_count_need, average_power_module_group_per_channel);

	ret = 0;
	return ret;
}

static int pdu_group_info_free_power_module_group_priority(pdu_group_info_t *pdu_group_info)
{
	int ret = -1;

	//可用的模块组数
	uint8_t available_power_module_group_count;
	//需要从现在充电的枪中剔除多少个模块组
	uint8_t more_power_module_group_count_need;

	//释放所有即将停止充电的枪关联的模块组
	free_power_module_group_for_stop_channel(pdu_group_info);

	//释放所有通道多余模块组数
	free_power_module_group_for_active_channel_priority(pdu_group_info);

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(pdu_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	//新启动枪至少一个模块组的前提下,至少需要几个模块组
	more_power_module_group_count_need = get_more_power_group_count_need(pdu_group_info);

	if(more_power_module_group_count_need > available_power_module_group_count) {
		more_power_module_group_count_need -= available_power_module_group_count;
	} else {//不需要再释放模块组，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	debug("more_power_module_group_count_need:%d", more_power_module_group_count_need);

	//从正在充电的枪中释放模块组
	active_channel_list_remove_power_module_group(pdu_group_info, more_power_module_group_count_need, 1);

	ret = 0;
	return ret;
}

static void update_channels_relay_board_config(pdu_group_info_t *pdu_group_info)
{
	channel_info_t *channel_info;
	power_module_group_info_t *power_module_group_info;
	relay_board_item_info_t *relay_board_item_info;
	relay_boards_com_info_t *relay_boards_com_info;
	struct list_head *head;
	struct list_head *head1;
	int i;
	channels_info_t *channels_info = (channels_info_t *)pdu_group_info->channels_info;

	relay_boards_com_info = (relay_boards_com_info_t *)channels_info->relay_boards_com_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info = channels_info->channel_info + i;

		if(channel_info->pdu_group_info != pdu_group_info) {
			debug("skip channel_id %d in pdu_group_id %d", channel_info->channel_id, channel_info->pdu_group_info->pdu_group_id);
			continue;
		}

		debug("channel_id:%d, state:%s, relay_board_number:%d",
		      channel_info->channel_id,
		      get_channel_state_des(channel_info->status.state),
		      channel_info->relay_board_number);

		head = &channel_info->relay_board_item_list;
		list_for_each_entry(relay_board_item_info, head, relay_board_item_info_t, list) {
			relay_board_item_info->config = 0;
		}

		head = &channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			head1 = &channel_info->relay_board_item_list;
			list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
				if(power_module_group_info->group_id < relay_board_item_info->offset) {
					continue;
				}

				if(power_module_group_info->group_id >= relay_board_item_info->offset + relay_board_item_info->number) {
					continue;
				}

				relay_board_item_info->config = set_u8_bits(relay_board_item_info->config,
				                                power_module_group_info->group_id - relay_board_item_info->offset,
				                                1);
				print_relay_board_item_info(2, relay_board_item_info);
			}
		}

		head = &channel_info->relay_board_item_list;
		list_for_each_entry(relay_board_item_info, head, relay_board_item_info_t, list) {
			print_relay_board_item_info(1, relay_board_item_info);

			relay_boards_com_update_config(relay_boards_com_info, relay_board_item_info->board_id, relay_board_item_info->config);
		}
	}
}

static int pdu_group_relay_board_config_sync(pdu_group_info_t *pdu_group_info)
{
	int ret = 0;
	channel_info_t *channel_info;
	relay_board_item_info_t *relay_board_item_info;
	struct list_head *head;
	int i;
	channels_info_t *channels_info = (channels_info_t *)pdu_group_info->channels_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info = channels_info->channel_info + i;

		if(channel_info->pdu_group_info != pdu_group_info) {
			debug("skip channel_id %d in pdu_group_id %d", channel_info->channel_id, channel_info->pdu_group_info->pdu_group_id);
			continue;
		}

		head = &channel_info->relay_board_item_list;
		list_for_each_entry(relay_board_item_info, head, relay_board_item_info_t, list) {
			if(relay_board_item_info->config != relay_board_item_info->remote_config) {
				debug("channel_id %d board_id %d config %02x, remote config %02x",
				      channel_info->channel_id,
				      relay_board_item_info->board_id,
				      relay_board_item_info->config,
				      relay_board_item_info->remote_config);
				ret = -1;
				break;
			}
		}

		if(ret != 0) {
			break;
		}
	}

	return ret;
}

static void channel_info_assign_power_module_group(channel_info_t *channel_info, uint8_t count)
{
	pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	struct list_head *head1;
	//uint32_t ticks = osKernelSysTick();

	if(count == 0) {
		return;
	}

	head = &pdu_group_info->power_module_group_idle_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		relay_board_item_info_t *relay_board_item_info;
		power_module_item_info_t *power_module_item_info;

		head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_IDLE) {
				debug("power module state is not idle:%s!!!", get_power_module_item_state_des(power_module_item_info->status.state));
			}

			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE;
			//power_module_item_info->test_stamp = ticks;
		}
		power_module_group_info->channel_info = channel_info;
		list_move_tail(&power_module_group_info->list, &channel_info->power_module_group_list);
		debug("assign module group_id %d to channel_id %d", power_module_group_info->group_id, channel_info->channel_id);

		head1 = &channel_info->relay_board_item_list;
		list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
			uint8_t group_id = power_module_group_info->group_id;

			if(group_id < relay_board_item_info->offset) {
				continue;
			}

			if(group_id >= relay_board_item_info->offset + relay_board_item_info->number) {
				continue;
			}

			//xiaofei clear power_module_group_info->relay_board_item_info
			power_module_group_info->relay_board_item_info = relay_board_item_info;
		}

		count--;

		if(count == 0) {
			break;
		}
	}

	if(count != 0) {
		debug("bug!!!");
	}
}

static void active_pdu_group_info_power_module_group_assign(pdu_group_info_t *pdu_group_info, uint8_t average_count, uint8_t available_count)
{

	channel_info_t *channel_info;
	struct list_head *head;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		uint8_t power_module_group_count = list_size(&channel_info->power_module_group_list);

		if(power_module_group_count >= average_count) {
			continue;
		}

		power_module_group_count = average_count - power_module_group_count;

		if(power_module_group_count > available_count) {
			power_module_group_count = available_count;
		}

		channel_info_assign_power_module_group(channel_info, power_module_group_count);
		available_count -= power_module_group_count;

		if(available_count == 0) {
			break;
		}
	}

	if(available_count == 0) {
		return;
	}

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		channel_info_assign_power_module_group(channel_info, 1);
		available_count -= 1;

		if(available_count == 0) {
			break;
		}
	}
}

static void active_pdu_group_info_power_module_group_assign_priority(pdu_group_info_t *pdu_group_info, uint8_t available_count)
{
	channel_info_t *channel_info;
	struct list_head *head;
	uint8_t unsatisfied = 0;

	head = &pdu_group_info->channel_active_list;

	debug("available_count:%d", available_count);
	//先保证至少有一个模块组可用
	list_for_each_entry(channel_info, head, channel_info_t, list) {
		uint8_t power_module_group_count = list_size(&channel_info->power_module_group_list);

		if(power_module_group_count >= 1) {//至少有一个模块组可用,本轮不分配
			continue;
		}

		if(available_count == 0) {
			unsatisfied++;
			continue;
		}

		channel_info_assign_power_module_group(channel_info, 1);
		available_count -= 1;
	}

	if(available_count == 0) {
		int i;

		for(i = 0; i < unsatisfied; i++) {
			cancel_last_start_channel(pdu_group_info);
		}

		return;
	}

	debug("available_count:%d", available_count);
	//还有剩余模块组,按先后分配给有需求的通道
	list_for_each_entry(channel_info, head, channel_info_t, list) {
		uint8_t power_module_group_count = list_size(&channel_info->power_module_group_list);

		debug("channel_id %d reassign_module_group_number:%d", channel_info->channel_id, channel_info->status.reassign_module_group_number);

		if(power_module_group_count >= channel_info->status.reassign_module_group_number) {//模块组足够
			continue;
		}

		power_module_group_count = channel_info->status.reassign_module_group_number - power_module_group_count;

		if(power_module_group_count > available_count) {
			power_module_group_count = available_count;
		}

		channel_info_assign_power_module_group(channel_info, power_module_group_count);
		available_count -= power_module_group_count;

		if(available_count == 0) {
			break;
		}
	}

	if(available_count == 0) {
		return;
	}

	debug("available_count:%d", available_count);

	//还有剩余模块组,平均一下,按先后分配给各个通道
	//if(available_count / list_size(&pdu_group_info->channel_active_list) != 0) {
	//	uint8_t power_module_group_count = available_count / list_size(&pdu_group_info->channel_active_list);

	//	list_for_each_entry(channel_info, head, channel_info_t, list) {
	//		channel_info_assign_power_module_group(channel_info, power_module_group_count);
	//		available_count -= power_module_group_count;

	//		if(available_count == 0) {
	//			break;
	//		}
	//	}
	//}

	//if(available_count == 0) {
	//	return;
	//}

	//debug("available_count:%d", available_count);
	//不够平均,按先后分配各通道一个,分完为止
	//list_for_each_entry(channel_info, head, channel_info_t, list) {
	//	channel_info_assign_power_module_group(channel_info, 1);
	//	available_count -= 1;

	//	if(available_count == 0) {
	//		break;
	//	}
	//}
}

static void pdu_group_info_assign_power_module_group_average(pdu_group_info_t *pdu_group_info)
{
	//充电中的枪数
	uint8_t active_channel_count;
	//所有正常模块组数
	uint8_t all_enable_power_module_group_count;
	//平均每个通道可以用的模块组数
	uint8_t average_power_module_group_per_channel;
	//可用的模块组数
	uint8_t available_power_module_group_count;

	//获取需要充电的枪数
	active_channel_count = list_size(&pdu_group_info->channel_active_list);
	debug("active_channel_count:%d", active_channel_count);

	if(active_channel_count == 0) {//如果没有枪需要充电,不分配
		return;
	}

	//获取所有可用模块组数
	all_enable_power_module_group_count = pdu_group_info->power_module_group_number - get_disable_power_group_count(pdu_group_info);
	debug("all_enable_power_module_group_count:%d", all_enable_power_module_group_count);

	if(all_enable_power_module_group_count == 0) {
		debug("bug!!!");
		return;
	}

	//计算平均每个需要充电的枪使用的模块组数
	average_power_module_group_per_channel = all_enable_power_module_group_count / active_channel_count;
	debug("average_power_module_group_per_channel:%d", average_power_module_group_per_channel);

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(pdu_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	active_pdu_group_info_power_module_group_assign(pdu_group_info, average_power_module_group_per_channel, available_power_module_group_count);
}

static void pdu_group_info_assign_power_module_group_priority(pdu_group_info_t *pdu_group_info)
{
	//充电中的枪数
	uint8_t active_channel_count;
	//可用的模块组数
	uint8_t available_power_module_group_count;

	//获取需要充电的枪数
	active_channel_count = list_size(&pdu_group_info->channel_active_list);
	debug("active_channel_count:%d", active_channel_count);

	if(active_channel_count == 0) {//如果没有枪需要充电,不分配
		return;
	}

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(pdu_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	active_pdu_group_info_power_module_group_assign_priority(pdu_group_info, available_power_module_group_count);
}

static int pdu_group_info_power_module_group_ready_sync(pdu_group_info_t *pdu_group_info)
{
	int ret = 0;
	channel_info_t *channel_info;
	struct list_head *head;
	struct list_head *head1;
	struct list_head *head2;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		power_module_group_info_t *power_module_group_info;
		head1 = &channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
			power_module_item_info_t *power_module_item_info;
			head2 = &power_module_group_info->power_module_item_list;
			list_for_each_entry(power_module_item_info, head2, power_module_item_info_t, list) {
				if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_READY) {
					continue;
				}

				if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_ACTIVE) {
					continue;
				}

				ret = -1;
				break;
			}

			if(ret != 0) {
				break;
			}
		}

		if(ret != 0) {
			break;
		}
	}

	return ret;
}

static int channels_module_assign_ready(pdu_group_info_t *pdu_group_info)
{
	int ret = 0;
	channel_info_t *channel_info;
	struct list_head *head;
	channels_info_t *channels_info = (channels_info_t *)pdu_group_info->channels_info;
	channels_com_info_t *channels_com_info = (channels_com_info_t *)channels_info->channels_com_info;

	head = &pdu_group_info->channel_active_list;

	list_for_each_entry(channel_info, head, channel_info_t, list) {
		if(channel_info->status.module_ready_notify == 1) {
			channel_info->status.module_ready_notify = 0;
			debug("channel_id %d module assign ready", channel_info->channel_id);
			channels_com_channel_module_assign_info(channels_com_info, channel_info->channel_id, MODULE_ASSIGN_INFO_READY);
		}
	}
	return ret;
}

char *get_relay_check_state_des(relay_check_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(RELAY_CHECK_STATE_NONE);
			add_des_case(RELAY_CHECK_STATE_START_SYNC);
			add_des_case(RELAY_CHECK_STATE_PREPARE);
			add_des_case(RELAY_CHECK_STATE_PREPARE_CONFIRM);
			add_des_case(RELAY_CHECK_STATE_INSULATION_CHECK);
			add_des_case(RELAY_CHECK_STATE_CHANNEL_MODULE_ID_CHECK);
			add_des_case(RELAY_CHECK_STATE_CHANNEL_CONFIG);
			add_des_case(RELAY_CHECK_STATE_CHANNEL_CONFIG_SYNC);
			add_des_case(RELAY_CHECK_STATE_CHANNEL_CHECK);
			add_des_case(RELAY_CHECK_STATE_STOP);

		default: {
		}
		break;
	}

	return des;
}

typedef int (*pdu_group_info_free_power_module_group_t)(pdu_group_info_t *pdu_group_info);
typedef void (*pdu_group_info_assign_power_module_group_t)(pdu_group_info_t *pdu_group_info);

typedef struct {
	uint8_t policy;
	pdu_group_info_free_power_module_group_t free;
	pdu_group_info_assign_power_module_group_t assign;
} pdu_group_info_power_module_group_policy_t;

static pdu_group_info_power_module_group_policy_t pdu_group_info_power_module_group_policy_average = {
	.policy = POWER_MODULE_POLICY_AVERAGE,
	.free = pdu_group_info_free_power_module_group_average,
	.assign = pdu_group_info_assign_power_module_group_average,
};

static pdu_group_info_power_module_group_policy_t pdu_group_info_power_module_group_policy_priority = {
	.policy = POWER_MODULE_POLICY_PRIORITY,
	.free = pdu_group_info_free_power_module_group_priority,
	.assign = pdu_group_info_assign_power_module_group_priority,
};

static pdu_group_info_power_module_group_policy_t *pdu_group_info_power_module_group_policy_sz[] = {
	&pdu_group_info_power_module_group_policy_average,
	&pdu_group_info_power_module_group_policy_priority,
};

static pdu_group_info_power_module_group_policy_t *get_pdu_group_info_power_module_group_policy(uint8_t policy)
{
	int i;
	pdu_group_info_power_module_group_policy_t *pdu_group_info_power_module_group_policy = &pdu_group_info_power_module_group_policy_average;

	for(i = 0; i < ARRAY_SIZE(pdu_group_info_power_module_group_policy_sz); i++) {
		pdu_group_info_power_module_group_policy_t *pdu_group_info_power_module_group_policy_item = pdu_group_info_power_module_group_policy_sz[i];

		if(pdu_group_info_power_module_group_policy_item->policy == policy) {
			pdu_group_info_power_module_group_policy = pdu_group_info_power_module_group_policy_item;
			break;
		}
	}

	return pdu_group_info_power_module_group_policy;
}

//根据通道状态变化，重新分配模块组
static void handle_channels_change_state(pdu_group_info_t *pdu_group_info)
{
	switch(pdu_group_info->channels_change_state) {
		case CHANNELS_CHANGE_STATE_IDLE: {
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE: {//准备释放模块组
			channels_info_t *channels_info = (channels_info_t *)pdu_group_info->channels_info;
			pdu_config_t *pdu_config = &channels_info->channels_settings.pdu_config;
			pdu_group_info_power_module_group_policy_t *policy = get_pdu_group_info_power_module_group_policy(pdu_config->policy);

			OS_ASSERT(policy != NULL);
			debug("pdu_group_id %d get policy %s", pdu_group_info->pdu_group_id, get_power_module_policy_des(pdu_config->policy));

			if(policy->free(pdu_group_info) == 0) {//返回0表示释放操作完成
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_FREE;
				debug("pdu_group_id %d channels_change_state to state %s",
				      pdu_group_info->pdu_group_id,
				      get_channels_change_state_des(pdu_group_info->channels_change_state));
			}
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_FREE: {//释放模块组-停用模块列表为空
			if(list_empty(&pdu_group_info->power_module_group_deactive_list)) {//所有该停用的模块组停用完毕
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG;
				debug("pdu_group_id %d channels_change_state to state %s",
				      pdu_group_info->pdu_group_id,
				      get_channels_change_state_des(pdu_group_info->channels_change_state));
			}
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG: {//释放模块组后下发配置
			update_channels_relay_board_config(pdu_group_info);

			pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC;
			debug("pdu_group_id %d channels_change_state to state %s",
			      pdu_group_info->pdu_group_id,
			      get_channels_change_state_des(pdu_group_info->channels_change_state));
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC: {//释放模块组后配置同步
			if(pdu_group_relay_board_config_sync(pdu_group_info) == 0) {
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_ASSIGN;
				debug("pdu_group_id %d channels_change_state to state %s",
				      pdu_group_info->pdu_group_id,
				      get_channels_change_state_des(pdu_group_info->channels_change_state));
			}
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_ASSIGN: {//分配模块组
			channels_info_t *channels_info = (channels_info_t *)pdu_group_info->channels_info;
			pdu_config_t *pdu_config = &channels_info->channels_settings.pdu_config;
			pdu_group_info_power_module_group_policy_t *policy = get_pdu_group_info_power_module_group_policy(pdu_config->policy);

			policy->assign(pdu_group_info);
			pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_READY_SYNC;
			debug("pdu_group_id %d channels_change_state to state %s",
			      pdu_group_info->pdu_group_id,
			      get_channels_change_state_des(pdu_group_info->channels_change_state));
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_READY_SYNC: {//根据情况等待模块组预充
			if(pdu_group_info_power_module_group_ready_sync(pdu_group_info) == 0) {
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG;
				debug("pdu_group_id %d channels_change_state to state %s",
				      pdu_group_info->pdu_group_id,
				      get_channels_change_state_des(pdu_group_info->channels_change_state));
			}
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG: {//分配模块组后下发配置
			update_channels_relay_board_config(pdu_group_info);

			pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC;
			debug("pdu_group_id %d channels_change_state to state %s",
			      pdu_group_info->pdu_group_id,
			      get_channels_change_state_des(pdu_group_info->channels_change_state));
		}
		break;

		case CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC: {//分配模块组后同步配置
			if(pdu_group_relay_board_config_sync(pdu_group_info) == 0) {
				channels_module_assign_ready(pdu_group_info);
				pdu_group_info->channels_change_state = CHANNELS_CHANGE_STATE_IDLE;
				debug("pdu_group_id %d channels_change_state to state %s",
				      pdu_group_info->pdu_group_id,
				      get_channels_change_state_des(pdu_group_info->channels_change_state));
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_pdu_group_deactive_list(pdu_group_info_t *pdu_group_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;

	head = &pdu_group_info->power_module_group_deactive_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		power_module_item_info_t *power_module_item_info;
		uint8_t power_module_group_idle = 1;
		uint8_t power_module_group_disable = 0;

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_IDLE) {
				power_module_group_idle = 0;
				break;
			}
		}

		if(power_module_group_idle == 1) {
			power_module_group_info->channel_info = NULL;
			power_module_group_info->relay_board_item_info = NULL;
			debug("set power module group_id %d idle!", power_module_group_info->group_id);
			list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_idle_list);
			continue;
		}

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_DISABLE) {
				power_module_group_disable = 1;
				break;
			}
		}

		if(power_module_group_disable == 1) {
			power_module_group_info->channel_info = NULL;
			power_module_group_info->relay_board_item_info = NULL;
			debug("set power module group_id %d disable", power_module_group_info->group_id);
			list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_disable_list);
			continue;
		}
	}
}

static void handle_pdu_group_disable_list(pdu_group_info_t *pdu_group_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;

	head = &pdu_group_info->power_module_group_disable_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		uint8_t power_module_group_disable = 0;

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_IDLE) {
				power_module_group_disable = 1;
			}
		}

		if(power_module_group_disable == 0) {
			debug("reenable power module group_id %d", power_module_group_info->group_id);
			list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_idle_list);
		}
	}
}

static uint8_t get_channel_active_power_module_item_count(channel_info_t *channel_info)
{
	uint8_t module_count = 0;
	struct list_head *head;
	power_module_group_info_t *power_module_group_info;

	head = &channel_info->power_module_group_list;
	list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
		struct list_head *head = &power_module_group_info->power_module_item_list;
		power_module_item_info_t *power_module_item_info;
		list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
			if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_DISABLE) {
				continue;
			}

			module_count++;
		}
	}

	return module_count;
}

static void handle_channels_max_power_limit(channels_info_t *channels_info)
{
	int i;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	uint32_t channels_max_output_power_left = channels_settings->channels_max_output_power * 100;

	for(i = 0; i < channels_info->pdu_group_number; i++) {
		pdu_group_info_t *pdu_group_info = channels_info->pdu_group_info + i;
		struct list_head *head;
		channel_info_t *channel_info;

		head = &pdu_group_info->channel_active_list;
		list_for_each_entry(channel_info, head, channel_info_t, list) {
			uint16_t charge_output_voltage = channel_info->status.charge_output_voltage;
			uint16_t charge_output_current = channel_info->status.charge_output_current;
			uint32_t charge_output_power = charge_output_voltage * charge_output_current;
			uint16_t require_output_voltage = channel_info->status.require_output_voltage;
			uint16_t require_output_current = channel_info->status.require_output_current;
			uint32_t require_output_power;
			uint8_t module_count = get_channel_active_power_module_item_count(channel_info);
			uint16_t module_require_current = (module_count != 0) ? (require_output_current / module_count) : 0;
			uint32_t channel_max_output_power;

			module_voltage_current_correction(channels_settings, &require_output_voltage, &module_require_current);
			module_power_limit_correction(channels_settings, &charge_output_voltage, &module_require_current);
			require_output_power = require_output_voltage * (module_require_current * module_count);

			//debug("channel %d channels_max_output_power_left:%d",
			//      channel_info->channel_id,
			//      channels_max_output_power_left);

			//debug("channel %d module_require_current:%d, module_count:%d",
			//      channel_info->channel_id,
			//      module_require_current,
			//      module_count);

			//debug("channel %d charge_output_voltage:%d, charge_output_current:%d",
			//      channel_info->channel_id,
			//      charge_output_voltage,
			//      charge_output_current);

			//debug("channel %d require_output_power:%d, charge_output_power:%d",
			//      channel_info->channel_id,
			//      require_output_power,
			//      charge_output_power);

			if(require_output_power > charge_output_power) {
				channel_max_output_power = require_output_power;
			} else {
				channel_max_output_power = charge_output_power;
			}

			if(channels_max_output_power_left >= channel_max_output_power) {
				channel_info->channel_max_output_power = channel_max_output_power / 100;
				channels_max_output_power_left -= channel_max_output_power;
			} else {//已经超过配额
				channel_info->channel_max_output_power = channels_max_output_power_left / 100;
				channels_max_output_power_left -= channels_max_output_power_left;
			}

			//debug("channel %d channel_max_output_power:%d",
			//      channel_info->channel_id,
			//      channel_info->channel_max_output_power);
		}
	}
}

char *get_power_module_item_state_des(power_module_item_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(POWER_MODULE_ITEM_STATE_IDLE);
			add_des_case(POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_READY);
			add_des_case(POWER_MODULE_ITEM_STATE_ACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_ACTIVE_RELAY_CHECK);
			add_des_case(POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_DEACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_DISABLE);

		default: {
		}
		break;
	}

	return des;
}

static void update_poewr_module_item_info_status(power_module_item_info_t *power_module_item_info)
{
	power_modules_info_t *power_modules_info = (power_modules_info_t *)power_module_item_info->power_modules_info;
	power_module_info_t *power_module_info = power_modules_info->power_module_info + power_module_item_info->module_id;
	power_module_item_status_t *status = &power_module_item_info->status;
	u_power_module_status_t u_power_module_status;
	uint32_t ticks = osKernelSysTick();

	pdu_group_info_t *pdu_group_info;
	channels_info_t *channels_info;
	channels_com_info_t *channels_com_info;
	module_status_t *module_status;
	uint8_t connect_timeout;
	power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
	channel_info_t *channel_info = (channel_info_t *)power_module_group_info->channel_info;
	uint8_t fault_changed = 0;
	uint8_t over_voltage = 0;
	uint8_t low_voltage = 0;

	if(ticks_duration(ticks, power_module_item_info->query_stamp) < (1 * 1000)) {
		return;
	}

	power_module_item_info->query_stamp = ticks;

	power_modules_query_status(power_modules_info, power_module_item_info->module_id);
	power_modules_query_a_line_input_voltage(power_modules_info, power_module_item_info->module_id);
	power_modules_query_b_line_input_voltage(power_modules_info, power_module_item_info->module_id);
	power_modules_query_c_line_input_voltage(power_modules_info, power_module_item_info->module_id);

	status->module_output_voltage = power_module_info->output_voltage;

	status->module_output_current = power_module_info->output_current;

	u_power_module_status.s = power_module_info->power_module_status;

	pdu_group_info = power_module_group_info->pdu_group_info;
	channels_info = (channels_info_t *)pdu_group_info->channels_info;

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_FAULT) != u_power_module_status.s.fault) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_FAULT, u_power_module_status.s.fault);

		if(u_power_module_status.s.fault != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if(ticks_duration(ticks, get_power_module_connect_stamp(power_modules_info, power_module_item_info->module_id)) >= (10 * 1000)) {
		if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_DISABLE) {
			debug("ticks:%d, power module_id %d stamps:%d", ticks, power_module_item_info->module_id, get_power_module_connect_stamp(power_modules_info, power_module_item_info->module_id));
		}

		connect_timeout = 1;
	} else {
		connect_timeout = 0;
	}

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_CONNECT_TIMEOUT, connect_timeout);

		if(connect_timeout != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if((power_module_info->input_aline_voltage < 1900) || (power_module_info->input_bline_voltage < 1900) || (power_module_info->input_cline_voltage < 1900)) {
		low_voltage = 1;
	} else if((power_module_info->input_aline_voltage > 2500) || (power_module_info->input_bline_voltage > 2500) || (power_module_info->input_cline_voltage > 2500)) {
		over_voltage = 1;

		if((power_module_info->input_aline_voltage > 2750) || (power_module_info->input_bline_voltage > 2750) || (power_module_info->input_cline_voltage > 2750)) {
			power_module_info->over_voltage_disable = 1;
		}
	}

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_LOW_VOLTAGE) != low_voltage) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_LOW_VOLTAGE, low_voltage);

		if(low_voltage != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_OVER_VOLTAGE) != over_voltage) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_OVER_VOLTAGE, over_voltage);

		if(over_voltage != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if(get_first_fault(power_module_item_info->faults) != -1) {
		uint8_t power_module_group_disable = 1;

		if(fault_changed == 0) {
			power_module_group_disable = 0;
		}

		if(list_contain(&power_module_group_info->list, &pdu_group_info->power_module_group_deactive_list) == 0) {
			power_module_group_disable = 0;
		} else if(list_contain(&power_module_group_info->list, &pdu_group_info->power_module_group_disable_list) == 0) {
			power_module_group_disable = 0;
		}

		if(power_module_group_disable == 1) {
			power_module_item_info_t *power_module_item_info_deactive;
			struct list_head *head = &power_module_group_info->power_module_item_list;

			if(channel_info != NULL) {
				if(channel_info->status.state != CHANNEL_STATE_IDLE) {
					debug("channel_id %d stop by module_id %d fault",
					      channel_info->channel_id, power_module_item_info->module_id);
					try_to_stop_channel(channel_info);
				}
			}

			list_for_each_entry(power_module_item_info_deactive, head, power_module_item_info_t, list) {
				power_module_item_info_deactive->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;

				debug("module_id %d to state %s",
				      power_module_item_info_deactive->module_id,
				      get_power_module_item_state_des(power_module_item_info_deactive->status.state));
			}

			list_move_tail(&power_module_group_info->list, &pdu_group_info->power_module_group_deactive_list);
		}
	}

	status->module_status = u_power_module_status.v;

	status->connect_state = get_power_module_connect_state(power_modules_info, power_module_item_info->module_id);

	channels_com_info = (channels_com_info_t *)channels_info->channels_com_info;
	module_status = (module_status_t *)channels_com_info->module_status;
	module_status += power_module_item_info->module_id;

	//debug("module_id:%d, output_voltage:%d, output_current:%d",
	//      power_module_item_info->module_id,
	//      power_module_info->output_voltage,
	//      power_module_info->output_current);

	module_status->setting_voltage_l = get_u8_l_from_u16(power_module_info->setting_voltage / 100);
	module_status->setting_voltage_h = get_u8_h_from_u16(power_module_info->setting_voltage / 100);
	module_status->setting_current_l = get_u8_l_from_u16(power_module_info->setting_current / 100);
	module_status->setting_current_h = get_u8_h_from_u16(power_module_info->setting_current / 100);
	module_status->output_voltage_l = get_u8_l_from_u16(power_module_info->output_voltage);
	module_status->output_voltage_h = get_u8_h_from_u16(power_module_info->output_voltage);
	module_status->output_current_l = get_u8_l_from_u16(power_module_info->output_current);
	module_status->output_current_h = get_u8_h_from_u16(power_module_info->output_current);
	module_status->module_state_l = get_u8_l_from_u16(status->module_status);
	module_status->module_state_h = get_u8_h_from_u16(status->module_status);
	module_status->connect_state_l = get_u8_l_from_u16(status->connect_state);
	module_status->connect_state_h = get_u8_h_from_u16(status->connect_state);
}

static void power_module_item_set_out_voltage_current(power_module_item_info_t *power_module_item_info, uint16_t voltage, uint16_t current)
{
	uint32_t ticks = osKernelSysTick();
	power_modules_info_t *power_modules_info = (power_modules_info_t *)power_module_item_info->power_modules_info;
	power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
	channel_info_t *channel_info = (channel_info_t *)power_module_group_info->channel_info;
	relay_board_item_info_t *relay_board_item_info = (relay_board_item_info_t *)power_module_group_info->relay_board_item_info;
	u_power_module_status_t u_power_module_status;
	uint8_t power_off = 0;

	if(channel_info != NULL) {
		if(get_first_fault(channel_info->faults) != -1) {
			voltage = 0;
			current = 0;
		}
	}

	if(relay_board_item_info != NULL) {
		if(get_first_fault(relay_board_item_info->faults) != -1) {
			voltage = 0;
			current = 0;
		}
	}

	if(get_first_fault(power_module_item_info->faults) != -1) {
		voltage = 0;
		current = 0;
	}

	if((power_module_item_info->status.setting_output_voltage == voltage) &&
	   (power_module_item_info->status.setting_output_current == current)) {
		if(ticks_duration(ticks, power_module_item_info->setting_stamp) >= (1 * 1000)) {
		} else {
			return;
		}
	} else {
		power_module_item_info->status.setting_output_voltage = voltage;
		power_module_item_info->status.setting_output_current = current;

		power_module_item_info->status.smooth_setting_output_current = power_module_item_info->status.setting_output_current;
	}

	power_module_item_info->setting_stamp = ticks;

	if(current > power_module_item_info->status.smooth_setting_output_current) {//需求值比设置值大
		if(current - power_module_item_info->status.smooth_setting_output_current > 30 * 10) {//需求值比设置值大30a
			power_module_item_info->status.smooth_setting_output_current += 30 * 10;
			current = power_module_item_info->status.smooth_setting_output_current;
		} else {
			power_module_item_info->status.smooth_setting_output_current = current;
		}
	} else if(current < power_module_item_info->status.smooth_setting_output_current) {//只平滑增大电流，不平滑减小电流
		power_module_item_info->status.smooth_setting_output_current = current;
	}

	u_power_module_status.v = power_module_item_info->status.module_status;

	if(voltage == 0) {
		power_off = 1;
	}

	if(current == 0) {
		power_off = 1;
	}

	//debug("set module_id:%d, poweroff:%d, voltage:%d, current:%d",
	//      power_module_item_info->module_id,
	//      power_off,
	//      voltage,
	//      current);

	if(u_power_module_status.s.poweroff != power_off) {
		if(power_off == 0) {
			if(channel_info != NULL) {
				channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
				channels_settings_t *channels_settings = &channels_info->channels_settings;
				uint32_t battery_voltage = channels_settings->module_min_output_voltage * 100;

				if(battery_voltage == 0) {
					battery_voltage = 200000;
				}

				power_modules_set_channel_id(power_modules_info, power_module_item_info->module_id, channel_info->channel_id);
				power_modules_set_battery_voltage(power_modules_info, power_module_item_info->module_id, battery_voltage);
			}
		}

		power_modules_set_poweroff(power_modules_info, power_module_item_info->module_id, power_off);
	}

	if(power_off == 0) {
		power_modules_set_out_voltage_current(power_modules_info, power_module_item_info->module_id, voltage * 1000 / 10, current * 1000 / 10);
	} else {
		power_modules_set_out_voltage_current(power_modules_info, power_module_item_info->module_id, 0, 0);
	}
}

static void handle_power_module_item_info_state(power_module_item_info_t *power_module_item_info)
{
	switch(power_module_item_info->status.state) {
		case POWER_MODULE_ITEM_STATE_IDLE: {
			power_module_item_set_out_voltage_current(power_module_item_info, 0, 0);
		}
		break;

		case POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE: {//准备启动模块组，根据情况预充
			//uint32_t ticks = osKernelSysTick();
			power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
			channel_info_t *channel_info = (channel_info_t *)power_module_group_info->channel_info;
			channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
			channels_settings_t *channels_settings = &channels_info->channels_settings;

			if((channel_info->status.require_work_state == CHANNEL_WORK_STATE_PREPARE_START) ||
			   (channel_info->status.require_work_state == CHANNEL_WORK_STATE_START) ||
			   (channel_info->status.require_work_state == CHANNEL_WORK_STATE_CHARGE)) {
				uint16_t voltage = channel_info->status.charge_output_voltage;
				uint16_t current = 1;

				module_voltage_current_correction(channels_settings, &voltage, &current);
				//module_power_limit_correction(channels_settings, &channel_info->status.charge_output_voltage, &current);
				power_module_item_set_out_voltage_current(power_module_item_info, voltage, current);

				//debug("module_id %d setting voltage:%d, current:%d",
				//      power_module_item_info->module_id,
				//      voltage,
				//      current);

				if(abs(power_module_item_info->status.setting_output_voltage - power_module_item_info->status.module_output_voltage) <= 300) {
					//if(ticks_duration(ticks, power_module_item_info->test_stamp) >= (power_module_item_info->module_id * 1000 / 4)) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_READY;
					debug("module_id %d to state %s",
					      power_module_item_info->module_id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
					//}
				} else {
					//debug("module_id %d setting_output_voltage:%d, module_output_voltage:%d",
					//      power_module_item_info->module_id,
					//      power_module_item_info->status.setting_output_voltage,
					//      power_module_item_info->status.module_output_voltage);
				}
			} else {
				if(power_module_item_info->status.module_output_voltage >= 500) {
					//if(ticks_duration(ticks, power_module_item_info->test_stamp) >= (power_module_item_info->module_id * 1000 / 4)) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_READY;
					debug("module_id %d to state %s",
					      power_module_item_info->module_id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
					//}
				} else {
					//debug("module_id %d setting_output_voltage:%d, module_output_voltage:%d",
					//      power_module_item_info->module_id,
					//      power_module_item_info->status.setting_output_voltage,
					//      power_module_item_info->status.module_output_voltage);
				}
			}

		}
		break;

		case POWER_MODULE_ITEM_STATE_READY: {//达到接通条件
			power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
			pdu_group_info_t *pdu_group_info = power_module_group_info->pdu_group_info;

			if(pdu_group_info->channels_change_state == CHANNELS_CHANGE_STATE_IDLE) {//等待配置同步完成,开关动作完成
				power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_ACTIVE;
				debug("module_id %d to state %s",
				      power_module_item_info->module_id,
				      get_power_module_item_state_des(power_module_item_info->status.state));
			}
		}
		break;

		case POWER_MODULE_ITEM_STATE_ACTIVE: {
			power_module_item_info->status.setting_output_voltage = power_module_item_info->status.require_output_voltage;
			power_module_item_info->status.setting_output_current = power_module_item_info->status.require_output_current;

			power_module_item_set_out_voltage_current(power_module_item_info,
			        power_module_item_info->status.require_output_voltage,
			        power_module_item_info->status.require_output_current);
			//debug("module_id %d require_output_voltage:%d, require_output_current:%d",
			//      power_module_item_info->module_id,
			//      power_module_item_info->status.require_output_voltage,
			//      power_module_item_info->status.require_output_current);
		}
		break;

		case POWER_MODULE_ITEM_STATE_ACTIVE_RELAY_CHECK: {
			power_module_item_set_out_voltage_current(power_module_item_info,
			        power_module_item_info->status.require_relay_check_voltage,
			        power_module_item_info->status.require_relay_check_current);
		}
		break;

		case POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE: {//准备停用模块组，进行停机操作
			u_power_module_status_t u_power_module_status;
			uint8_t module_disable = 0;

			power_module_item_set_out_voltage_current(power_module_item_info, 0, 0);

			u_power_module_status.v = power_module_item_info->status.module_status;

			if(get_first_fault(power_module_item_info->faults) != -1) {
				module_disable = 1;
			}

			if(module_disable == 1) {
				power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_DISABLE;
				debug("module_id %d to state %s",
				      power_module_item_info->module_id,
				      get_power_module_item_state_des(power_module_item_info->status.state));
			} else {
				if(u_power_module_status.s.poweroff == 1) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_DEACTIVE;
					debug("module_id %d to state %s",
					      power_module_item_info->module_id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
				}
			}
		}
		break;

		case POWER_MODULE_ITEM_STATE_DEACTIVE: {//停机完成
			power_module_item_info->status.setting_output_voltage = 0;
			power_module_item_info->status.setting_output_current = 0;

			power_module_item_set_out_voltage_current(power_module_item_info, 0, 0);

			if(power_module_item_info->status.module_output_voltage <= 500) {
				power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_IDLE;
				debug("module_id %d to state %s",
				      power_module_item_info->module_id,
				      get_power_module_item_state_des(power_module_item_info->status.state));
			}
		}
		break;

		case POWER_MODULE_ITEM_STATE_DISABLE: {//禁用模块组
			u_power_module_status_t u_power_module_status;

			power_module_item_info->status.setting_output_voltage = 0;
			power_module_item_info->status.setting_output_current = 0;

			power_module_item_set_out_voltage_current(power_module_item_info, 0, 0);

			if(get_first_fault(power_module_item_info->faults) == -1) {
				u_power_module_status.v = power_module_item_info->status.module_status;

				if(u_power_module_status.s.poweroff == 1) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_IDLE;
					debug("module_id %d to state %s",
					      power_module_item_info->module_id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_power_module_items_info_state(channels_info_t *channels_info)
{
	int i;

	for(i = 0; i < channels_info->power_module_item_number; i++) {
		power_module_item_info_t *power_module_item_info = &channels_info->power_module_item_info[i];

		update_poewr_module_item_info_status(power_module_item_info);
		handle_power_module_item_info_state(power_module_item_info);
	}
}

static uint8_t is_power_module_item_info_idle(power_module_item_info_t *power_module_item_info)
{
	uint8_t idle = -1;

	switch(power_module_item_info->status.state) {
		case POWER_MODULE_ITEM_STATE_IDLE:
		case POWER_MODULE_ITEM_STATE_DISABLE: {
			idle = 0;
		}
		break;

		default: {
		}
		break;
	}

	return idle;
}

static void handle_fan_state(channels_info_t *channels_info)
{
	int i;
	GPIO_PinState state = GPIO_PIN_RESET;
	channels_config_t *channels_config = channels_info->channels_config;

	for(i = 0; i < channels_info->power_module_item_number; i++) {
		power_module_item_info_t *power_module_item_info = &channels_info->power_module_item_info[i];

		if(is_power_module_item_info_idle(power_module_item_info) != 0) {
			state = GPIO_PIN_SET;
			break;
		}
	}

	HAL_GPIO_WritePin(channels_config->gpio_port_fan, channels_config->gpio_pin_fan, state);
}

static void update_relay_board_item_info_state(relay_board_item_info_t *relay_board_item_info)
{
	uint32_t ticks = osKernelSysTick();
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)relay_board_item_info->relay_boards_com_info;

	if(ticks_duration(ticks, relay_board_item_info->update_stamp) < (1 * 1000)) {
		return;
	}

	relay_board_item_info->update_stamp = ticks;
	relay_board_item_info->connect_state = relay_boards_com_get_connect_state(relay_boards_com_info, relay_board_item_info->board_id);
}

static void update_relay_boards_info_state(channels_info_t *channels_info)
{
	int i;

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info_t *relay_board_item_info = &channels_info->relay_board_item_info[i];

		update_relay_board_item_info_state(relay_board_item_info);
	}
}

static uint8_t update_power_module_policy_request = 0;
static uint8_t power_module_policy_value = POWER_MODULE_POLICY_AVERAGE;

void set_power_module_policy_request(power_module_policy_t policy)
{
	update_power_module_policy_request = 1;
	power_module_policy_value = policy;
}

static int is_channels_idle(channels_info_t *channels_info)
{
	int ret = 0;
	int i;
	channel_info_t *channel_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info = channels_info->channel_info + i;

		if(channel_info->status.state != CHANNEL_STATE_IDLE) {
			ret = -1;
			break;
		}
	}

	return ret;
}

static void handle_power_module_policy_request(channels_info_t *channels_info)
{
	if(update_power_module_policy_request == 1) {
		if(is_channels_idle(channels_info) == 0) {
			pdu_config_t *pdu_config = &channels_info->channels_settings.pdu_config;

			update_power_module_policy_request = 0;
			pdu_config->policy = power_module_policy_value;
			channels_info_save_config(channels_info);

			debug("set policy %s", get_power_module_policy_des(pdu_config->policy));
		}
	}
}

static void handle_power_module_type_config(channels_info_t *channels_info)
{
	power_modules_info_t *power_modules_info = (power_modules_info_t *)channels_info->power_modules_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if((power_modules_handler == NULL) || (channels_info->channels_settings.power_module_settings.power_module_type != power_modules_handler->power_module_type)) {
		power_modules_set_type(power_modules_info, channels_settings->power_module_settings.power_module_type);
	}
}

static int relay_check_start_sync(channels_info_t *channels_info)
{
	int ret = 0;
	int i;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(channel_info->status.state != CHANNEL_STATE_RELAY_CHECK) {
			ret = -1;
			break;
		}
	}

	return ret;
}

static int relay_check_prepare(channels_info_t *channels_info)
{
	int ret = 0;
	int i;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	uint16_t voltage = channels_settings->module_min_output_voltage;
	uint16_t current = channels_settings->module_min_output_current;
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)channels_info->relay_boards_com_info;

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + i;
		relay_board_item_info->config = 0x00;
		relay_boards_com_update_config(relay_boards_com_info, relay_board_item_info->board_id, relay_board_item_info->config);
	}

	for(i = 0; i < channels_info->power_module_group_number; i++) {
		power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + i;
		struct list_head *head = &power_module_group_info->power_module_item_list;
		power_module_item_info_t *power_module_item_info;
		list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
			power_module_item_info->status.require_relay_check_voltage = voltage;
			power_module_item_info->status.require_relay_check_current = current;
			module_voltage_current_correction(channels_settings, &power_module_item_info->status.require_relay_check_voltage, &power_module_item_info->status.require_relay_check_current);
			//module_power_limit_correction(channels_settings, &power_module_item_info->status.require_relay_check_voltage, &power_module_item_info->status.require_relay_check_current);

			if(power_module_item_info->status.require_relay_check_voltage == channels_settings->module_max_output_voltage) {
				debug("module_id %d relay check voltage %d == module max voltage %d",
				      power_module_item_info->module_id,
				      power_module_item_info->status.require_relay_check_voltage,
				      channels_settings->module_max_output_voltage);
				ret = -1;
				break;
			}

			debug("module_id %d voltage %d, current %d",
			      power_module_item_info->module_id,
			      power_module_item_info->status.require_relay_check_voltage,
			      power_module_item_info->status.require_relay_check_current);

			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_ACTIVE_RELAY_CHECK;

		}

		if(ret != 0) {
			break;
		}

		voltage += 200;
	}

	return ret;
}

static int relay_check_confirm(channels_info_t *channels_info)
{
	int ret = 0;
	int i;

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + i;

		if(relay_board_item_info->config != relay_board_item_info->remote_config) {
			debug("board_id %d config %02x, remote config %02x",
			      relay_board_item_info->board_id,
			      relay_board_item_info->config,
			      relay_board_item_info->remote_config);
			ret = -1;
			break;
		}
	}

	if(ret != 0) {
		return ret;
	}

	for(i = 0; i < channels_info->power_module_group_number; i++) {
		power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + i;
		struct list_head *head = &power_module_group_info->power_module_item_list;
		power_module_item_info_t *power_module_item_info;
		list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
			if(abs(power_module_item_info->status.require_relay_check_voltage - power_module_item_info->status.module_output_voltage) > 50) {
				debug("module_id %d require_relay_check_voltage %d, module_output_voltage %d",
				      power_module_item_info->module_id,
				      power_module_item_info->status.require_relay_check_voltage,
				      power_module_item_info->status.module_output_voltage);
				ret = -1;
				break;
			}
		}

		if(ret != 0) {
			break;
		}
	}

	return ret;
}

static int relay_check_insulation_check(channels_info_t *channels_info)
{
	int ret = 0;
	int i;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(channel_info->status.charge_output_voltage > 50) {
			ret = -1;
			break;
		}
	}

	return ret;
}

static int relay_check_channel_module_group_id_check(channels_info_t *channels_info)
{
	int ret = -1;
	relay_check_info_t *relay_check_info = &channels_info->relay_check_info;
	uint8_t channel_id = relay_check_info->relay_check_channel_id;
	uint8_t group_id = relay_check_info->relay_check_power_module_group_id;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + group_id;

	if(channel_id >= channels_info->channel_number) {
		debug("channel_id >= channels_info->channel_number");
		return ret;
	}

	if(group_id >= channels_info->power_module_group_number) {
		debug("group_id >= channels_info->power_module_group_number");
		return ret;
	}

	if(channel_info->pdu_group_info == power_module_group_info->pdu_group_info) {
		ret = 0;
		debug("channel_id %d group_id %d start relay check", channel_id, group_id);
	}

	return ret;
}

static int relay_check_channel_module_id_next(channels_info_t *channels_info)
{
	int ret = -1;
	relay_check_info_t *relay_check_info = &channels_info->relay_check_info;
	uint8_t channel_id = relay_check_info->relay_check_channel_id;
	uint8_t group_id = relay_check_info->relay_check_power_module_group_id;

	if(channel_id >= channels_info->channel_number) {
		debug("channel_id >= channels_info->channel_number");
		return ret;
	}

	if(group_id >= channels_info->power_module_group_number) {
		debug("group_id >= channels_info->power_module_group_number");
		return ret;
	}

	group_id++;

	if(group_id >= channels_info->power_module_group_number) {
		group_id = 0;
		channel_id++;

		if(channel_id >= channels_info->channel_number) {
		} else {
			relay_check_info->relay_check_channel_id = channel_id;
			relay_check_info->relay_check_power_module_group_id = group_id;
			ret = 0;
		}
	} else {
		relay_check_info->relay_check_power_module_group_id = group_id;
		ret = 0;
	}

	return ret;
}

static int relay_check_channel_config(channels_info_t *channels_info)
{
	int ret = 0;
	relay_check_info_t *relay_check_info = &channels_info->relay_check_info;
	uint8_t channel_id = relay_check_info->relay_check_channel_id;
	uint8_t group_id = relay_check_info->relay_check_power_module_group_id;
	uint8_t board_id = 0xff;
	uint8_t config = 0x00;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	relay_board_item_info_t *relay_board_item_info;
	struct list_head *head;
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)channels_info->relay_boards_com_info;

	if(channel_id >= channels_info->channel_number) {
		debug("channel_id >= channels_info->channel_number");
		ret = -1;
		return ret;
	}

	if(group_id >= channels_info->power_module_group_number) {
		debug("group_id >= channels_info->power_module_group_number");
		ret = -1;
		return ret;
	}

	head = &channel_info->relay_board_item_list;
	list_for_each_entry(relay_board_item_info, head, relay_board_item_info_t, list) {
		if(group_id < relay_board_item_info->offset) {
			continue;
		}

		if(group_id >= relay_board_item_info->offset + relay_board_item_info->number) {
			continue;
		}

		board_id = relay_board_item_info->board_id;
		config = set_u8_bits(0x00, group_id - relay_board_item_info->offset, 1);

		relay_board_item_info->config = config;
		relay_check_info->relay_check_board_id = board_id;

		relay_boards_com_update_config(relay_boards_com_info, relay_board_item_info->board_id, relay_board_item_info->config);
	}

	if(board_id == 0xff) {
		debug("config channel_id %d group_id %d error!", channel_id, group_id);
		ret = -1;
		return ret;
	}

	return ret;
}

static int relay_check_channel_check(channels_info_t *channels_info)
{
	int ret = 0;
	relay_check_info_t *relay_check_info = &channels_info->relay_check_info;
	uint8_t channel_id = relay_check_info->relay_check_channel_id;
	uint8_t group_id = relay_check_info->relay_check_power_module_group_id;
	uint8_t board_id = relay_check_info->relay_check_board_id;
	power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + group_id;
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)channels_info->relay_boards_com_info;

	int i;

	if(channel_id >= channels_info->channel_number) {
		debug("channel_id >= channels_info->channel_number");
		ret = -1;
		return ret;
	}

	if(group_id >= channels_info->power_module_group_number) {
		debug("group_id >= channels_info->power_module_group_number");
		ret = -1;
		return ret;
	}

	if(board_id >= channels_info->relay_board_item_number) {
		debug("board_id >= channels_info->relay_board_item_number");
		ret = -1;
		return ret;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(channel_id == channel_info->channel_id) {
			struct list_head *head = &power_module_group_info->power_module_item_list;
			power_module_item_info_t *power_module_item_info;
			list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
				if(abs(channel_info->status.charge_output_voltage - power_module_item_info->status.require_relay_check_voltage) > 50) {
					debug("channel_id %d charge_output_voltage:%d, require_relay_check_voltage:%d",
					      channel_info->channel_id,
					      channel_info->status.charge_output_voltage,
					      power_module_item_info->status.require_relay_check_voltage);
					ret = -1;
					break;
				}
			}

			if(ret != 0) {
				break;
			}
		} else {
			if(channel_info->status.charge_output_voltage > 50) {
				debug("channel_id %d charge_output_voltage:%d",
				      channel_info->channel_id,
				      channel_info->status.charge_output_voltage);
				ret = -1;
				break;
			}
		}
	}

	if(ret == 0) {
		relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + board_id;

		relay_board_item_info->config = 0x00;
		relay_boards_com_update_config(relay_boards_com_info, relay_board_item_info->board_id, relay_board_item_info->config);
	}

	return ret;
}

static int relay_check_stop(channels_info_t *channels_info)
{
	int ret = 0;
	int i;
	relay_boards_com_info_t *relay_boards_com_info = (relay_boards_com_info_t *)channels_info->relay_boards_com_info;

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info_t *relay_board_item_info = channels_info->relay_board_item_info + i;
		relay_board_item_info->config = 0x00;
		relay_boards_com_update_config(relay_boards_com_info, relay_board_item_info->board_id, relay_board_item_info->config);
	}

	for(i = 0; i < channels_info->power_module_group_number; i++) {
		power_module_group_info_t *power_module_group_info = channels_info->power_module_group_info + i;
		struct list_head *head = &power_module_group_info->power_module_item_list;
		power_module_item_info_t *power_module_item_info;
		list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
	}

	return ret;
}

#define RELAY_CHECK_STATE_TIMEOUT (30 * 1000)

static int handle_channels_relay_check_state(channels_info_t *channels_info)
{
	int ret = -1;
	uint32_t ticks = osKernelSysTick();

	relay_check_info_t *relay_check_info = &channels_info->relay_check_info;

	switch(relay_check_info->relay_check_state) {
		case RELAY_CHECK_STATE_NONE: {
		}
		break;

		case RELAY_CHECK_STATE_STOP: {
		}
		break;

		default: {
			if(relay_check_info->relay_check_request_state == RELAY_CHECK_REQUEST_STATE_STOP) {
				relay_check_info->relay_check_request_state = RELAY_CHECK_REQUEST_STATE_NONE;

				set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 0);
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;
	}

	switch(relay_check_info->relay_check_state) {
		case RELAY_CHECK_STATE_NONE: {
			if(relay_check_info->relay_check_request_state == RELAY_CHECK_REQUEST_STATE_START) {
				relay_check_info->relay_check_request_state = RELAY_CHECK_REQUEST_STATE_NONE;

				set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 0);
				relay_check_info->stamp = ticks;
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_START_SYNC;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;

		case RELAY_CHECK_STATE_START_SYNC: {
			if(ticks_duration(ticks, relay_check_info->stamp) >= RELAY_CHECK_STATE_TIMEOUT) {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_NONE;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				if(relay_check_start_sync(channels_info) == 0) {//等待所有通道都同步到自检状态
					relay_check_info->relay_check_state = RELAY_CHECK_STATE_PREPARE;
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				}
			}
		}
		break;

		case RELAY_CHECK_STATE_PREPARE: {
			if(relay_check_prepare(channels_info) == 0) {//配置所有开关板关断，所有模块组加梯度电压
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_PREPARE_CONFIRM;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;

		case RELAY_CHECK_STATE_PREPARE_CONFIRM: {
			if(relay_check_confirm(channels_info) == 0) {//确认开关板状态及梯度电压
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_INSULATION_CHECK;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;

		case RELAY_CHECK_STATE_INSULATION_CHECK: {//加模块组梯度电压后，检测继电器断开时的开路电压是否为0
			if(relay_check_insulation_check(channels_info) == 0) {
				relay_check_info->stamp = ticks;
				relay_check_info->relay_check_channel_id = 0;
				relay_check_info->relay_check_power_module_group_id = 0;

				relay_check_info->relay_check_state = RELAY_CHECK_STATE_CHANNEL_MODULE_ID_CHECK;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
				set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 1);
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;

		case RELAY_CHECK_STATE_CHANNEL_MODULE_ID_CHECK: {
			int ret = relay_check_channel_module_group_id_check(channels_info);

			if(ret == 0) {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_CHANNEL_CONFIG;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				if(relay_check_channel_module_id_next(channels_info) != 0) {//检测完成
					relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				} else {
					relay_check_info_t *relay_check_info = &channels_info->relay_check_info;

					debug("try channel_id %d module_id %d",
					      relay_check_info->relay_check_channel_id,
					      relay_check_info->relay_check_power_module_group_id);
				}
			}
		}
		break;

		case RELAY_CHECK_STATE_CHANNEL_CONFIG: {
			if(relay_check_channel_config(channels_info) == 0) {
				relay_check_info->stamp = ticks;
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_CHANNEL_CONFIG_SYNC;
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
				set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 1);
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			}
		}
		break;

		case RELAY_CHECK_STATE_CHANNEL_CONFIG_SYNC: {
			if(ticks_duration(ticks, relay_check_info->stamp) >= RELAY_CHECK_STATE_TIMEOUT) {
				relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
				set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 1);
				debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
			} else {
				if(relay_check_confirm(channels_info) == 0) {//确认开关板状态及梯度电压
					relay_check_info->stamp = ticks;
					relay_check_info->relay_check_state = RELAY_CHECK_STATE_CHANNEL_CHECK;
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				}
			}
		}
		break;

		case RELAY_CHECK_STATE_CHANNEL_CHECK: {
			if(relay_check_channel_check(channels_info) == 0) {//反馈电压检查
				if(relay_check_channel_module_id_next(channels_info) == 0) {//检查下一路接线
					relay_check_info_t *relay_check_info = &channels_info->relay_check_info;

					debug("try channel_id %d module_id %d",
					      relay_check_info->relay_check_channel_id,
					      relay_check_info->relay_check_power_module_group_id);

					relay_check_info->relay_check_state = RELAY_CHECK_STATE_CHANNEL_MODULE_ID_CHECK;
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				} else {//测试完毕
					relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				}
			} else {//测试失败
				if(ticks_duration(ticks, relay_check_info->stamp) >= RELAY_CHECK_STATE_TIMEOUT) {
					relay_check_info->relay_check_state = RELAY_CHECK_STATE_STOP;
					set_fault(relay_check_info->faults, RELAY_CHECK_FAULT_FAULT, 1);
					debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
				}
			}
		}
		break;

		case RELAY_CHECK_STATE_STOP: {
			relay_check_stop(channels_info);
			relay_check_info->relay_check_state = RELAY_CHECK_STATE_NONE;
			debug("channels relay check to state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
		}
		break;

		default: {
			debug("channels relay check state %s", get_relay_check_state_des(relay_check_info->relay_check_state));
		}
		break;
	}

	return ret;
}

static void channels_debug(channels_info_t *channels_info)
{
	int i;
	power_module_group_info_t *power_module_group_info;
	power_module_item_info_t *power_module_item_info;
	relay_board_item_info_t *relay_board_item_info;
	channel_info_t *channel_info;
	pdu_group_info_t *pdu_group_info;
	struct list_head *head;
	struct list_head *head1;

	_printf("all module items:\n");

	for(i = 0; i < channels_info->power_module_item_number; i++) {
		power_module_item_info = channels_info->power_module_item_info + i;
		print_power_module_item_info(1, power_module_item_info);
	}

	_printf("\n");

	_printf("all module groups:\n");

	for(i = 0; i < channels_info->power_module_group_number; i++) {
		power_module_group_info = channels_info->power_module_group_info + i;
		print_power_module_group_info(1, power_module_group_info);
	}

	_printf("\n");

	_printf("all relay board items:\n");

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info = channels_info->relay_board_item_info + i;

		print_relay_board_item_info(1, relay_board_item_info);
	}

	_printf("\n");

	for(i = 0; i < channels_info->pdu_group_number; i++) {
		pdu_group_info = channels_info->pdu_group_info + i;

		_printf("pdu group %d module groups:\n", pdu_group_info->pdu_group_id);

		_printf("\tall idle module groups:\n");
		head = &pdu_group_info->power_module_group_idle_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");

		_printf("\tall deactive module groups:\n");
		head = &pdu_group_info->power_module_group_deactive_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");

		_printf("\tall disable module groups:\n");
		head = &pdu_group_info->power_module_group_disable_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");
	}

	_printf("all channels:\n");

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info = channels_info->channel_info + i;

		print_channel_info(1, channel_info);

		head = &channel_info->relay_board_item_list;
		list_for_each_entry(relay_board_item_info, head, relay_board_item_info_t, list) {
			print_relay_board_item_info(2, relay_board_item_info);
		}

		head = &channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
	}

	_printf("\n");

	for(i = 0; i < channels_info->pdu_group_number; i++) {
		pdu_group_info = channels_info->pdu_group_info + i;

		_printf("pdu group %d channels:\n", pdu_group_info->pdu_group_id);

		_printf("\tidle channels:\n");
		head = &pdu_group_info->channel_idle_list;
		list_for_each_entry(channel_info, head, channel_info_t, list) {
			print_channel_info(2, channel_info);

			head1 = &channel_info->relay_board_item_list;
			list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
				print_relay_board_item_info(3, relay_board_item_info);
			}

			head1 = &channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(2, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tactive channels:\n");
		head = &pdu_group_info->channel_active_list;
		list_for_each_entry(channel_info, head, channel_info_t, list) {
			print_channel_info(2, channel_info);

			head1 = &channel_info->relay_board_item_list;
			list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
				print_relay_board_item_info(3, relay_board_item_info);
			}

			head1 = &channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tdeactive channels:\n");
		head = &pdu_group_info->channel_deactive_list;
		list_for_each_entry(channel_info, head, channel_info_t, list) {
			print_channel_info(2, channel_info);

			head1 = &channel_info->relay_board_item_list;
			list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
				print_relay_board_item_info(3, relay_board_item_info);
			}

			head1 = &channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tdisable channels:\n");
		head = &pdu_group_info->channel_disable_list;
		list_for_each_entry(channel_info, head, channel_info_t, list) {
			print_channel_info(2, channel_info);

			head1 = &channel_info->relay_board_item_list;
			list_for_each_entry(relay_board_item_info, head1, relay_board_item_info_t, list) {
				print_relay_board_item_info(3, relay_board_item_info);
			}

			head1 = &channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");
	}
}

static uint8_t dump_channels_stats = 0;

void start_dump_channels_stats(void)
{
	dump_channels_stats = 1;
}

static void do_dump_channels_stats(channels_info_t *channels_info)
{
	if(dump_channels_stats == 0) {
		return;
	}

	dump_channels_stats = 0;

	channels_debug(channels_info);
}

static void handle_channels_fault(channels_info_t *channels_info)
{
	int i;
	uint8_t fault;

	fault = 0;

	if(get_first_fault(channels_info->relay_check_info.faults) != -1) {
		debug("");
		fault = 1;
	}

	set_fault(channels_info->faults, CHANNELS_FAULT_RELAY_CHECK, fault);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		power_module_group_info_t *power_module_group_info;
		struct list_head *head;
		struct list_head *head1;

		fault = 0;
		head = &channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			head1 = &power_module_group_info->power_module_item_list;
			power_module_item_info_t *power_module_item_info;
			uint8_t module_group_size = list_size(head1);
			uint8_t power_module_disble_count = 0;
			list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
				if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_DISABLE) {
					power_module_disble_count++;
					continue;
				}

				if(get_first_fault(power_module_item_info->faults) != -1) {
					fault = 1;
					break;
				}
			}

			if(power_module_disble_count == module_group_size) {
				fault = 1;
			}

			if(fault == 1) {
				break;
			}
		}
		set_fault(channel_info->faults, CHANNEL_FAULT_POWER_MODULE, fault);

		if(fault != 0) {
			debug("");
		}
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		pdu_group_info_t *pdu_group_info = channel_info->pdu_group_info;

		fault = 0;

		if(channel_info->status.state == CHANNEL_STATE_IDLE) {
			continue;
		}

		if(get_first_fault(channels_info->faults) != -1) {
			debug("channels fault!");
			fault = 1;
		}

		if(get_first_fault(pdu_group_info->faults) != -1) {
			debug("pdu_group fault!");
			fault = 1;
		}

		if(get_first_fault(channel_info->faults) != -1) {
			debug("channel %d fault!", channel_info->channel_id);
			fault = 1;
		}

		if(fault != 0) {
			if(channel_info->status.state != CHANNEL_STATE_IDLE) {
				try_to_stop_channel(channel_info);
			}
		}
	}
}

static void channels_process_event(channels_info_t *channels_info)
{
	channels_event_t *channels_event = NULL;
	int ret = -1;

	if(channels_info == NULL) {
		return;
	}

	ret = event_pool_wait_event(channels_info->event_pool, CHANNEL_TASK_PERIODIC);

	if(ret != 0) {
		return;
	}

	channels_event = event_pool_get_event(channels_info->event_pool);

	if(channels_event == NULL) {
		return;
	}

	do_callback_chain(channels_info->common_event_chain, channels_event);

	if(channels_event->event != NULL) {
		os_free(channels_event->event);
	}

	os_free(channels_event);
}

static void handle_gpio_signal(channels_info_t *channels_info)
{
	uint8_t force_stop = 0;
	uint8_t door = 0;
	channels_config_t *channels_config = channels_info->channels_config;

	if(HAL_GPIO_ReadPin(channels_config->gpio_port_force_stop, channels_config->gpio_pin_force_stop) == GPIO_PIN_SET) {
		force_stop = 1;
	}

	if(HAL_GPIO_ReadPin(channels_config->gpio_port_door, channels_config->gpio_pin_door) == GPIO_PIN_SET) {
		door = 1;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP) != force_stop) {
		set_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP, force_stop);

		if(force_stop != 0) {
			debug("");
		}
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_DOOR) != door) {
		set_fault(channels_info->faults, CHANNELS_FAULT_DOOR, door);

		if(door != 0) {
			debug("");
		}
	}
}

static void handle_common_periodic(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	int i;

	for(i = 0; i < channels_info->pdu_group_number; i++) {
		pdu_group_info_t *pdu_group_info = channels_info->pdu_group_info + i;

		handle_channels_change_state(pdu_group_info);
		handle_pdu_group_deactive_list(pdu_group_info);
		handle_pdu_group_disable_list(pdu_group_info);
	}

	handle_channels_max_power_limit(channels_info);

	handle_power_module_items_info_state(channels_info);

	handle_fan_state(channels_info);

	update_relay_boards_info_state(channels_info);

	handle_power_module_policy_request(channels_info);

	handle_power_module_type_config(channels_info);

	handle_channels_relay_check_state(channels_info);

	do_dump_channels_stats(channels_info);

	handle_gpio_signal(channels_info);

	handle_channels_fault(channels_info);
}

static void channels_periodic(channels_info_t *channels_info)
{
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channels_info->periodic_stamp) >= CHANNEL_TASK_PERIODIC) {
		channels_info->periodic_stamp = ticks;

		do_callback_chain(channels_info->common_periodic_chain, channels_info);
	}
}

void task_channels(void const *argument)
{
	channels_info_t *channels_info = (channels_info_t *)argument;

	if(channels_info == NULL) {
		debug("");
		app_panic();
	}

	while(1) {
		if(channels_info->configed == 0) {
			osDelay(10);
			continue;
		}

		channels_process_event(channels_info);

		//处理周期性事件
		channels_periodic(channels_info);
	}
}

int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout)
{
	return event_pool_put_event(channels_info->event_pool, channels_event, timeout);
}

void start_stop_channel(channels_info_t *channels_info, uint8_t channel_id, channel_event_type_t channel_event_type)
{
	uint32_t timeout = 1000;
	channels_event_t *channels_event = (channels_event_t *)os_calloc(1, sizeof(channels_event_t));
	channel_event_t *channel_event = (channel_event_t *)os_calloc(1, sizeof(channel_event_t));
	int ret;

	OS_ASSERT(channels_event != NULL);
	OS_ASSERT(channel_event != NULL);

	channels_event->type = CHANNELS_EVENT_CHANNEL_EVENT;
	channels_event->event = channel_event;

	channel_event->channel_id = channel_id;
	channel_event->type = channel_event_type;

	ret = send_channels_event(channels_info, channels_event, timeout);

	if(ret != 0) {
		debug("");
		os_free(channels_event);
	}
}

__weak void channels_modbus_data_action(void *fn_ctx, void *chain_ctx)
{
	//channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	modbus_data_ctx_t *modbus_data_ctx = (modbus_data_ctx_t *)chain_ctx;

	debug("op:%s, addr:%d, value:%d",
	      (modbus_data_ctx->action == MODBUS_DATA_ACTION_GET) ? "get" :
	      (modbus_data_ctx->action == MODBUS_DATA_ACTION_SET) ? "set" :
	      "unknow",
	      modbus_data_ctx->addr,
	      modbus_data_ctx->value);
}

__weak void load_channels_display_cache(channels_info_t *channels_info)
{
}

__weak void sync_channels_display_cache(channels_info_t *channels_info)
{
}

static void channels_info_channels_settings_invalid(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	modbus_data_ctx_t *modbus_data_ctx = (modbus_data_ctx_t *)chain_ctx;

	if(modbus_data_ctx->influence < (void *)&channels_info->channels_settings) {
		return;
	}

	if(modbus_data_ctx->influence >= (void *)(&channels_info->channels_settings + 1)) {
		return;
	}

	debug("[%p, %p, %p]", &channels_info->channels_settings, modbus_data_ctx->influence, &channels_info->channels_settings + 1);

	channels_info->channels_settings_invalid = 1;
}

static void channels_info_channels_settings_changed(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;

	sync_channels_display_cache(channels_info);

	if(channels_info->channels_settings_invalid != 0) {
		channels_info->channels_settings_invalid = 0;
		channels_info_save_config(channels_info);
	}
}

static int channels_set_channels_config(channels_info_t *channels_info, channels_config_t *channels_config)
{
	int ret = -1;
	int i;
	uint32_t ticks = osKernelSysTick();
	uint8_t power_module_group_offset = 0;
	uint8_t power_module_item_offset = 0;
	uint8_t channel_offset = 0;
	uint8_t relay_board_item_offset = 0;
	int relay_board_power_module_group_id_offset = 0;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	pdu_config_t *pdu_config = &channels_settings->pdu_config;
	display_info_t *display_info;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info->storage_info != NULL);
	channels_info->storage_info = app_info->storage_info;

	osThreadDef(task_channels, task_channels, osPriorityNormal, 0, 128 * 2 * 2);
	//osThreadDef(task_channel_event, task_channel_event, osPriorityNormal, 0, 128 * 2);

	init_channels_settings(channels_info);

	channels_info->pdu_group_number = pdu_config->pdu_group_number;
	OS_ASSERT(channels_info->pdu_group_number != 0);
	OS_ASSERT(channels_info->pdu_group_number <= PDU_GROUP_NUMBER_MAX);
	debug("channels_info->pdu_group_number:%d", channels_info->pdu_group_number);

	channels_info->power_module_group_number = get_power_module_group_number(pdu_config);
	OS_ASSERT(channels_info->power_module_group_number != 0);
	debug("channels_info->power_module_group_number:%d", channels_info->power_module_group_number);

	channels_info->power_module_item_number = get_power_module_item_number(pdu_config);
	OS_ASSERT(channels_info->power_module_item_number != 0);
	debug("channels_info->power_module_item_number:%d", channels_info->power_module_item_number);

	channels_info->channel_number = get_channel_number(pdu_config);
	OS_ASSERT(channels_info->channel_number != 0);
	debug("channels_info->channel_number:%d", channels_info->channel_number);

	channels_info->relay_board_item_number = get_relay_board_item_number(pdu_config);
	OS_ASSERT(channels_info->relay_board_item_number != 0);
	debug("channels_info->relay_board_item_number:%d", channels_info->relay_board_item_number);

	//分配pdu组信息
	channels_info->pdu_group_info = (pdu_group_info_t *)os_calloc(channels_info->pdu_group_number, sizeof(pdu_group_info_t));
	OS_ASSERT(channels_info->pdu_group_info != NULL);

	//分配模块组信息
	channels_info->power_module_group_info = (power_module_group_info_t *)os_calloc(channels_info->power_module_group_number, sizeof(power_module_group_info_t));
	OS_ASSERT(channels_info->power_module_group_info != NULL);

	//分配模块信息
	channels_info->power_module_item_info = (power_module_item_info_t *)os_calloc(channels_info->power_module_item_number, sizeof(power_module_item_info_t));
	OS_ASSERT(channels_info->power_module_item_info != NULL);

	for(i = 0; i < channels_info->power_module_item_number; i++) {
		power_module_item_info_t *power_module_item_info = channels_info->power_module_item_info + i;
		power_module_item_info->faults = alloc_bitmap(POWER_MODULE_ITEM_FAULT_SIZE);
		OS_ASSERT(power_module_item_info->faults != NULL);
	}

	//分配通道信息
	channels_info->channel_info = (channel_info_t *)os_calloc(channels_info->channel_number, sizeof(channel_info_t));
	OS_ASSERT(channels_info->channel_info != NULL);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info_item = channels_info->channel_info + i;
		channel_info_item->faults = alloc_bitmap(CHANNEL_FAULT_SIZE);
		OS_ASSERT(channel_info_item->faults != NULL);
	}


	//分配开关板信息
	channels_info->relay_board_item_info = (relay_board_item_info_t *)os_calloc(channels_info->relay_board_item_number, sizeof(relay_board_item_info_t));
	OS_ASSERT(channels_info->relay_board_item_info != NULL);

	for(i = 0; i < channels_info->relay_board_item_number; i++) {
		relay_board_item_info_t *relay_board_item_info_item = channels_info->relay_board_item_info + i;
		relay_board_item_info_item->faults = alloc_bitmap(RELAY_BOARD_ITEM_FAULT_SIZE);
		OS_ASSERT(relay_board_item_info_item->faults != NULL);
	}

	channels_config->power_module_config.power_module_number = channels_info->power_module_item_number;
	channels_config->channel_number = channels_info->channel_number;
	channels_config->relay_board_number = channels_info->relay_board_item_number;

	channels_info->power_modules_info = alloc_power_modules_info(channels_info);
	OS_ASSERT(channels_info->power_modules_info != NULL);
	power_modules_set_type(channels_info->power_modules_info, channels_settings->power_module_settings.power_module_type);

	channels_info->channels_com_info = get_or_alloc_channels_com_info(channels_info);
	OS_ASSERT(channels_info->channels_com_info != NULL);

	channels_info->relay_boards_com_info = get_or_alloc_relay_boards_com_info(channels_info);
	OS_ASSERT(channels_info->relay_boards_com_info != NULL);

	for(i = 0; i < pdu_config->pdu_group_number; i++) {
		pdu_group_config_t *pdu_group_config = &pdu_config->pdu_group_config[i];
		pdu_group_info_t *pdu_group_info_item = channels_info->pdu_group_info + i;
		int j;

		pdu_group_info_item->faults = alloc_bitmap(PDU_GROUP_FAULT_SIZE);
		OS_ASSERT(pdu_group_info_item->faults != 0);
		OS_ASSERT(pdu_group_config->channel_number != 0);
		OS_ASSERT(pdu_group_config->relay_board_number_per_channel != 0);
		OS_ASSERT(pdu_group_config->relay_board_number_per_channel <= RELAY_BROAD_NUMBER_PER_CHANNEL_MAX);

		pdu_group_info_item->pdu_group_id = i;
		pdu_group_info_item->channel_number = pdu_group_config->channel_number;
		pdu_group_info_item->power_module_group_number = get_power_module_group_number_per_pdu_group_config(pdu_group_config);

		INIT_LIST_HEAD(&pdu_group_info_item->channel_idle_list);
		INIT_LIST_HEAD(&pdu_group_info_item->channel_active_list);
		INIT_LIST_HEAD(&pdu_group_info_item->channel_deactive_list);
		INIT_LIST_HEAD(&pdu_group_info_item->channel_disable_list);
		INIT_LIST_HEAD(&pdu_group_info_item->power_module_group_idle_list);
		INIT_LIST_HEAD(&pdu_group_info_item->power_module_group_deactive_list);
		INIT_LIST_HEAD(&pdu_group_info_item->power_module_group_disable_list);

		pdu_group_info_item->channels_change_state = CHANNELS_CHANGE_STATE_IDLE;
		pdu_group_info_item->channels_info = channels_info;

		for(j = 0; j < RELAY_BROAD_NUMBER_PER_CHANNEL_MAX; j++) {
			int k;

			for(k = 0; k < pdu_group_config->power_module_group_number_per_relay_board[j]; k++) {
				int l;
				uint8_t group_id = power_module_group_offset++;
				power_module_group_info_t *power_module_group_info_item = channels_info->power_module_group_info + group_id;
				uint8_t power_module_number = pdu_group_config->power_module_number_per_power_module_group;

				OS_ASSERT(power_module_number != 0);
				OS_ASSERT(group_id < channels_info->power_module_group_number);
				list_add_tail(&power_module_group_info_item->list, &pdu_group_info_item->power_module_group_idle_list);
				power_module_group_info_item->group_id = group_id;
				power_module_group_info_item->pdu_group_info = pdu_group_info_item;
				INIT_LIST_HEAD(&power_module_group_info_item->power_module_item_list);

				for(l = 0; l < power_module_number; l++) {
					uint8_t module_id = power_module_item_offset++;
					power_module_item_info_t *power_module_item_info = channels_info->power_module_item_info + module_id;
					OS_ASSERT(module_id < channels_info->power_module_item_number);
					list_add_tail(&power_module_item_info->list, &power_module_group_info_item->power_module_item_list);
					power_module_item_info->module_id = module_id;
					power_module_item_info->power_modules_info = channels_info->power_modules_info;
					power_module_item_info->power_module_group_info = power_module_group_info_item;
					power_module_item_info->query_stamp = ticks - (1 * 1000);
					power_module_item_info->setting_stamp = ticks - (2 * 1000);
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
				}
			}
		}

		//当前组分配通道
		for(j = 0; j < pdu_group_info_item->channel_number; j++) {
			uint8_t channel_id = channel_offset++;
			channel_info_t *channel_info_item = channels_info->channel_info + channel_id;
			int k;
			//当前通道当前开关板电源模块组偏移
			int relay_board_offset = relay_board_power_module_group_id_offset;

			OS_ASSERT(channel_id < channels_info->channel_number);
			list_add_tail(&channel_info_item->list, &pdu_group_info_item->channel_idle_list);
			channel_info_item->channel_id = channel_id;
			channel_info_item->channels_info = channels_info;
			channel_info_item->pdu_group_info = pdu_group_info_item;
			channel_info_item->status.state = CHANNEL_STATE_IDLE;
			channel_info_item->channel_request_state = CHANNEL_REQUEST_STATE_NONE;
			channel_info_item->relay_board_number = pdu_group_config->relay_board_number_per_channel;
			INIT_LIST_HEAD(&channel_info_item->relay_board_item_list);

			//初始化当前通道对应的开关板信息
			for(k = 0; k < channel_info_item->relay_board_number; k++) {
				//当前通道当前开关板连接的模块组数
				uint8_t relay_board_power_module_group_number = pdu_group_config->power_module_group_number_per_relay_board[k];
				//计算当前通道当前开关板的id
				uint8_t board_id = relay_board_item_offset++;
				//当前通道当前开关板
				relay_board_item_info_t *relay_board_item_info_item = channels_info->relay_board_item_info + board_id;

				OS_ASSERT(board_id < channels_info->relay_board_item_number);
				list_add_tail(&relay_board_item_info_item->list, &channel_info_item->relay_board_item_list);
				relay_board_item_info_item->board_id = board_id;
				relay_board_item_info_item->channel_id = channel_id;
				relay_board_item_info_item->relay_boards_com_info = channels_info->relay_boards_com_info;
				relay_board_item_info_item->offset = relay_board_offset;
				relay_board_item_info_item->number = relay_board_power_module_group_number;//模块组数
				relay_board_offset += relay_board_power_module_group_number;
			}

			INIT_LIST_HEAD(&channel_info_item->power_module_group_list);
		}

		relay_board_power_module_group_id_offset += power_module_group_offset;
	}

	OS_ASSERT(channel_offset == channels_info->channel_number);
	OS_ASSERT(power_module_group_offset == channels_info->power_module_group_number);
	OS_ASSERT(power_module_item_offset == channels_info->power_module_item_number);
	OS_ASSERT(relay_board_item_offset == channels_info->relay_board_item_number);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		channel_info->common_event_callback_item.fn = default_handle_channel_event;
		channel_info->common_event_callback_item.fn_ctx = channel_info;
		OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_info->common_event_callback_item) == 0);

		channel_info->common_periodic_callback_item.fn = default_handle_channel_periodic;
		channel_info->common_periodic_callback_item.fn_ctx = channel_info;
		OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_info->common_periodic_callback_item) == 0);
	}

	channels_info->common_periodic_callback_item.fn = handle_common_periodic;
	channels_info->common_periodic_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_info->common_periodic_callback_item) == 0);

	alloc_display_info(channels_info);
	OS_ASSERT(channels_info->display_info != NULL);
	display_info = (display_info_t *)channels_info->display_info;

	channels_info->display_data_action_callback_item.fn = channels_modbus_data_action;
	channels_info->display_data_action_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(display_info->modbus_slave_info->data_action_chain, &channels_info->display_data_action_callback_item) == 0);

	channels_info->display_data_invalid_callback_item.fn = channels_info_channels_settings_invalid;
	channels_info->display_data_invalid_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(display_info->modbus_slave_info->data_invalid_chain, &channels_info->display_data_invalid_callback_item) == 0);

	channels_info->display_data_changed_callback_item.fn = channels_info_channels_settings_changed;
	channels_info->display_data_changed_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(display_info->modbus_slave_info->data_changed_chain, &channels_info->display_data_changed_callback_item) == 0);

	{
		//int i;

		debug("modbus addr max:%d!", MODBUS_ADDR_SIZE - 1);

		//for(i = 0; i < MODBUS_ADDR_SIZE; i++) {
		//	modbus_data_ctx_t modbus_data_ctx;
		//	modbus_data_ctx.ctx = NULL;
		//	modbus_data_ctx.action = MODBUS_DATA_ACTION_GET;
		//	modbus_data_ctx.addr = i;
		//	channels_modbus_data_action(channels_info, &modbus_data_ctx);
		//}
	}

	osThreadCreate(osThread(task_channels), channels_info);
	//osThreadCreate(osThread(task_channel_event), channels_info);

	channels_info->configed = 1;

	ret = 0;

	return ret;
}

static channels_info_t *alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;
	channels_info = (channels_info_t *)os_calloc(1, sizeof(channels_info_t));
	OS_ASSERT(channels_info != NULL);
	channels_info->channels_config = channels_config;

	channels_info->event_pool = alloc_event_pool();
	OS_ASSERT(channels_info->event_pool != NULL);

	channels_info->faults = alloc_bitmap(CHANNELS_FAULT_SIZE);
	OS_ASSERT(channels_info->faults != NULL);

	channels_info->relay_check_info.faults = alloc_bitmap(RELAY_CHECK_FAULT_SIZE);
	OS_ASSERT(channels_info->relay_check_info.faults != NULL);

	channels_info->common_periodic_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_periodic_chain != NULL);

	channels_info->common_event_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_event_chain != NULL);


	OS_ASSERT(channels_set_channels_config(channels_info, channels_config) == 0);

	return channels_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channels_info_t *channels_info = (channels_info_t *)o;
	channels_config_t *channels_config = (channels_config_t *)ctx;

	if(channels_info->channels_config == channels_config) {
		ret = 0;
	}

	return ret;
}

channels_info_t *get_or_alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;

	os_enter_critical();

	if(channels_class == NULL) {
		channels_class = object_class_alloc();
	}

	os_leave_critical();

	channels_info = (channels_info_t *)object_class_get_or_alloc_object(channels_class, object_filter, channels_config, (object_alloc_t)alloc_channels_info, (object_free_t)free_channels_info);

	return channels_info;
}

channels_info_t *start_channels(void)
{
	channels_info_t *channels_info;
	channels_config_t *channels_config = get_channels_config(0);

	channels_info = get_or_alloc_channels_info(channels_config);
	OS_ASSERT(channels_info != NULL);

	return channels_info;
}
