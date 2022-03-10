

/*================================================================
 *
 *
 *   文件名称：power_manager_handler_native.c
 *   创 建 者：肖飞
 *   创建日期：2021年11月23日 星期二 15时40分30秒
 *   修改日期：2022年03月09日 星期三 16时46分31秒
 *   描    述：
 *
 *================================================================*/
#include "power_manager_handler_native.h"

#include <stdlib.h>

#include "channel.h"

#include "log.h"

typedef struct {
	uint32_t power_modules_status_periodic_stamp;
} power_manager_ctx_t;

#define POWER_MANAGER_CHANNEL_PREPARE_START_TIMEOUT (30 * 1000)
#define POWER_MANAGER_CHANNEL_PREPARE_STOP_TIMEOUT (30 * 1000)

static void handle_power_manager_channel_state(power_manager_channel_info_t *power_manager_channel_info)
{
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_manager_channel_info->power_manager_group_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	channels_info_t *channels_info = power_manager_info->channels_info;
	channel_info_t *channel_info = channels_info->channel_info + power_manager_channel_info->id;
	uint32_t ticks = osKernelSysTick();
	power_manager_channel_info->status.require_output_voltage = 0;
	power_manager_channel_info->status.require_output_current = 0;
	power_manager_channel_info->status.max_output_power = channel_info->channel_settings.max_output_power;

	switch(power_manager_channel_info->status.state) {
		case POWER_MANAGER_CHANNEL_STATE_IDLE: {
			if(power_manager_channel_info->request_state == POWER_MANAGER_CHANNEL_REQUEST_STATE_START) {//开机
				power_manager_channel_info->status.module_ready_notify = 1;

				if((power_manager_info->power_manager_group_policy_handler != NULL) &&
				   (power_manager_info->power_manager_group_policy_handler->channel_start != NULL)) {
					power_manager_info->power_manager_group_policy_handler->channel_start(power_manager_channel_info);
				}

				list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_active_list);
				power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE;//模块组重分配
				debug("power manager group %d change to state %s",
				      power_manager_group_info->id,
				      get_power_manager_group_change_state_des(power_manager_group_info->change_state));

				power_manager_channel_info->state_change_stamp = ticks;
				power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_PREPARE_START;
				debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
			}
		}
		break;

		case POWER_MANAGER_CHANNEL_STATE_PREPARE_START: {
			if(power_manager_channel_info->status.require_output_voltage != channel_info->require_voltage) {
				power_manager_channel_info->status.require_output_voltage = channel_info->require_voltage;
				debug("power manager channel %d require output voltage:%d", power_manager_channel_info->id, power_manager_channel_info->status.require_output_voltage);
			}

			if(power_manager_channel_info->status.require_output_current != channel_info->require_current) {
				power_manager_channel_info->status.require_output_current = channel_info->require_current;
				debug("power manager channel %d require output voltage:%d", power_manager_channel_info->id, power_manager_channel_info->status.require_output_current);
			}

			if(power_manager_channel_info->request_state == POWER_MANAGER_CHANNEL_REQUEST_STATE_STOP) {//关机
				power_manager_channel_info->request_state = POWER_MANAGER_CHANNEL_REQUEST_STATE_NONE;

				list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_deactive_list);
				power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组
				debug("power manager group %d change to state %s",
				      power_manager_group_info->id,
				      get_power_manager_group_change_state_des(power_manager_group_info->change_state));

				power_manager_channel_info->state_change_stamp = ticks;
				power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP;
				debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
			} else {
				if(list_contain(&power_manager_channel_info->list, &power_manager_group_info->channel_active_list) != 0) {//无法启动，停止启动过程
					power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_IDLE;
					debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
					channel_request_stop(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_POWER_MANAGER);
				} else {
					if(power_manager_group_info->change_state != POWER_MANAGER_CHANGE_STATE_IDLE) {//配置没有同步完成
						if(ticks_duration(ticks, power_manager_channel_info->state_change_stamp) >= POWER_MANAGER_CHANNEL_PREPARE_START_TIMEOUT) {//超时,报警?
							debug("power manager group %d change state %s! channel_id %d in state %s!",
							      power_manager_group_info->id,
							      get_power_manager_group_change_state_des(power_manager_group_info->change_state),
							      power_manager_channel_info->id,
							      get_power_manager_channel_state_des(power_manager_channel_info->status.state));

							list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_deactive_list);
							power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组
							debug("power manager group %d change to state %s",
							      power_manager_group_info->id,
							      get_power_manager_group_change_state_des(power_manager_group_info->change_state));

							power_manager_channel_info->state_change_stamp = ticks;
							power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP;
							debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
						}
					} else {
						power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_START;
						debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
					}
				}
			}
		}
		break;

		case POWER_MANAGER_CHANNEL_STATE_START: {
			power_manager_channel_info->status.require_output_voltage = channel_info->require_voltage;
			power_manager_channel_info->status.require_output_current = channel_info->require_current;

			power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_RUNNING;
			debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
		}
		break;

		case POWER_MANAGER_CHANNEL_STATE_RUNNING: {
			power_manager_channel_info->status.require_output_voltage = channel_info->require_voltage;
			power_manager_channel_info->status.require_output_current = channel_info->require_current;

			if(power_manager_channel_info->request_state == POWER_MANAGER_CHANNEL_REQUEST_STATE_STOP) {//关机
				power_manager_channel_info->request_state = POWER_MANAGER_CHANNEL_REQUEST_STATE_NONE;

				list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_deactive_list);
				power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE;//重新分配模块组
				debug("power manager group %d change to state %s",
				      power_manager_group_info->id,
				      get_power_manager_group_change_state_des(power_manager_group_info->change_state));

				power_manager_channel_info->state_change_stamp = ticks;
				power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP;
				debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
			} else {
				if((power_manager_info->power_manager_group_policy_handler != NULL) &&
				   (power_manager_info->power_manager_group_policy_handler->channel_charging != NULL)) {
					power_manager_info->power_manager_group_policy_handler->channel_charging(power_manager_channel_info);
				}
			}
		}
		break;

		case POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP: {
			if(power_manager_group_info->change_state != POWER_MANAGER_CHANGE_STATE_IDLE) {//配置没有同步完成
				if(ticks_duration(ticks, power_manager_channel_info->state_change_stamp) >= POWER_MANAGER_CHANNEL_PREPARE_STOP_TIMEOUT) {//超时,报警?
					debug("power manager group %d change state %s! channel_id %d in state %s!",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state),
					      power_manager_channel_info->id,
					      get_power_manager_channel_state_des(power_manager_channel_info->status.state));
				}
			} else {
				power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_STOP;
				debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
			}
		}
		break;

		case POWER_MANAGER_CHANNEL_STATE_STOP: {
			list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_idle_list);

			power_manager_channel_info->status.state = POWER_MANAGER_CHANNEL_STATE_IDLE;
			debug("power manager channel %d to state %s", power_manager_channel_info->id, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
		}
		break;

		default: {
		}
		break;
	}

	channel_info->power_manager_channel_state = power_manager_channel_info->status.state;
}

