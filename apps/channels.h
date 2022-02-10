

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2020年06月18日 星期四 09时23分40秒
 *   修改日期：2022年02月10日 星期四 11时39分27秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_H
#define _CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "event_helper.h"
#include "list_utils.h"
#include "channels_config.h"
#include "pdu_config.h"
#include "storage.h"
#include "bitmap_ops.h"
#include "callback_chain.h"
#include "display_cache.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_TASK_PERIODIC (50)

typedef enum {
	CHANNELS_CHANGE_STATE_IDLE = 0,
	CHANNELS_CHANGE_STATE_MODULE_PREPARE_FREE,
	CHANNELS_CHANGE_STATE_MODULE_FREE,
	CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG,
	CHANNELS_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC,
	CHANNELS_CHANGE_STATE_MODULE_ASSIGN,
	CHANNELS_CHANGE_STATE_MODULE_READY_SYNC,
	CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG,
	CHANNELS_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC,
} channels_change_state_t;

typedef enum {
	PDU_GROUP_FAULT_RELAY_BOARD = 0,
	PDU_GROUP_FAULT_SIZE,
} pdu_group_fault_t;

typedef struct {
	uint8_t pdu_group_id;

	uint8_t channel_number;
	uint8_t power_module_group_number;

	struct list_head channel_idle_list;//枪空闲
	struct list_head channel_active_list;//枪使用中
	struct list_head channel_deactive_list;//枪停用中
	struct list_head channel_disable_list;//枪故障禁用

	struct list_head power_module_group_idle_list;//电源模块组空闲列表
	struct list_head power_module_group_deactive_list;//电源模块组执行关断列表,表示相应模块组正在关断，各枪需要等待关断完成
	struct list_head power_module_group_disable_list;//电源模块组禁用列表

	channels_change_state_t channels_change_state;//通道切换状态

	bitmap_t *faults;//pdu_group_fault_t

	void *channels_info;
} pdu_group_info_t;//pdu分组信息

typedef enum {
	CHANNEL_STATE_IDLE = 0,
	CHANNEL_STATE_PREPARE_START,
	CHANNEL_STATE_START,
	CHANNEL_STATE_RUNNING,
	CHANNEL_STATE_PREPARE_STOP,
	CHANNEL_STATE_STOP,
	CHANNEL_STATE_RELAY_CHECK,
} channel_state_t;

typedef struct {
	channel_state_t state;

	//需求//需求电压或电流为0,关机
	uint32_t require_output_stamp;
	uint32_t module_sync_stamp;
	uint16_t require_output_voltage;//通道需求电压
	uint16_t require_output_current;//通道需求电流

	uint8_t require_work_state;//桩上报状态

	uint8_t reassign;//快充模式需要二次重分配模块
	uint8_t reassign_module_group_number;//快充模式当前分配任务模块组需求

	//充电机输出
	uint32_t charge_output_stamp;
	uint16_t charge_output_voltage;//通道输出电压0.1v
	uint16_t charge_output_current;//通道输出电流0.1a -400

	uint8_t module_ready_notify;

	uint16_t connect_state;
	uint16_t connect_state_stamp;
} channel_status_t;

typedef enum {
	CHANNEL_REQUEST_STATE_NONE = 0,
	CHANNEL_REQUEST_STATE_START,
	CHANNEL_REQUEST_STATE_STOP,
	CHANNEL_REQUEST_STATE_RELAY_CHECK,
} channel_request_state_t;

typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
	CHANNEL_EVENT_TYPE_START_CHANNEL,
	CHANNEL_EVENT_TYPE_STOP_CHANNEL,
	CHANNEL_EVENT_TYPE_RELAY_CHECK,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	channel_event_type_t type;
} channel_event_t;

typedef enum {
	CHANNELS_EVENT_CHANNEL_UNKNOW = 0,
	CHANNELS_EVENT_CHANNEL_EVENT,
} channels_event_type_t;

typedef struct {
	channels_event_type_t type;
	void *event;
} channels_event_t;

typedef int (*handle_channel_event_t)(void *channel_info, channel_event_t *channel_event);
typedef void (*handle_channel_periodic_t)(void *channel_info);

typedef struct {
	handle_channel_event_t handle_channel_event;
	handle_channel_periodic_t handle_channel_periodic;
} channel_callback_t;

typedef enum {
	RELAY_BOARD_ITEM_FAULT_FAULT = 0,
	RELAY_BOARD_ITEM_FAULT_CONNECT_TIMEOUT,
	RELAY_BOARD_ITEM_FAULT_OVER_TEMPERATURE,
	RELAY_BOARD_ITEM_FAULT_SIZE,
} relay_board_item_fault_t;

