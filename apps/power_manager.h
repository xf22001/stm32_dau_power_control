

/*================================================================
 *   
 *   
 *   文件名称：power_manager.h
 *   创 建 者：肖飞
 *   创建日期：2021年11月23日 星期二 14时08分56秒
 *   修改日期：2022年02月16日 星期三 14时11分55秒
 *   描    述：
 *
 *================================================================*/
#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channels.h"
#include "callback_chain.h"
#include "power_modules.h"

typedef enum {
	POWER_MODULE_ITEM_STATE_IDLE = 0,
	POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE,//准备接通
	POWER_MODULE_ITEM_STATE_READY,//达到接通条件
	POWER_MODULE_ITEM_STATE_ACTIVE,//接通
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
	int id;

	void *power_modules_info;
	void *power_module_group_info;

	uint32_t query_stamp;//1000
	uint32_t setting_stamp;//2000

	power_module_item_status_t status;
	bitmap_t *faults;//power_module_item_fault_t
} power_module_item_info_t;

typedef union {
	power_module_status_t s;
	uint16_t v;
} u_power_module_status_t;

typedef struct {
	struct list_head list;
	struct list_head power_module_item_list;//电源模块列表
	uint8_t id;
	void *power_manager_group_info;
	void *power_manager_channel_info;
} power_module_group_info_t;

typedef enum {
	POWER_MANAGER_CHANNEL_STATE_IDLE = 0,
	POWER_MANAGER_CHANNEL_STATE_PREPARE_START,
	POWER_MANAGER_CHANNEL_STATE_START,
	POWER_MANAGER_CHANNEL_STATE_RUNNING,
	POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP,
	POWER_MANAGER_CHANNEL_STATE_STOP,
} power_manager_channel_state_t;

typedef struct {
	power_manager_channel_state_t state;

	//需求电压或电流为0,关机
	uint16_t require_output_voltage;//通道需求电压
	uint16_t require_output_current;//通道需求电流
	uint32_t max_output_power;

	uint8_t reassign;//快充模式需要二次重分配模块
	uint8_t reassign_module_group_number;//快充模式当前分配任务模块组需求

	//充电机输出
	uint32_t charge_output_stamp;
	uint16_t charge_output_voltage;//通道输出电压0.1v
	uint16_t charge_output_current;//通道输出电流0.1a -400

	uint16_t charge_output_voltage_module;//电源模块计算通道输出电压0.1v
	uint16_t charge_output_current_module;//电源模块计算通道输出电流0.1a -400

	uint8_t module_ready_notify;
} power_manager_channel_status_t;

typedef enum {
	POWER_MANAGER_CHANNEL_REQUEST_STATE_NONE = 0,
	POWER_MANAGER_CHANNEL_REQUEST_STATE_START,
	POWER_MANAGER_CHANNEL_REQUEST_STATE_STOP,
} power_manager_channel_request_state_t;

typedef struct {
	struct list_head list;
	struct list_head power_module_group_list;//关联的电源模块组
	struct list_head relay_board_list;//关联的dau模块
	uint8_t id;
	void *power_manager_group_info;
	power_manager_channel_status_t status;
	power_manager_channel_request_state_t request_state;
	uint32_t state_change_stamp;
} power_manager_channel_info_t;

typedef enum {
	POWER_MANAGER_RELAY_BOARD_FAULT_FAULT = 0,
	POWER_MANAGER_RELAY_BOARD_FAULT_CONNECT,
	POWER_MANAGER_RELAY_BOARD_FAULT_OVER_TEMPERATURE,
	POWER_MANAGER_RELAY_BOARD_FAULT_SIZE,
} power_manager_relay_board_fault_t;

typedef struct {
	struct list_head list;
	uint8_t id;
	void *power_manager_channel_info;
	uint8_t offset;//当前dau模块在电源管理模块组的绝对偏移
	uint8_t number;//当前dau模块有效位数
	uint8_t config;//当前dau模块配置
	uint8_t remote_config;//远端dau模块配置
	int16_t temperature1;
	int16_t temperature2;
	bitmap_t *faults;//power_manager_relay_board_fault_t
} power_manager_relay_board_info_t;

typedef enum {
	POWER_MANAGER_CHANGE_STATE_IDLE = 0,
	POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE,
	POWER_MANAGER_CHANGE_STATE_MODULE_FREE,
	POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG,
	POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC,
	POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN,
	POWER_MANAGER_CHANGE_STATE_MODULE_READY_SYNC,
	POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG,
	POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC,
} power_manager_change_state_t;