static void channel_power_limit_correction(uint32_t channel_max_output_power, uint16_t *voltage, uint16_t *current)
{
	uint32_t power = *voltage * *current;

	uint32_t max_power = channel_max_output_power * 10;

	if(power > max_power) {
		uint16_t limit_current = max_power / *voltage;

		if(limit_current < *current) {
			*current = limit_current;
		}
	}
}

static void power_module_voltage_current_correction(channels_settings_t *channels_settings, uint16_t *voltage, uint16_t *current)
{
	if(*voltage == 0) {
		*current = 0;
		return;
	}

	if(*current == 0) {
		*voltage = 0;
		return;
	}

	//debug("voltage:%d, current:%d", *voltage, *current);
	//debug("module_max_output_voltage:%d, module_min_output_voltage:%d, module_max_output_current:%d, module_min_output_current:%d",
	//      channels_settings->module_max_output_voltage,
	//      channels_settings->module_min_output_voltage,
	//      channels_settings->module_max_output_current,
	//      channels_settings->module_min_output_current);

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

static void power_module_power_limit_correction(channels_settings_t *channels_settings, uint16_t *voltage, uint16_t *current)
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

static void handle_power_manager_channel_power_module_group_info(power_manager_channel_info_t *power_manager_channel_info, uint16_t module_require_voltage, uint16_t module_group_require_current)
{
	power_module_group_info_t *power_module_group_info;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_manager_channel_info->power_manager_group_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	channels_info_t *channels_info = power_manager_info->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	struct list_head *head = &power_manager_channel_info->power_module_group_list;
	uint16_t charge_output_voltage_module = 0;
	uint16_t charge_output_current_module = 0;

	list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		uint16_t module_require_current;
		power_module_item_info_t *power_module_item_info;
		uint8_t power_module_number = list_size(head1);

		if(power_module_number == 0) {
			debug("");
			continue;
		}

		module_require_current = module_group_require_current / power_module_number;

		if((module_require_current == 0) && (power_manager_channel_info->status.require_output_current != 0)) {
			module_require_current = channels_settings->module_min_output_current;
		}

		//debug("setting power module group %d module_require_voltage:%d, require_output_current:%d",
		//      power_module_group_info->id,
		//      module_require_voltage,
		//      module_require_current);
		power_module_voltage_current_correction(channels_settings, &module_require_voltage, &module_require_current);
		//debug("setting power module group %d module_require_voltage:%d, require_output_current:%d",
		//      power_module_group_info->id,
		//      module_require_voltage,
		//      module_require_current);
		power_module_power_limit_correction(channels_settings, &power_manager_channel_info->status.charge_output_voltage, &module_require_current);
		//debug("setting power module group %d module_require_voltage:%d, require_output_current:%d",
		//      power_module_group_info->id,
		//      module_require_voltage,
		//      module_require_current);

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.require_output_voltage = module_require_voltage;
			power_module_item_info->status.require_output_current = module_require_current;
			//debug("setting power module %d require_output_voltage:%d, require_output_current:%d",
			//      power_module_item_info->id,
			//      power_module_item_info->status.require_output_voltage,
			//      power_module_item_info->status.require_output_current);

			charge_output_voltage_module = power_module_item_info->status.module_output_voltage;
			charge_output_current_module += power_module_item_info->status.module_output_current;
		}
	}
	power_manager_channel_info->status.charge_output_voltage_module = charge_output_voltage_module;
	power_manager_channel_info->status.charge_output_current_module = charge_output_current_module;
	//debug("channel %d charge_output_voltage_module:%d, charge_output_current_module:%d",
	//      power_manager_channel_info->id,
	//      power_manager_channel_info->status.charge_output_voltage_module,
	//      power_manager_channel_info->status.charge_output_current_module);
}