typedef struct {
	struct list_head list;//关联的开关板列表
	int8_t board_id;
	uint8_t channel_id;
	void *relay_boards_com_info;
	uint8_t offset;
	uint8_t number;
	uint8_t config;

	uint8_t remote_config;
	int16_t temperature1;
	int16_t temperature2;

	uint16_t connect_state;
	uint32_t update_stamp;

	bitmap_t *faults;//relay_board_item_fault_t
} relay_board_item_info_t;

typedef enum {
	CHANNEL_FAULT_FAULT = 0,
	CHANNEL_FAULT_CONNECT_TIMEOUT,
	CHANNEL_FAULT_POWER_MODULE,
	CHANNEL_FAULT_RELAY_BOARD_OVER_TEMPERATURE,
	CHANNEL_FAULT_SIZE,
} channel_fault_t;

typedef struct {
	struct list_head list;
	uint8_t channel_id;
	void *channels_info;
	pdu_group_info_t *pdu_group_info;
	channel_status_t status;
	channel_request_state_t channel_request_state;
	callback_item_t common_event_callback_item;
	callback_item_t common_periodic_callback_item;
	uint32_t channel_state_change_stamp;
	uint8_t relay_board_number;
	struct list_head relay_board_item_list;//关联的开关板列表
	struct list_head power_module_group_list;//关联的电源模块组
	bitmap_t *faults;//channel_fault_t
	uint32_t channel_max_output_power;
} channel_info_t;

typedef enum {
	POWER_MODULE_ITEM_STATE_IDLE = 0,
	POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE,//准备接通
	POWER_MODULE_ITEM_STATE_READY,//达到接通条件
	POWER_MODULE_ITEM_STATE_ACTIVE,//接通
	POWER_MODULE_ITEM_STATE_ACTIVE_RELAY_CHECK,//自检供电
	POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE,//准备关断
	POWER_MODULE_ITEM_STATE_DEACTIVE,//关断
	POWER_MODULE_ITEM_STATE_DISABLE,//禁用
} power_module_item_state_t;//状态要与开关板同步

typedef struct {
	power_module_item_state_t state;

	//模块需求
	uint16_t require_output_voltage;//电源模块需求电压0.1v
	uint16_t require_output_current;//电源模块需求电流0.1a

	//模块设置输出
	uint16_t setting_output_voltage;//电源模块需求电压0.1v
	uint16_t setting_output_current;//电源模块需求电流0.1a

	uint16_t smooth_setting_output_current;//平滑电源模块需求电流0.1a

	//自检输出电压
	uint16_t require_relay_check_voltage;//电源模块需求电压0.1v
	uint16_t require_relay_check_current;//电源模块需求电流0.1a

	//模块输出
	uint16_t module_output_voltage;//电源模块输出电压0.1v
	uint16_t module_output_current;//电源模块输出电流0.1a

	uint16_t module_status;
	uint16_t connect_state;
} power_module_item_status_t;

typedef enum {
	POWER_MODULE_ITEM_FAULT_CONNECT_TIMEOUT = 0,
	POWER_MODULE_ITEM_FAULT_FAULT,
	POWER_MODULE_ITEM_FAULT_INPUT_LOW_VOLTAGE,
	POWER_MODULE_ITEM_FAULT_INPUT_OVER_VOLTAGE,
	POWER_MODULE_ITEM_FAULT_SIZE,
} power_module_item_fault_t;

typedef struct {
	struct list_head list;
	int module_id;

	void *power_modules_info;
	void *power_module_group_info;

	uint32_t query_stamp;//1000
	uint32_t setting_stamp;//2000

	//uint32_t test_stamp;//xiaofei

	power_module_item_status_t status;
	bitmap_t *faults;//power_module_item_fault_t
} power_module_item_info_t;

typedef struct {
	struct list_head list;
	int group_id;
	pdu_group_info_t *pdu_group_info;//pdu分组信息
	void *channel_info;
	void *relay_board_item_info;
	struct list_head power_module_item_list;//电源模块空闲列表
} power_module_group_info_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t power_module_type;//power_module_type_t
	uint16_t rate_current;//华为模块参考电流 a
} power_module_settings_t;

typedef struct {
	pdu_config_t pdu_config;

	power_module_settings_t power_module_settings;

	//模块输出能力
	uint16_t module_max_output_voltage;//最大输出电压 0.1v
	uint16_t module_min_output_voltage;//最小输出电压 0.1v
	uint16_t module_max_output_current;//最大输出电流 0.1a
	uint16_t module_min_output_current;//最小输出电流 0.1a
	uint16_t module_max_output_power;//单模块最大功率 w
	uint32_t channels_max_output_power;//柜体最大功率 w
} channels_settings_t;

