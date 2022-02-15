

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 10时08分44秒
 *   修改日期：2022年02月15日 星期二 14时18分35秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_H
#define _CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "storage.h"
#include "event_helper.h"
#include "channels_config.h"

#include "callback_chain.h"
#include "bitmap_ops.h"
#include "channel_record.h"

#include "display_cache.h"

#define CHANNEL_TASK_PERIODIC (10)

//for channel event
typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
	CHANNEL_EVENT_TYPE_START_CHANNEL,
	CHANNEL_EVENT_TYPE_STOP_CHANNEL,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	uint8_t type;//channel_event_type_t
	void *ctx;
} channel_event_t;

//all channels event type
typedef enum {
	CHANNELS_EVENT_UNKNOW = 0,
	CHANNELS_EVENT_INSULATION,
	CHANNELS_EVENT_TELEMETER,
	CHANNELS_EVENT_CARD_READER,
	CHANNELS_EVENT_DISPLAY,
	CHANNELS_EVENT_CHANNEL,
} channels_event_type_t;

//all channels event typedef
typedef struct {
	channels_event_type_t type;
	void *event;
} channels_event_t;

typedef int (*channel_init_t)(void *_channel_info);
typedef int (*channel_deinit_t)(void *_channel_info);

typedef struct {
	channel_type_t channel_type;
	channel_init_t init;
	channel_deinit_t deinit;
} channel_handler_t;

typedef enum {
	CHANNEL_STATE_NONE = 0,
	CHANNEL_STATE_IDLE,
	CHANNEL_STATE_START,
	CHANNEL_STATE_WAITING,
	CHANNEL_STATE_STARTING,
	CHANNEL_STATE_CHARGING,
	CHANNEL_STATE_STOP,
	CHANNEL_STATE_STOPPING,
	CHANNEL_STATE_END,
	CHANNEL_STATE_CLEANUP,
} channel_state_t;

typedef enum {
	CHANNEL_FAULT_FAULT = 0,
	CHANNEL_FAULT_CONNECT_TIMEOUT,
	CHANNEL_FAULT_RELAY_BOARD_OVER_TEMPERATURE,
	CHANNEL_FAULT_SIZE,
} channel_fault_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t charger_type;//channel_charger_bms_type_t
} charger_settings_t;

typedef struct {
	uint8_t channel_type;//channel_type_t
	charger_settings_t charger_settings;

	uint8_t ac_current_limit;//ac_current_limit_t

	uint32_t max_output_power;//0.0001kw
	uint32_t max_output_voltage;//0.1v
	uint16_t min_output_voltage;//0.1v
	uint32_t max_output_current;//0.1a
	uint16_t min_output_current;//0.1a
} channel_settings_t;

#pragma pack(pop)

typedef struct {
	uint8_t start_reason;//channel_record_item_start_reason_t
	uint8_t charge_mode;//channel_record_charge_mode_t

	uint64_t card_id;

	uint8_t serial_no[32];

	uint8_t account[32];
	uint8_t password[32];
	uint32_t account_balance;//0.01

	time_t charge_duration;//设置充电时长
	uint32_t charge_amount;//设置充电金额
	uint32_t charge_energy;//设置充电电量

	time_t start_time;
} channel_event_start_t;

typedef struct {
	uint8_t stop_reason;//channel_record_item_stop_reason_t
} channel_event_stop_t;

