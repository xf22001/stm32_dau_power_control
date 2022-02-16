

/*================================================================
 *
 *
 *   文件名称：channels_config.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 11时00分11秒
 *   修改日期：2022年02月16日 星期三 15时12分39秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_CONFIG_H
#define _CHANNELS_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

#define PRICE_SEGMENT_SIZE 48

typedef enum {
	CHANNEL_TYPE_NONE = 0,
	CHANNEL_TYPE_NATIVE,
	CHANNEL_TYPE_PROXY_REMOTE,
	CHANNEL_TYPE_PROXY_LOCAL,
} channel_type_t;

typedef struct {
	channel_type_t channel_type;
} channel_config_t;

typedef struct {
	uint8_t power_module_default_type;
	void *hcan;
} power_module_config_t;

typedef struct {
	void *huart;
	void *con_gpio;
	uint16_t con_pin;
	void *led_gpio;
	uint16_t led_pin;
	uint8_t station;
} display_config_t;

typedef struct {
	uint8_t proxy_channel_index;
	uint8_t channel_id;
} proxy_channel_item_t;

typedef struct {
	void *hcan;
	uint8_t proxy_channel_number;
	proxy_channel_item_t *items;
} proxy_channel_info_t;

typedef enum {
	POWER_MANAGER_TYPE_NONE = 0,
	POWER_MANAGER_TYPE_NATIVE,
} power_manager_type_t;

typedef struct {
	power_manager_type_t power_manager_default_type;
	void *hcan_relay_board;
} power_manager_config_t;

typedef struct {
	uint8_t id;
	uint8_t channel_number;
	channel_config_t **channel_config;
	power_module_config_t power_module_config;
	power_manager_config_t power_manager_config;
	display_config_t display_config;
	proxy_channel_info_t proxy_channel_info;

	void *board_temperature_adc;
	uint8_t board_temperature_adc_rank;

	void *force_stop_port;
	uint16_t force_stop_pin;
	uint16_t force_stop_normal_state;

	void *fan_port;
	uint16_t fan_pin;

	void *door_port;
	uint16_t door_pin;
	uint16_t door_normal_state;

	void *fault_port;
	uint16_t fault_pin;
} channels_config_t;

char *get_channel_config_channel_type_des(channel_type_t type);
char *get_power_manager_type_des(power_manager_type_t type);
channels_config_t *get_channels_config(uint8_t id);
proxy_channel_item_t *get_proxy_channel_item_by_proxy_channel_index(proxy_channel_info_t *proxy_channel_info, uint8_t proxy_channel_index);
proxy_channel_item_t *get_proxy_channel_item_by_channel_id(proxy_channel_info_t *proxy_channel_info, uint8_t channel_id);

#endif //_CHANNELS_CONFIG_H