#pragma pack(pop)

typedef enum {
	RELAY_CHECK_REQUEST_STATE_NONE = 0,
	RELAY_CHECK_REQUEST_STATE_START,
	RELAY_CHECK_REQUEST_STATE_STOP,
} relay_check_request_state_t;

typedef enum {
	RELAY_CHECK_STATE_NONE = 0,
	RELAY_CHECK_STATE_START_SYNC,
	RELAY_CHECK_STATE_PREPARE,
	RELAY_CHECK_STATE_PREPARE_CONFIRM,
	RELAY_CHECK_STATE_INSULATION_CHECK,
	RELAY_CHECK_STATE_CHANNEL_MODULE_ID_CHECK,
	RELAY_CHECK_STATE_CHANNEL_CONFIG,
	RELAY_CHECK_STATE_CHANNEL_CONFIG_SYNC,
	RELAY_CHECK_STATE_CHANNEL_CHECK,
	RELAY_CHECK_STATE_STOP,
} relay_check_state_t;

typedef enum {
	RELAY_CHECK_FAULT_FAULT = 0,
	RELAY_CHECK_FAULT_SIZE,
} relay_check_fault_t;

typedef struct {
	uint32_t stamp;
	relay_check_request_state_t relay_check_request_state;
	relay_check_state_t relay_check_state;
	uint8_t relay_check_channel_id;
	uint8_t relay_check_power_module_group_id;
	uint8_t relay_check_board_id;
	bitmap_t *faults;//relay_check_fault_t
} relay_check_info_t;

typedef enum {
	CHANNELS_FAULT_RELAY_CHECK = 0,
	CHANNELS_FAULT_FORCE_STOP,
	CHANNELS_FAULT_DOOR,
	CHANNELS_FAULT_DISPLAY,
	CHANNELS_FAULT_SIZE,
} channels_fault_t;

typedef struct {
	struct list_head list;
	event_pool_t *event_pool;
	channels_config_t *channels_config;
	uint32_t periodic_stamp;

	storage_info_t *storage_info;

	channels_settings_t channels_settings;
	uint8_t channels_settings_invalid;
	display_cache_channels_t display_cache_channels;

	callback_chain_t *common_periodic_chain;
	callback_chain_t *common_event_chain;

	callback_item_t common_periodic_callback_item;

	callback_item_t display_data_action_callback_item;
	callback_item_t display_data_invalid_callback_item;
	callback_item_t display_data_changed_callback_item;

	uint8_t channel_number;//所有通道数
	channel_info_t *channel_info;//os_alloc 所有通道信息

	uint8_t power_module_group_number;//所有模块组数
	power_module_group_info_t *power_module_group_info;//模块组信息

	uint8_t power_module_item_number;//所有模块数
	power_module_item_info_t *power_module_item_info;//模块信息

	uint8_t relay_board_item_number;//所有开关板数
	relay_board_item_info_t *relay_board_item_info;//开关板信息

	uint8_t pdu_group_number;//pdu组数
	pdu_group_info_t *pdu_group_info;//pdu分组信息

	void *power_modules_info;
	void *channels_com_info;
	void *relay_boards_com_info;
	void *display_info;

	uint8_t configed;

	relay_check_info_t relay_check_info;

	bitmap_t *faults;//channels_fault_t
} channels_info_t;

char *get_channel_state_des(channel_state_t state);
char *get_channel_event_type_des(channel_event_type_t type);
char *get_channels_event_type_des(channels_event_type_t type);
char *get_channels_change_state_des(channels_change_state_t state);
char *get_power_module_policy_des(power_module_policy_t policy);
int set_fault(bitmap_t *faults, int fault, uint8_t v);
int get_fault(bitmap_t *faults, int fault);
int get_first_fault(bitmap_t *faults);
void free_channels_info(channels_info_t *channels_info);
char *get_relay_check_state_des(relay_check_state_t state);
char *get_power_module_item_state_des(power_module_item_state_t state);
void set_power_module_policy_request(power_module_policy_t policy);
void start_dump_channels_stats(void);
void task_channels(void const *argument);
int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout);
void start_stop_channel(channels_info_t *channels_info, uint8_t channel_id, channel_event_type_t channel_event_type);
void channels_modbus_data_action(void *fn_ctx, void *chain_ctx);
void load_channels_display_cache(channels_info_t *channels_info);
void sync_channels_display_cache(channels_info_t *channels_info);
channels_info_t *get_or_alloc_channels_info(channels_config_t *channels_config);
channels_info_t *start_channels(void);

#endif //_CHANNELS_H