typedef enum {
	POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_FAULT = 0,
	POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_CONNECT_TIMEOUT,
	POWER_MANAGER_GROUP_FAULT_RELAY_SIZE,
} power_manager_group_fault_t;

typedef struct {
	uint8_t power_module_group_number;
	uint8_t channel_number;
	void *power_manager_info;
	uint8_t id;
	power_manager_change_state_t change_state;//电源管理切换状态
	uint32_t change_state_stamps;

	struct list_head channel_idle_list;//通道空闲
	struct list_head channel_active_list;//通道使用中
	struct list_head channel_deactive_list;//通道停用中
	struct list_head channel_disable_list;//通道故障禁用

	struct list_head power_module_group_idle_list;//电源模块组空闲列表
	struct list_head power_module_group_deactive_list;//电源模块组执行关断列表,表示相应模块组正在关断，各枪需要等待关断完成
	struct list_head power_module_group_disable_list;//电源模块组禁用列表

	power_module_group_info_t *power_module_group_info;
	bitmap_t *faults;//power_manager_group_fault_t
} power_manager_group_info_t;//电源管理分组信息

typedef int (*power_manager_handler_init_t)(void *_power_manager_info);
typedef int (*power_manager_handler_deinit_t)(void *_power_manager_info);

typedef struct {
	power_manager_type_t type;
	power_manager_handler_init_t init;
	power_manager_handler_deinit_t deinit;
} power_manager_handler_t;

typedef int (*power_manager_group_policy_handler_init_t)(void *_power_manager_info);
typedef int (*power_manager_group_policy_handler_deinit_t)(void *_power_manager_info);
typedef int (*power_manager_group_policy_handler_channel_start_t)(void *_power_manager_channel_info);
typedef int (*power_manager_group_policy_handler_channel_charging_t)(void *_power_manager_channel_info);
typedef int (*power_manager_group_policy_handler_free_t)(void *_power_manager_group_info);
typedef int (*power_manager_group_policy_handler_assign_t)(void *_power_manager_group_info);
typedef int (*power_manager_group_policy_handler_config_t)(void *_power_manager_group_info);
typedef int (*power_manager_group_policy_handler_sync_t)(void *_power_manager_group_info);

typedef struct {
	uint8_t policy;
	power_manager_group_policy_handler_init_t init;
	power_manager_group_policy_handler_deinit_t deinit;
	power_manager_group_policy_handler_channel_start_t channel_start;
	power_manager_group_policy_handler_channel_charging_t channel_charging;
	power_manager_group_policy_handler_free_t free;
	power_manager_group_policy_handler_assign_t assign;
	power_manager_group_policy_handler_config_t config;
	power_manager_group_policy_handler_sync_t sync;
} power_manager_group_policy_handler_t;

typedef struct {
	uint8_t power_manager_group_number;

	channels_info_t *channels_info;
	power_modules_info_t *power_modules_info;//模块信息
	power_module_item_info_t *power_module_item_info;//模块分配信息
	power_manager_channel_info_t *power_manager_channel_info;//通道分配信息
	power_manager_relay_board_info_t *power_manager_relay_board_info;//dau模块分配信息
	power_manager_group_info_t *power_manager_group_info;//电源管理组分配信息
	power_manager_handler_t *power_manager_handler;
	power_manager_group_policy_handler_t *power_manager_group_policy_handler;
	callback_chain_t *power_manager_channel_module_assign_ready_chain;
	callback_item_t periodic_callback_item;
	callback_item_t power_manager_periodic_callback_item;
	void *power_manager_ctx;
	void *power_manager_group_policy_ctx;
} power_manager_info_t;

char *get_power_manager_channel_state_des(power_manager_channel_state_t state);
char *get_power_manager_group_change_state_des(power_manager_change_state_t state);
char *get_power_module_item_state_des(power_module_item_state_t state);
void start_dump_channels_stats(void);
void alloc_power_manager(channels_info_t *channels_info);
void power_manager_restore_config(channels_info_t *channels_info);
power_manager_group_policy_handler_t *get_power_manager_group_policy_handler(uint8_t policy);
int set_power_manager_channel_request_state(power_manager_info_t *power_manager_info, uint8_t channel_id, power_manager_channel_request_state_t state);

#endif //_POWER_MANAGER_H