typedef struct {
	channel_config_t *channel_config;
	uint8_t channel_id;
	void *channels_info;
	channel_handler_t *channel_handler;

	channel_settings_t channel_settings;
	uint8_t channel_settings_invalid;
	display_cache_channel_t display_cache_channel;

	bitmap_t *faults;//channel_fault_t

	uint8_t request_start_state;
	uint8_t request_charging_state;
	uint8_t request_end_state;
	uint8_t request_idle_state;
	channel_state_t request_state;
	channel_state_t state;

	uint8_t charger_lock_state;

	uint8_t bms_state;

	callback_chain_t *channel_periodic_chain;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	callback_chain_t *start_chain;
	callback_chain_t *end_chain;
	callback_chain_t *state_changed_chain;
	callback_chain_t *request_stop_chain;
	callback_chain_t *power_manager_channel_request_state_chain;
	callback_chain_t *bms_auto_start_chain;

	callback_item_t channel_start_callback_item;
	callback_item_t channel_end_callback_item;
	callback_item_t channel_state_changed_callback_item;
	callback_item_t power_manager_channel_request_state_callback_item;
	callback_item_t bms_auto_start_callback_item;

	//channel record cb
	void *channel_record_task_info;
	callback_item_t channel_record_sync_callback_item;

	callback_item_t display_data_action_callback_item;
	callback_item_t display_data_invalid_callback_item;
	callback_item_t display_data_changed_callback_item;

	void *charger_info;
	void *energy_meter_info;

	uint16_t temperature_p_ad;
	int16_t temperature_p;
	uint16_t temperature_n_ad;
	int16_t temperature_n;
	uint16_t cp_ad;
	uint16_t cp_ad_voltage;
	uint16_t adhe_ad;
	uint16_t adhe_ad_voltage;

	uint16_t cp_pwm_duty;

	uint32_t total_energy;//0.0001kwh==0.1wh
	uint32_t total_energy_base;//0.0001kwh
	uint32_t voltage;//0.1v
	uint32_t current;//0.1a
	uint32_t power;//0.1w

	uint16_t va;
	uint16_t vb;
	uint16_t vc;
	uint16_t ca;
	uint16_t cb;
	uint16_t cc;

	channel_record_item_t channel_record_item;

	channel_event_start_t *channel_event_start;
	channel_event_start_t channel_event_start_display;
	channel_event_start_t channel_event_start_net_client;
	channel_event_start_t channel_event_start_bms;

	channel_event_stop_t channel_event_stop;

	uint32_t require_voltage;//0.1v
	uint32_t require_current;//0.1v
	uint8_t power_manager_channel_state;//power_manager_channel_state_t

	uint8_t pre_channels_fault;
	uint8_t pre_channel_fault;
	uint32_t pre_channel_fault_stamps;
} channel_info_t;

#pragma pack(push, 1)

typedef struct {
	uint32_t price[PRICE_SEGMENT_SIZE];//0.0001
} price_info_t;

typedef struct {
	uint8_t power_module_type;//power_module_type_t
	uint16_t rate_current;//华为模块参考电流 a
} power_module_settings_t;

typedef struct {
	uint8_t power_module_number;
} power_module_group_settings_t;

//每功率管理组通道数
#if !defined(DEFAULT_POWER_MANAGER_GROUP_CHANNEL_NUMBER)
#define DEFAULT_POWER_MANAGER_GROUP_CHANNEL_NUMBER 6
#endif

//功率管理组内每通道dau模块数
#if !defined(RELAY_BOARD_NUMBER_PER_CHANNEL_MAX)
#define RELAY_BOARD_NUMBER_PER_CHANNEL_MAX 1
#endif

//每功率管理组电源模块组数
#if !defined(POWER_MODULE_GROUP_MAX_SIZE)
#define POWER_MODULE_GROUP_MAX_SIZE 8
#endif

typedef struct {
	uint8_t power_manager_type;//power_manager_type_t
	uint8_t channel_number;
	uint8_t relay_board_number_per_channel;
	uint8_t slot_per_relay_board[RELAY_BOARD_NUMBER_PER_CHANNEL_MAX];
	uint8_t power_module_group_number;
	power_module_group_settings_t power_module_group_settings[POWER_MODULE_GROUP_MAX_SIZE];
} power_manager_group_settings_t;

//功率管理组数
#if !defined(POWER_MANAGER_GROUP_MAX_SIZE)
#define POWER_MANAGER_GROUP_MAX_SIZE 1
#endif

typedef struct {
	uint8_t type;//power_manager_type_t
	uint8_t power_manager_group_number;
	power_manager_group_settings_t power_manager_group_settings[POWER_MANAGER_GROUP_MAX_SIZE];
} power_manager_settings_t;

typedef struct {
	uint8_t type;//card_reader_type_t
} card_reader_settings_t;

typedef struct {
	uint8_t authorize;
	uint8_t precharge_enable;
	power_module_settings_t power_module_settings;
	power_manager_settings_t power_manager_settings;
	card_reader_settings_t card_reader_settings;

	uint16_t module_max_output_voltage;//最大输出电压 0.1v
	uint16_t module_min_output_voltage;//最小输出电压 0.1v
	uint16_t module_max_output_current;//最大输出电流 0.1a
	uint16_t module_min_output_current;//最小输出电流 0.1a
	uint16_t module_max_input_voltage;//最大输入电压 0.1v
	uint16_t module_min_input_voltage;//最小输入电压 0.1v
	uint16_t module_max_output_power;//单模块最大功率 w
	uint32_t channels_max_output_power;//柜体最大功率 w
	uint8_t power_manager_group_policy;

	uint8_t noload_protect_disable;
	uint8_t soc_threshold;
	uint16_t power_threshold;//单位 0.1kW
	uint8_t magnification;//电表放大倍率.0:2位小数, 1:3位小数
	price_info_t price_info_energy;
	price_info_t price_info_service;
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元

	uint8_t pe_detect_disable;
} channels_settings_t;