static void handle_power_manager_channel_power(power_manager_channel_info_t *power_manager_channel_info)
{
	struct list_head *head;
	uint8_t module_group_size;
	uint16_t module_require_voltage;
	uint16_t module_group_require_current;
	uint16_t charge_output_voltage = power_manager_channel_info->status.charge_output_voltage;
	uint16_t require_output_current = power_manager_channel_info->status.require_output_current;

	head = &power_manager_channel_info->power_module_group_list;

	module_group_size = list_size(head);

	if(module_group_size == 0) {
		power_manager_channel_info->status.charge_output_voltage_module = 0;
		power_manager_channel_info->status.charge_output_current_module = 0;
		return;
	}

	//debug("setting channel %d require_output_voltage:%d, require_output_current:%d",
	//      power_manager_channel_info->id,
	//      power_manager_channel_info->status.require_output_voltage,
	//      power_manager_channel_info->status.require_output_current);

	module_require_voltage = power_manager_channel_info->status.require_output_voltage;
	//debug("channel %d module_require_voltage:%d",
	//      power_manager_channel_info->id,
	//      module_require_voltage);
	channel_power_limit_correction(power_manager_channel_info->status.max_output_power, &charge_output_voltage, &require_output_current);
	//debug("channel %d max_output_power:%d, charge_output_voltage:%d, require_output_current:%d",
	//      power_manager_channel_info->id,
	//      power_manager_channel_info->status.max_output_power,
	//      charge_output_voltage,
	//      require_output_current);
	module_group_require_current = require_output_current / module_group_size;

	handle_power_manager_channel_power_module_group_info(power_manager_channel_info, module_require_voltage, module_group_require_current);
}

