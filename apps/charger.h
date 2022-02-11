

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分24秒
 *   修改日期：2022年02月08日 星期二 16时00分44秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_H
#define _CHARGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "can_txrx.h"
#include "callback_chain.h"
#include "channels_config.h"
#include "channels.h"
#include "bms_spec.h"

typedef int (*charger_bms_state_handle_t)(void *_charger_info);

typedef struct {
	uint8_t bms_state;
	charger_bms_state_handle_t prepare;
	charger_bms_state_handle_t handle_request;
	charger_bms_state_handle_t handle_response;
} charger_bms_state_handler_t;

typedef int (*charger_bms_handle_init_t)(void *_charger_info);
typedef int (*charger_bms_handle_deinit_t)(void *_charger_info);

typedef struct {
	channel_charger_bms_type_t charger_type;
	charger_bms_handle_init_t init;
	charger_bms_handle_deinit_t deinit;
} charger_bms_handler_t;

typedef int (*charger_init_t)(void *_charger_info);

typedef enum {
	CHARGER_BMS_REQUEST_ACTION_NONE = 0,
	CHARGER_BMS_REQUEST_ACTION_START,
	CHARGER_BMS_REQUEST_ACTION_STOP,
} charger_bms_request_action_t;

typedef enum {
	CHARGER_BMS_WORK_STATE_IDLE,
	CHARGER_BMS_WORK_STATE_STARTING,
	CHARGER_BMS_WORK_STATE_RUNNING,
	CHARGER_BMS_WORK_STATE_STOPPING,
} charger_bms_work_state_t;

typedef enum {
	CHARGER_STATE_EVENT_NONE = 0,
	CHARGER_STATE_EVENT_CHARGER_CONNECTOR,
	CHARGER_STATE_EVENT_VEHICLE_CONNECTOR,
	CHARGER_STATE_EVENT_CHARGER_CONNECTOR_READY,
	CHARGER_STATE_EVENT_CHANNEL_REQUIRE_UPDATE,
	CHARGER_STATE_EVENT_BMS_GB_BRM_RECEIVED,
} charger_state_event_t;

typedef struct {
	charger_state_event_t event;
	void *ctx;
} charger_state_event_ctx_t;

typedef enum {
	CHARGER_LED_STATE_IDLE = 0,//blue blink
	CHARGER_LED_STATE_CHARGER_CONNECTED,//blue
	CHARGER_LED_STATE_CHARGING,//green blink
	CHARGER_LED_STATE_CHARGE_END,//green
} charger_led_state_t;

typedef struct {
	channel_info_t *channel_info;

	os_mutex_t handle_mutex;

	can_info_t *bms_can_info;
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;

	callback_item_t periodic_request_cb;

	charger_bms_handler_t *charger_bms_handler;
	charger_bms_state_handler_t *charger_bms_state_handler;
	uint8_t state;
	uint8_t request_state;
	uint8_t bms_restart_count;
	callback_chain_t *charger_state_event_chain;

	callback_item_t charger_state_event_notify_callback_item;
	callback_item_t vin_verify_callback_item;

	charger_bms_work_state_t charger_bms_work_state;
	charger_bms_request_action_t charger_bms_request_action;

	bms_data_t bms_data;
	multi_packets_info_t multi_packets_info;

	int8_t dc_p_temperature;
	int8_t dc_n_temperature;

	uint32_t periodic_stamps;

	uint8_t charger_connect_state;
	uint8_t charger_lock_state;
	uint8_t vehicle_relay_state;

	uint32_t adhe_p_gpio_alive_stamps;
	uint32_t adhe_n_gpio_alive_stamps;

	uint32_t led_blink_stamps;
	charger_led_state_t led_state;

	void *charger_bms_ctx;
	void *function_board_info;

	uint8_t charger_lock_action_request;
	uint8_t charger_lock_action_state;
	uint32_t charger_lock_action_stamps;
	uint8_t auxiliary_power_on_action_state;
	uint32_t auxiliary_power_on_action_stamps;
	uint8_t discharge_action_request;
	uint8_t discharge_action_state;
	uint32_t discharge_action_stamps;
	uint8_t precharge_action_state;
	uint32_t precharge_action_stamps;
	uint8_t insulation_detect_action_state;
	uint32_t insulation_detect_action_stamps;
	uint8_t battery_voltage_confirm_action_state;
	uint32_t battery_voltage_confirm_action_stamps;
	uint8_t wait_no_current_state;
	uint32_t wait_no_current_stamps;

	uint8_t output_relay_state;

	uint8_t vin_verify_enable;
} charger_info_t;

void alloc_charger_info(channel_info_t *channel_info);

#endif //_CHARGER_H