#pragma pack(pop)

typedef enum {
	CHANNELS_FAULT_RELAY_CHECK = 0,
	CHANNELS_FAULT_FORCE_STOP,
	CHANNELS_FAULT_DOOR,
	CHANNELS_FAULT_DISPLAY,
	CHANNELS_FAULT_SIZE,
} channels_fault_t;

typedef enum {
	ELECTRIC_LEAKAGE_DETECT_TYPE_UNKNOW = 0,
	ELECTRIC_LEAKAGE_DETECT_TYPE_A,
	ELECTRIC_LEAKAGE_DETECT_TYPE_B,
	ELECTRIC_LEAKAGE_DETECT_TYPE_C,
} electric_leakage_detect_type_t;

typedef enum {
	ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_PREPARE = 0,//cal to high, test to low, trip to low
	ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_START,//cal keep high >= 100ms, then cal to low
	ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_PREPARE,//cal 100ms >= keep >= 50ms, then cal to high
	ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_START,//test keep low >=500ms, then to high
	ELECTRIC_LEAKAGE_DETECT_B_STATE_WAIT_TRIP,//test keep high 400ms, then to low, wait trip;
	ELECTRIC_LEAKAGE_DETECT_B_STATE_PREPARE_DETECT,//wait trip to low = 100ms
	ELECTRIC_LEAKAGE_DETECT_B_STATE_DETECT,//wait trip
	ELECTRIC_LEAKAGE_DETECT_B_STATE_ERROR,
} electric_leakage_detect_b_state_t;

typedef enum {
	CHANNELS_NOTIFY_NONE = 0,
	CHANNELS_NOTIFY_CHANNEL_STATE_CHANGE,
	CHANNELS_NOTIFY_CHANNEL_CHARGER_STATE_EVENT,
	CHANNELS_NOTIFY_CARD_READER_START,
	CHANNELS_NOTIFY_CARD_READER_GET_CARD,
	CHANNELS_NOTIFY_CARD_READER_MARK_INVALID,
	CHANNELS_NOTIFY_CARD_VERIFY_RESULT,
	CHANNELS_NOTIFY_VIN_VERIFY_RESULT,
	CHANNELS_NOTIFY_SELECT_CHARGE_TYPE,
} channels_notify_t;

typedef struct {
	channels_notify_t notify;
	void *ctx;
} channels_notify_ctx_t;

typedef struct {
	channels_config_t *channels_config;
	storage_info_t *storage_info;

	event_pool_t *event_pool;
	callback_chain_t *common_periodic_chain;
	callback_chain_t *common_event_chain;
	callback_chain_t *channels_notify_chain;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	callback_item_t display_data_action_callback_item;
	callback_item_t display_data_invalid_callback_item;
	callback_item_t display_data_changed_callback_item;

	uint8_t configed;

	uint32_t periodic_stamp;

	uint8_t channel_number;
	uint8_t relay_board_number;
	uint8_t power_module_number;

	channel_info_t *channel_info;
	void *card_reader_info;
	void *display_info;
	void *voice_info;
	channels_settings_t channels_settings;
	uint8_t channels_settings_invalid;
	uint8_t update_channels_settings;
	display_cache_channels_t display_cache_channels;
	bitmap_t *faults;//channels_fault_t
	void *channel_comm_channel_info;
	void *channel_comm_channels_info;
	uint16_t temperature_ad;
	int8_t temperature;

	uint8_t electric_leakage_detect_type;//electric_leakage_detect_type_t;
	uint8_t electric_leakage_detect_b_state;//electric_leakage_detect_b_state_t
	uint32_t electric_leakage_detect_b_stamps;

	void *channels_comm_proxy_ctx;
	void *relay_boards_comm_proxy_ctx;
	void *power_manager_info;
} channels_info_t;

char *get_channel_event_type_des(channel_event_type_t type);
char *get_channels_event_type_des(channels_event_type_t type);
int set_fault(bitmap_t *faults, int fault, uint8_t v);
int get_fault(bitmap_t *faults, int fault);
int get_first_fault(bitmap_t *faults);
int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout);
int channels_info_load_config(channels_info_t *channels_info);
int channels_info_save_config(channels_info_t *channels_info);
void channels_modbus_data_action(void *fn_ctx, void *chain_ctx);
void load_channels_display_cache(channels_info_t *channels_info);
void sync_channels_display_cache(channels_info_t *channels_info);
channels_info_t *start_channels(void);
channels_info_t *get_channels(void);

#endif //_CHANNELS_H
