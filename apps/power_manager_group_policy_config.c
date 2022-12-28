

/*================================================================
 *
 *
 *   文件名称：power_manager_group_policy_config.c
 *   创 建 者：肖飞
 *   创建日期：2022年07月22日 星期五 12时30分44秒
 *   修改日期：2022年12月28日 星期三 16时05分22秒
 *   描    述：
 *
 *================================================================*/
#include "power_manager_group_policy_chain.h"

#include "power_manager.h"
#include "main.h"

#include "log.h"

void power_manager_restore_config(channels_info_t *channels_info)
{
	int i;
	int j;

	channels_config_t *channels_config = channels_info->channels_config;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	power_manager_settings_t *power_manager_settings = &channels_settings->power_manager_settings;

	channels_settings->power_module_settings.power_module_type = channels_config->power_module_config.power_module_default_type;
	channels_settings->power_module_settings.rate_current = 21;
	channels_settings->power_manager_settings.type = channels_config->power_manager_config.power_manager_default_type;
	channels_settings->module_max_output_voltage = 10000;
	channels_settings->module_min_output_voltage = 500;
	channels_settings->module_max_output_current = 670;
	channels_settings->module_min_output_current = 5;
	channels_settings->module_max_input_voltage = 0;//2530;
	channels_settings->module_min_input_voltage = 0;//1870;
	channels_settings->module_max_output_power = 20000;
	channels_settings->channels_max_output_power = 4800000;
	channels_settings->power_manager_group_policy = POWER_MANAGER_GROUP_POLICY_PRIORITY;

	power_manager_settings->power_manager_group_number = 2;

	debug("power_manager_group_number:%d", power_manager_settings->power_manager_group_number);

	for(i = 0; i < power_manager_settings->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];

		power_manager_group_settings->channel_number = 6;
		power_manager_group_settings->relay_board_number_per_channel = 1;

		for(j = 0; j < power_manager_group_settings->relay_board_number_per_channel; j++) {
			power_manager_group_settings->slot_per_relay_board[j] = 6;
		}

		power_manager_group_settings->power_module_group_number = 6;

		channels_info->channel_number += power_manager_group_settings->channel_number;

		for(j = 0; j < power_manager_group_settings->power_module_group_number; j++) {
			power_module_group_settings_t *power_module_group_settings = &power_manager_group_settings->power_module_group_settings[j];
			power_module_group_settings->power_module_number = 2;
		}
	}
}

void channel_info_reset_default_config(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;
	channel_settings_t *channel_settings = &channel_info->channel_settings;

	memset(channel_settings, 0, sizeof(channel_settings_t));

	channel_settings->channel_type = channel_config->channel_type;

	channel_settings->max_output_power = 4800000;

	channel_settings->max_output_voltage = 10000;
	channel_settings->min_output_voltage = 500;
	channel_settings->max_output_current = 2500;
	channel_settings->min_output_current = 5;
}

static uint32_t power_supply_alive_stamp_1;
static uint32_t power_supply_alive_stamp_2;

void update_channel_charger_connected_state(uint8_t channel_id)
{
	if((channel_id >= 0) && (channel_id <= 5)) {
		power_supply_alive_stamp_1 = osKernelSysTick();
	} else if((channel_id >= 6) && (channel_id <= 11)) {
		power_supply_alive_stamp_2 = osKernelSysTick();
	}
}

void handle_channel_charger_connected_state(void)
{
	uint32_t ticks = osKernelSysTick();
	GPIO_PinState power_supply;

	power_supply = GPIO_PIN_SET;

	if(ticks_duration(ticks, power_supply_alive_stamp_1) > 5 * 1000) {
		power_supply = GPIO_PIN_RESET;
	}

	HAL_GPIO_WritePin(relay_4_GPIO_Port, relay_4_Pin, power_supply);

	power_supply = GPIO_PIN_SET;

	if(ticks_duration(ticks, power_supply_alive_stamp_2) > 5 * 1000) {
		power_supply = GPIO_PIN_RESET;
	}

	HAL_GPIO_WritePin(out_7_GPIO_Port, out_7_Pin, power_supply);
}
