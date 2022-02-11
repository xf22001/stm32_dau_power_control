

/*================================================================
 *
 *
 *   文件名称：channels_config.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 09时26分44秒
 *   修改日期：2022年02月11日 星期五 16时29分02秒
 *   描    述：
 *
 *================================================================*/
#include "channels_config.h"
#include "os_utils.h"
#include "power_modules.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
extern SPI_HandleTypeDef hspi3;

char *get_channel_config_channel_type_des(channel_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNEL_TYPE_NONE);
			add_des_case(CHANNEL_TYPE_NATIVE);
			add_des_case(CHANNEL_TYPE_PROXY_REMOTE);
			add_des_case(CHANNEL_TYPE_PROXY_LOCAL);

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_config_charger_bms_type_des(channel_charger_bms_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNEL_CHARGER_BMS_TYPE_NONE);
			add_des_case(CHANNEL_CHARGER_BMS_TYPE_GB);
			add_des_case(CHANNEL_CHARGER_BMS_TYPE_AC);
			add_des_case(CHANNEL_CHARGER_BMS_TYPE_NOBMS);
			add_des_case(CHANNEL_CHARGER_BMS_TYPE_CUSTOM);

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_config_energy_meter_type_des(energy_meter_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(ENERGY_METER_TYPE_NONE);
			add_des_case(ENERGY_METER_TYPE_AC);
			add_des_case(ENERGY_METER_TYPE_AC_HLW8032);
			add_des_case(ENERGY_METER_TYPE_DC);
			add_des_case(ENERGY_METER_TYPE_AC_SDM_220);
			add_des_case(ENERGY_METER_TYPE_AC_SDM_630);
			add_des_case(ENERGY_METER_TYPE_PROXY);

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_config_function_board_type_des(function_board_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(FUNCTION_BOARD_TYPE_NONE);
			add_des_case(FUNCTION_BOARD_TYPE_485);
			add_des_case(FUNCTION_BOARD_TYPE_MODBUS);

		default: {
		}
		break;
	}

	return des;
}

char *get_power_manager_type_des(power_manager_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(POWER_MANAGER_TYPE_NONE);
			add_des_case(POWER_MANAGER_TYPE_NATIVE);

		default: {
		}
		break;
	}

	return des;
}

static channel_config_t channel0_config = {
	.channel_type = CHANNEL_TYPE_PROXY_LOCAL,
	.charger_config = {
		.charger_type = CHANNEL_CHARGER_BMS_TYPE_NOBMS,
	},
};

static channel_config_t channel1_config = {
	.channel_type = CHANNEL_TYPE_PROXY_REMOTE,
};

static channel_config_t channel2_config = {
	.channel_type = CHANNEL_TYPE_PROXY_REMOTE,
};

static channel_config_t channel3_config = {
	.channel_type = CHANNEL_TYPE_PROXY_REMOTE,
};

static channel_config_t *channel_config_sz[] = {
	&channel0_config,
	&channel0_config,
	&channel0_config,
	&channel0_config,
	&channel0_config,
	&channel0_config,
};

static channels_config_t channels_config_0 = {
	.id = 0,
	.channel_number = ARRAY_SIZE(channel_config_sz),
	.channel_config = channel_config_sz,
	.power_module_config = {
		.power_module_number = 0,
		.hcan = &hcan1,
		.power_module_default_type = POWER_MODULE_TYPE_PSEUDO,
	},
	.power_manager_config = {
		.power_manager_default_type = POWER_MANAGER_TYPE_NATIVE,
	},
	.display_config = {
		//.station = 1,
		//.huart = &huart6,
	},
	.proxy_channel_info = {
		.hcan = &hcan1,
	},
};

static channels_config_t *channels_config_sz[] = {
	&channels_config_0,
};

channels_config_t *get_channels_config(uint8_t id)
{
	int i;
	channels_config_t *channels_config = NULL;
	channels_config_t *channels_config_item = NULL;

	for(i = 0; i < ARRAY_SIZE(channels_config_sz); i++) {
		channels_config_item = channels_config_sz[i];

		if(channels_config_item->id == id) {
			channels_config = channels_config_item;
			break;
		}
	}

	return channels_config;
}

proxy_channel_item_t *get_proxy_channel_item_by_proxy_channel_index(proxy_channel_info_t *proxy_channel_info, uint8_t proxy_channel_index)
{
	proxy_channel_item_t *item = NULL;
	int i;

	if(proxy_channel_info->proxy_channel_number == 0) {
		return item;
	}

	if(proxy_channel_info->items == NULL) {
		return item;
	}

	for(i = 0; i < proxy_channel_info->proxy_channel_number; i++) {
		proxy_channel_item_t *_item = proxy_channel_info->items + i;

		if(_item->proxy_channel_index == proxy_channel_index) {
			item = _item;
			break;
		}
	}

	return item;
}

proxy_channel_item_t *get_proxy_channel_item_by_channel_id(proxy_channel_info_t *proxy_channel_info, uint8_t channel_id)
{
	proxy_channel_item_t *item = NULL;
	int i;

	if(proxy_channel_info->proxy_channel_number == 0) {
		return item;
	}

	if(proxy_channel_info->items == NULL) {
		return item;
	}

	for(i = 0; i < proxy_channel_info->proxy_channel_number; i++) {
		proxy_channel_item_t *_item = proxy_channel_info->items + i;

		if(_item->channel_id == channel_id) {
			item = _item;
			break;
		}
	}

	return item;
}