#define POWER_MODULES_QUERY_PERIODIC (1000)

static int power_manager_group_info_power_module_group_ready_sync(power_manager_group_info_t *power_manager_group_info)
{
	int ret = 0;
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;
	struct list_head *head1;
	struct list_head *head2;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		power_module_group_info_t *power_module_group_info;
		head1 = &power_manager_channel_info->power_module_group_list;
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

static int power_manager_group_poewr_module_assign_ready(power_manager_group_info_t *power_manager_group_info)
{
	int ret = 0;
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		if(power_manager_channel_info->status.module_ready_notify == 1) {
			power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
			channels_info_t *channels_info = power_manager_info->channels_info;
			channel_info_t *channel_info = channels_info->channel_info + power_manager_channel_info->id;

			power_manager_channel_info->status.module_ready_notify = 0;
			debug("channel %d module assign ready", power_manager_channel_info->id);
			do_callback_chain(power_manager_info->power_manager_channel_module_assign_ready_chain, channel_info);
		}
	}
	return ret;
}

static void handle_power_manager_group_change_state(power_manager_group_info_t *power_manager_group_info)
{
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;

	switch(power_manager_group_info->change_state) {
		case POWER_MANAGER_CHANGE_STATE_IDLE: {
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE: {//准备释放模块组
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->free != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->free(power_manager_group_info) == 0) {
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_FREE;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_FREE: {//释放模块组-停用模块列表为空
			if(list_empty(&power_manager_group_info->power_module_group_deactive_list)) {//所有该停用的模块组停用完毕
				power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG;
				debug("power manager group %d change to state %s",
				      power_manager_group_info->id,
				      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG: {//释放模块组后下发配置
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->config != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->config(power_manager_group_info) == 0) {
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC: {//释放模块组后配置同步
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->sync != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->sync(power_manager_group_info) == 0) {
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN: {//分配模块组
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->assign != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->assign(power_manager_group_info) == 0) {
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_READY_SYNC;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_READY_SYNC: {//根据情况等待模块组预充
			if(power_manager_group_info_power_module_group_ready_sync(power_manager_group_info) == 0) {
				power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG;
				debug("power manager group %d change to state %s",
				      power_manager_group_info->id,
				      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG: {//分配模块组后下发配置
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->config != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->config(power_manager_group_info) == 0) {
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		case POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC: {//分配模块组后同步配置
			if((power_manager_info->power_manager_group_policy_handler != NULL) &&
			   (power_manager_info->power_manager_group_policy_handler->sync != NULL)) {
				if(power_manager_info->power_manager_group_policy_handler->sync(power_manager_group_info) == 0) {
					power_manager_group_poewr_module_assign_ready(power_manager_group_info);
					power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_IDLE;
					debug("power manager group %d change to state %s",
					      power_manager_group_info->id,
					      get_power_manager_group_change_state_des(power_manager_group_info->change_state));
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_power_manager_group_deactive_list(power_manager_group_info_t *power_manager_group_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;

	head = &power_manager_group_info->power_module_group_deactive_list;

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
			power_module_group_info->power_manager_channel_info = NULL;
			debug("set power module group %d idle!", power_module_group_info->id);
			list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_idle_list);
			continue;
		}

		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state == POWER_MODULE_ITEM_STATE_DISABLE) {
				power_module_group_disable = 1;
				break;
			}
		}

		if(power_module_group_disable == 1) {
			power_module_group_info->power_manager_channel_info = NULL;
			debug("set power module group %d disable", power_module_group_info->id);
			list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_disable_list);
			continue;
		}
	}
}

static void handle_power_manager_group_disable_list(power_manager_group_info_t *power_manager_group_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;

	head = &power_manager_group_info->power_module_group_disable_list;

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
			debug("reenable power module group %d", power_module_group_info->id);
			list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_idle_list);
		}
	}
}

static void update_power_manager_group_policy(power_manager_info_t *power_manager_info)
{
	channels_info_t *channels_info = power_manager_info->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;

	if((power_manager_info->power_manager_group_policy_handler == NULL) ||
	   (power_manager_info->power_manager_group_policy_handler->policy != channels_settings->power_manager_group_policy)) {

		if((power_manager_info->power_manager_group_policy_handler != NULL) &&
		   (power_manager_info->power_manager_group_policy_handler->deinit != NULL)) {
			OS_ASSERT(power_manager_info->power_manager_group_policy_handler->deinit(power_manager_info) == 0);
			debug("reset power manager group policy handler %s", get_power_manager_group_policy_des(power_manager_info->power_manager_group_policy_handler->policy));
		}

		debug("set power manager group policy handler %s", get_power_manager_group_policy_des(channels_settings->power_manager_group_policy));

		power_manager_info->power_manager_group_policy_handler = get_power_manager_group_policy_handler(channels_settings->power_manager_group_policy);

		if((power_manager_info->power_manager_group_policy_handler != NULL) &&
		   (power_manager_info->power_manager_group_policy_handler->init != NULL)) {
			OS_ASSERT(power_manager_info->power_manager_group_policy_handler->init(power_manager_info) == 0);
		} else {
			debug("skip init power manager group policy handler %s", get_power_manager_group_policy_des(channels_settings->power_manager_group_policy));
		}
	}
}

static void update_poewr_module_item_info_status(power_module_item_info_t *power_module_item_info)
{
	power_modules_info_t *power_modules_info = (power_modules_info_t *)power_module_item_info->power_modules_info;
	power_module_info_t *power_module_info = power_modules_info->power_module_info + power_module_item_info->id;
	power_module_item_status_t *status = &power_module_item_info->status;
	power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
	power_manager_group_info_t *power_manager_group_info = power_module_group_info->power_manager_group_info;
	power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)power_module_group_info->power_manager_channel_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	u_power_module_status_t u_power_module_status;
	uint32_t ticks = osKernelSysTick();
	uint8_t connect_timeout;
	uint8_t fault_changed = 0;
	uint8_t over_voltage = 0;
	uint8_t low_voltage = 0;

	if(ticks_duration(ticks, power_module_item_info->query_stamp) < (1 * 1000)) {
		return;
	}

	power_module_item_info->query_stamp = ticks;

	power_modules_query_status(power_modules_info, power_module_item_info->id);
	power_modules_query_a_line_input_voltage(power_modules_info, power_module_item_info->id);
	power_modules_query_b_line_input_voltage(power_modules_info, power_module_item_info->id);
	power_modules_query_c_line_input_voltage(power_modules_info, power_module_item_info->id);

	status->module_output_voltage = power_module_info->output_voltage;

	status->module_output_current = power_module_info->output_current;

	u_power_module_status.s = power_module_info->power_module_status;

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_FAULT) != u_power_module_status.s.fault) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_FAULT, u_power_module_status.s.fault);

		if(u_power_module_status.s.fault != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if(ticks_duration(ticks, get_power_module_connect_stamp(power_modules_info, power_module_item_info->id)) >= (10 * 1000)) {
		if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_DISABLE) {
			debug("ticks:%d, power module %d stamps:%d", ticks, power_module_item_info->id, get_power_module_connect_stamp(power_modules_info, power_module_item_info->id));
		}

		connect_timeout = 1;
	} else {
		connect_timeout = 0;
	}

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_CONNECT_TIMEOUT) != connect_timeout) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_CONNECT_TIMEOUT, connect_timeout);

		if(connect_timeout != 0) {
			debug("power module %d connect timeout!", power_module_item_info->id);
		}

		fault_changed = 1;
	}

	if((power_module_info->input_aline_voltage < 1900) || (power_module_info->input_bline_voltage < 1900) || (power_module_info->input_cline_voltage < 1900)) {
		debug("power module %d input_aline_voltage:%d, input_bline_voltage:%d, input_cline_voltage:%d",
		      power_module_item_info->id,
		      power_module_info->input_aline_voltage,
		      power_module_info->input_bline_voltage,
		      power_module_info->input_cline_voltage);
		low_voltage = 1;
	} else if((power_module_info->input_aline_voltage > 2500) || (power_module_info->input_bline_voltage > 2500) || (power_module_info->input_cline_voltage > 2500)) {
		debug("power module %d input_aline_voltage:%d, input_bline_voltage:%d, input_cline_voltage:%d",
		      power_module_item_info->id,
		      power_module_info->input_aline_voltage,
		      power_module_info->input_bline_voltage,
		      power_module_info->input_cline_voltage);
		over_voltage = 1;
	}

	if(low_voltage == 0) {
		power_module_item_info->input_lowvoltage_stamps = ticks;
	}

	if(over_voltage == 0) {
		power_module_item_info->input_overvoltage_stamps = ticks;
	}

	if(ticks_duration(ticks, power_module_item_info->input_lowvoltage_stamps) >= 5000) {
		low_voltage = 1;
	} else {
		low_voltage = 0;
	}

	if(get_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_LOW_VOLTAGE) != low_voltage) {
		set_fault(power_module_item_info->faults, POWER_MODULE_ITEM_FAULT_INPUT_LOW_VOLTAGE, low_voltage);

		if(low_voltage != 0) {
			debug("");
		}

		fault_changed = 1;
	}

	if(ticks_duration(ticks, power_module_item_info->input_overvoltage_stamps) >= 5000) {
		over_voltage = 1;
	} else {
		over_voltage = 0;
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

		if(list_contain(&power_module_group_info->list, &power_manager_group_info->power_module_group_deactive_list) == 0) {
			power_module_group_disable = 0;
		} else if(list_contain(&power_module_group_info->list, &power_manager_group_info->power_module_group_disable_list) == 0) {
			power_module_group_disable = 0;
		}

		if(power_module_group_disable == 1) {
			power_module_item_info_t *power_module_item_info_deactive;
			struct list_head *head = &power_module_group_info->power_module_item_list;

			if(power_manager_channel_info != NULL) {
				if(power_manager_channel_info->status.state != POWER_MANAGER_CHANNEL_STATE_IDLE) {
					debug("channel %d stop by power module %d fault",
					      power_manager_channel_info->id, power_module_item_info->id);
					set_power_manager_channel_request_state(power_manager_info, power_manager_channel_info->id, POWER_MANAGER_CHANNEL_REQUEST_STATE_STOP);
				}
			}

			list_for_each_entry(power_module_item_info_deactive, head, power_module_item_info_t, list) {
				power_module_item_info_deactive->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;

				debug("power module %d to state %s",
				      power_module_item_info_deactive->id,
				      get_power_module_item_state_des(power_module_item_info_deactive->status.state));
			}

			list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_deactive_list);
		}
	}

	status->module_status = u_power_module_status.v;

	status->connect_state = get_power_module_connect_state(power_modules_info, power_module_item_info->id);

	if(status->connect_state == 0) {
		power_modules_start(power_modules_info, power_module_item_info->id);
	}
}

