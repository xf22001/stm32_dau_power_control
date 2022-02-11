

/*================================================================
 *
 *
 *   文件名称：channels_config.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 11时00分11秒
 *   修改日期：2022年02月11日 星期五 20时11分07秒
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

typedef enum {
	CHANNEL_CHARGER_BMS_TYPE_NONE = 0,
	CHANNEL_CHARGER_BMS_TYPE_GB,
	CHANNEL_CHARGER_BMS_TYPE_AC,
	CHANNEL_CHARGER_BMS_TYPE_NOBMS,
	CHANNEL_CHARGER_BMS_TYPE_CUSTOM,
} channel_charger_bms_type_t;

typedef enum {
	FUNCTION_BOARD_TYPE_NONE = 0,
	FUNCTION_BOARD_TYPE_485,
	FUNCTION_BOARD_TYPE_MODBUS,
	FUNCTION_BOARD_TYPE_V5,
} function_board_type_t;

typedef struct {
	function_board_type_t type;
	void *huart;
	void *con_gpio;
	uint16_t con_pin;
	void *charge_voltage_adc;
	uint8_t charge_voltage_adc_rank;
	void *battery_voltage_adc;
	uint8_t battery_voltage_adc_rank;
	void *insulation_voltage_adc;
	uint8_t insulation_voltage_adc_rank;
	void *insulation_k1_gpio;
	uint16_t insulation_k1_pin;
	void *insulation_k3_gpio;
	uint16_t insulation_k3_pin;
	void *discharge_gpio;
	uint16_t discharge_pin;
} function_board_config_item_t;

typedef struct {
	function_board_type_t default_type;
	uint8_t size;
	function_board_config_item_t **items;
} function_board_config_t;

typedef struct {
	channel_charger_bms_type_t charger_type;

	void *relay_charger_lock_p_gpio;
	uint16_t relay_charger_lock_p_pin;
	void *relay_charger_lock_n_gpio;
	uint16_t relay_charger_lock_n_pin;
	void *charger_lock_state_gpio;
	uint16_t charger_lock_state_pin;
	void *charger_auxiliary_power_choose_gpio;
	uint16_t charger_auxiliary_power_choose_pin;
	void *charger_auxiliary_power_onoff_gpio;
	uint16_t charger_auxiliary_power_onoff_pin;

	//ac
	void *cp_pwm_timer;
	uint32_t cp_pwm_channel;

	void *kl_gpio;
	uint16_t kl_pin;
	void *kn_gpio;
	uint16_t kn_pin;
	void *rey3_gpio;
	uint16_t rey3_pin;
	void *rey4_gpio;
	uint16_t rey4_pin;

	void *led_r_gpio;
	uint16_t led_r_pin;
	void *led_g_gpio;
	uint16_t led_g_pin;
	void *led_b_gpio;
	uint16_t led_b_pin;

	//dc
	void *hcan_bms;
	void *output_relay_gpio;
	uint16_t output_relay_pin;
} channel_charger_config_t;

typedef enum {
	ENERGY_METER_TYPE_NONE = 0,
	ENERGY_METER_TYPE_AC,
	ENERGY_METER_TYPE_AC_HLW8032,
	ENERGY_METER_TYPE_DC,
	ENERGY_METER_TYPE_AC_SDM_220,
	ENERGY_METER_TYPE_AC_SDM_630,
	ENERGY_METER_TYPE_PROXY,
} energy_meter_type_t;

typedef struct {
	energy_meter_type_t type;
	void *huart;
	void *con_gpio;
	uint16_t con_pin;
} energy_meter_config_item_t;

typedef struct {
	energy_meter_type_t default_type;
	uint8_t size;
	energy_meter_config_item_t **items;
} energy_meter_config_t;

typedef struct {
	channel_type_t channel_type;
	channel_charger_config_t charger_config;
	energy_meter_config_t energy_meter_config;
	function_board_config_t function_board_config;
	void *charger_temperature_p_adc;
	uint8_t charger_temperature_p_adc_rank;
	void *charger_temperature_n_adc;
	uint8_t charger_temperature_n_adc_rank;
	void *adhe_ad_adc;
	uint8_t adhe_ad_adc_rank;
	void *adhe_p_gpio;
	uint16_t adhe_p_pin;
	void *adhe_n_gpio;
	uint16_t adhe_n_pin;
	void *cp_ad_adc;
	uint8_t cp_ad_adc_rank;

	void *auxilary_select_gpio;
	uint16_t auxilary_select_pin;
	void *auxilary_output_gpio;
	uint16_t auxilary_output_pin;
} channel_config_t;

typedef struct {
	uint8_t power_module_default_type;
	uint8_t power_module_number;
	void *hcan;
} power_module_config_t;

typedef struct {
	void *data_port;
	uint16_t data_pin;
	void *cs_port;
	uint16_t cs_pin;
	void *clk_port;
	uint16_t clk_pin;
} voice_config_t;

typedef enum {
	CARD_READER_TYPE_PSEUDO = 0,
	CARD_READER_TYPE_MT_318_626,
	CARD_READER_TYPE_MT_318_628,
	CARD_READER_TYPE_ZLG,
} card_reader_type_t;

typedef struct {
	card_reader_type_t type;
	void *huart;
} card_reader_config_item_t;

typedef struct {
	card_reader_type_t default_type;
	uint8_t size;
	card_reader_config_item_t **items;
} card_reader_config_t;

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
} power_manager_config_t;

typedef struct {
	uint8_t id;
	uint8_t channel_number;
	channel_config_t **channel_config;
	power_module_config_t power_module_config;
	power_manager_config_t power_manager_config;
	voice_config_t voice_config;
	card_reader_config_t card_reader_config;
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

	void *pe_detect_port;
	uint16_t pe_detect_pin;

	void *electric_leakage_detect_cal_port;
	uint16_t electric_leakage_detect_cal_pin;
	void *electric_leakage_detect_test_port;
	uint16_t electric_leakage_detect_test_pin;
	void *electric_leakage_detect_trip_port;
	uint16_t electric_leakage_detect_trip_pin;
} channels_config_t;

char *get_channel_config_channel_type_des(channel_type_t type);
char *get_channel_config_charger_bms_type_des(channel_charger_bms_type_t type);
char *get_channel_config_energy_meter_type_des(energy_meter_type_t type);
char *get_channel_config_function_board_type_des(function_board_type_t type);
char *get_card_reader_type_des(card_reader_type_t type);
char *get_power_manager_type_des(power_manager_type_t type);
channels_config_t *get_channels_config(uint8_t id);
card_reader_config_item_t *get_card_reader_config_item(channels_config_t *channels_config, card_reader_type_t type);
proxy_channel_item_t *get_proxy_channel_item_by_proxy_channel_index(proxy_channel_info_t *proxy_channel_info, uint8_t proxy_channel_index);
proxy_channel_item_t *get_proxy_channel_item_by_channel_id(proxy_channel_info_t *proxy_channel_info, uint8_t channel_id);

#endif //_CHANNELS_CONFIG_H