static void power_module_item_set_out_voltage_current(power_module_item_info_t *power_module_item_info, uint16_t voltage, uint16_t current)
{
	uint32_t ticks = osKernelSysTick();
	power_modules_info_t *power_modules_info = (power_modules_info_t *)power_module_item_info->power_modules_info;
	power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_module_group_info->power_manager_group_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	channels_info_t *channels_info = power_manager_info->channels_info;
	power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)power_module_group_info->power_manager_channel_info;
	u_power_module_status_t u_power_module_status;
	uint8_t power_off = 0;

	if(get_first_fault(channels_info->faults) != -1) {
		voltage = 0;
		current = 0;
	}

	if(power_manager_channel_info != NULL) {
		channel_info_t *channel_info = channels_info->channel_info + power_manager_channel_info->id;

		if(get_first_fault(channel_info->faults) != -1) {
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
		if(ticks_duration(ticks, power_module_item_info->setting_stamp) < (1 * 1000)) {
			return;
		}
	} else {
		//保存原来设置输出电流值
		power_module_item_info->status.smooth_setting_output_current = power_module_item_info->status.setting_output_current;
		power_module_item_info->status.setting_output_voltage = voltage;
		power_module_item_info->status.setting_output_current = current;
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

	//debug("set power module %d poweroff:%d, voltage:%d, current:%d",
	//      power_module_item_info->id,
	//      power_off,
	//      voltage,
	//      current);

	if(u_power_module_status.s.poweroff != power_off) {
		if(power_off == 0) {
			if(power_manager_channel_info != NULL) {
				channels_settings_t *channels_settings = &channels_info->channels_settings;
				uint32_t battery_voltage = channels_settings->module_min_output_voltage * 100;


				if(battery_voltage == 0) {
					battery_voltage = 200000;
				}

				power_modules_set_channel_id(power_modules_info, power_module_item_info->id, power_manager_channel_info->id);
				power_modules_set_battery_voltage(power_modules_info, power_module_item_info->id, battery_voltage);
			} else {
				debug("");
			}
		}

		power_modules_set_poweroff(power_modules_info, power_module_item_info->id, power_off);
	}

	if(power_off == 0) {
		power_modules_set_out_voltage_current(power_modules_info, power_module_item_info->id, voltage * 1000 / 10, current * 1000 / 10);
	} else {
		power_modules_set_out_voltage_current(power_modules_info, power_module_item_info->id, 0, 0);
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
			power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
			power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)power_module_group_info->power_manager_channel_info;
			power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_manager_channel_info->power_manager_group_info;
			power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
			channels_info_t *channels_info = power_manager_info->channels_info;
			channels_settings_t *channels_settings = &channels_info->channels_settings;
			channel_info_t *channel_info = channels_info->channel_info + power_manager_channel_info->id;

			if((channel_info->state == CHANNEL_STATE_STARTING) ||
			   (channel_info->state == CHANNEL_STATE_CHARGING)) {
				uint16_t voltage = power_manager_channel_info->status.charge_output_voltage;
				uint16_t current = 1;

				power_module_voltage_current_correction(channels_settings, &voltage, &current);
				power_module_power_limit_correction(channels_settings, &power_manager_channel_info->status.charge_output_voltage, &current);
				power_module_item_set_out_voltage_current(power_module_item_info, voltage, current);

				debug("power module %d setting voltage:%d, current:%d",
				      power_module_item_info->id,
				      voltage,
				      current);

				if(abs(power_module_item_info->status.setting_output_voltage - power_module_item_info->status.module_output_voltage) <= 300) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_READY;
					debug("power module %d to state %s",
					      power_module_item_info->id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
				} else {
					debug("power module %d setting_output_voltage:%d, module_output_voltage:%d",
					      power_module_item_info->id,
					      power_module_item_info->status.setting_output_voltage,
					      power_module_item_info->status.module_output_voltage);
				}
			} else {
				if(power_module_item_info->status.module_output_voltage >= 500) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_READY;
					debug("power module %d to state %s",
					      power_module_item_info->id,
					      get_power_module_item_state_des(power_module_item_info->status.state));
				} else {
					debug("power module %d setting_output_voltage:%d, module_output_voltage:%d",
					      power_module_item_info->id,
					      power_module_item_info->status.setting_output_voltage,
					      power_module_item_info->status.module_output_voltage);
				}
			}

		}
		break;

		case POWER_MODULE_ITEM_STATE_READY: {//达到接通条件
			power_module_group_info_t *power_module_group_info = (power_module_group_info_t *)power_module_item_info->power_module_group_info;
			power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_module_group_info->power_manager_group_info;

			if(power_manager_group_info->change_state == POWER_MANAGER_CHANGE_STATE_IDLE) {//等待配置同步完成,开关动作完成
				power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_ACTIVE;
				debug("power module %d to state %s",
				      power_module_item_info->id,
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
			//debug("power module %d require_output_voltage:%d, require_output_current:%d",
			//      power_module_item_info->id,
			//      power_module_item_info->status.require_output_voltage,
			//      power_module_item_info->status.require_output_current);
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
				debug("power module %d to state %s",
				      power_module_item_info->id,
				      get_power_module_item_state_des(power_module_item_info->status.state));
			} else {
				if(u_power_module_status.s.poweroff == 1) {
					power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_DEACTIVE;
					debug("power module %d to state %s",
					      power_module_item_info->id,
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
				debug("power module %d to state %s",
				      power_module_item_info->id,
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
					debug("poewr module %d to state %s",
					      power_module_item_info->id,
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

static void update_channels_module_input_voltage(power_manager_info_t *power_manager_info)
{
	channels_info_t *channels_info = power_manager_info->channels_info;
	uint16_t va = 0;
	uint16_t vb = 0;
	uint16_t vc = 0;
	int i;

	for(i = 0; i < power_manager_info->power_modules_info->power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + i;
		power_module_info_t *power_module_info = power_manager_info->power_modules_info->power_module_info + power_module_item_info->id;

		if(get_first_fault(power_module_item_info->faults) == -1) {
			va = power_module_info->input_aline_voltage;
			vb = power_module_info->input_bline_voltage;
			vc = power_module_info->input_cline_voltage;
			break;
		}
	}

	channels_info->va = va;
	channels_info->vb = vb;
	channels_info->vc = vc;
}

static void handle_power_module_item_info(power_manager_info_t *power_manager_info)
{
	int i;

	for(i = 0; i < power_manager_info->power_modules_info->power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + i;

		update_poewr_module_item_info_status(power_module_item_info);
		handle_power_module_item_info_state(power_module_item_info);
	}

	update_channels_module_input_voltage(power_manager_info);
}

static uint8_t is_power_module_item_info_idle(power_module_item_info_t *power_module_item_info)
{
	uint8_t idle = 0;

	switch(power_module_item_info->status.state) {
		case POWER_MODULE_ITEM_STATE_IDLE:
		case POWER_MODULE_ITEM_STATE_DISABLE: {
			idle = 1;
		}
		break;

		default: {
		}
		break;
	}

	return idle;
}

static void handle_fan_state(power_manager_info_t *power_manager_info)
{
	int i;
	GPIO_PinState state = GPIO_PIN_RESET;
	channels_info_t *channels_info = power_manager_info->channels_info;
	channels_config_t *channels_config = channels_info->channels_config;

	for(i = 0; i < power_manager_info->power_modules_info->power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + i;

		if(is_power_module_item_info_idle(power_module_item_info) == 0) {
			state = GPIO_PIN_SET;
			break;
		}
	}

	if(channels_config->fan_port != NULL) {
		HAL_GPIO_WritePin(channels_config->fan_port, channels_config->fan_pin, state);
	}
}

static void periodic(void *fn_ctx, void *chain_ctx)
{
	power_manager_info_t *power_manager_info = (power_manager_info_t *)fn_ctx;
	channels_info_t *channels_info = power_manager_info->channels_info;
	int i;

	for(i = 0; i < channels_info->channel_number; i++) {
		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;

		handle_power_manager_channel_power(power_manager_channel_info);
		handle_power_manager_channel_state(power_manager_channel_info);
	}

	for(i = 0; i < power_manager_info->power_manager_group_number; i++) {
		power_manager_group_info_t *power_manager_group_info = power_manager_info->power_manager_group_info + i;

		handle_power_manager_group_change_state(power_manager_group_info);
		handle_power_manager_group_deactive_list(power_manager_group_info);
		handle_power_manager_group_disable_list(power_manager_group_info);
	}

	handle_power_module_item_info(power_manager_info);
	update_power_manager_group_policy(power_manager_info);
	handle_fan_state(power_manager_info);
}

static int _init(void *_power_manager_info)
{
	int ret = -1;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)_power_manager_info;
	channels_info_t *channels_info = power_manager_info->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;

	power_manager_info->power_manager_ctx = os_calloc(1, sizeof(power_manager_ctx_t));
	OS_ASSERT(power_manager_info->power_manager_ctx != NULL);

	debug("set power module type %s", get_power_modules_type_des(channels_settings->power_module_settings.power_module_type));
	power_modules_set_type(power_manager_info->power_modules_info, channels_settings->power_module_settings.power_module_type);

	update_power_manager_group_policy(power_manager_info);

	power_manager_info->periodic_callback_item.fn = periodic;
	power_manager_info->periodic_callback_item.fn_ctx = power_manager_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &power_manager_info->periodic_callback_item) == 0);
	ret = 0;
	return ret;
}

static int _deinit(void *_power_manager_info)
{
	int ret = -1;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)_power_manager_info;
	channels_info_t *channels_info = power_manager_info->channels_info;

	remove_callback(channels_info->common_periodic_chain, &power_manager_info->periodic_callback_item);
	ret = 0;
	return ret;
}

power_manager_handler_t power_manager_handler_native = {
	.type = POWER_MANAGER_TYPE_NATIVE,
	.init = _init,
	.deinit = _deinit,
};
