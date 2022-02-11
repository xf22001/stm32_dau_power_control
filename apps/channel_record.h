

/*================================================================
 *   
 *   
 *   文件名称：channel_record.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分28秒
 *   修改日期：2022年02月08日 星期二 08时54分28秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_RECORD_H
#define _CHANNEL_RECORD_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "callback_chain.h"
#include "storage.h"
#include "channels_config.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_RECORD_NUMBER 500

#pragma pack(push, 1)

typedef struct {
	uint16_t start;
	uint16_t end;
} channel_record_info_t;

typedef enum {
	CHANNEL_RECORD_ITEM_STATE_INVALID = 0,
	CHANNEL_RECORD_ITEM_STATE_INIT,
	CHANNEL_RECORD_ITEM_STATE_UPDATE,
	CHANNEL_RECORD_ITEM_STATE_FINISH,
	CHANNEL_RECORD_ITEM_STATE_UPLOAD,
} channel_record_item_state_t;

typedef enum {
	CHANNEL_RECORD_ITEM_START_REASON_NONE = 0,
	CHANNEL_RECORD_ITEM_START_REASON_MANUAL,
	CHANNEL_RECORD_ITEM_START_REASON_CARD,
	CHANNEL_RECORD_ITEM_START_REASON_REMOTE,
	CHANNEL_RECORD_ITEM_START_REASON_VIN,
	CHANNEL_RECORD_ITEM_START_REASON_BMS,
	CHANNEL_RECORD_ITEM_START_REASON_CHARGER,
} channel_record_item_start_reason_t;

typedef enum {
	//正常结束
	CHANNEL_RECORD_ITEM_STOP_REASON_NONE = 0,
	CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL,
	CHANNEL_RECORD_ITEM_STOP_REASON_AMOUNT,
	CHANNEL_RECORD_ITEM_STOP_REASON_ENERGY,
	CHANNEL_RECORD_ITEM_STOP_REASON_DURATION,
	CHANNEL_RECORD_ITEM_STOP_REASON_REMOTE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS,

	//channels fault
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INSULATION,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_CARD_READER,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_DISPLAY,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_POWER_MODULES,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_DOOR,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_RELAY_ADHESION,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_FORCE_STOP,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INPUT_OVER_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INPUT_LOW_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ELECTRIC_LEAK_CALIBRATION,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ELECTRIC_LEAK_PROTECT,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_PE_PROTECT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BREAKDOWN,

	//channel fault
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ENERGYMETER,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_CONNECTOR,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_FUNCTION_BOARD_CONNECT,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ADHESION_P,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ADHESION_N,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_TEMPERATURE_LIMIT,
	CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_VOLTAGE_LIMIT,

	//bms
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OVER_CURRENT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CURRENT_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_OTHER_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRM_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCP_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRO_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRO_READY_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCS_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCL_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BST_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BSD_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_CRO_NOT_READY_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_PRECHARGE_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_EXTERN_VOLTAGE_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_CHECK_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_WITH_ELECTRIFIED,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ABNORMAL_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_CHARGER_NOT_CONNECTED,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_L_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_L,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_N_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_N,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_0_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_1_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CHARGER_LOCK_ERROR,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_UNMATCHED_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_CHARGE_DISABLE,
} channel_record_item_stop_reason_t;

typedef enum {
	CHANNEL_RECORD_CHARGE_MODE_UNKNOW = 0,
	CHANNEL_RECORD_CHARGE_MODE_UNLIMIT,
	CHANNEL_RECORD_CHARGE_MODE_DURATION,
	CHANNEL_RECORD_CHARGE_MODE_AMOUNT,
	CHANNEL_RECORD_CHARGE_MODE_ENERGY,
} channel_record_charge_mode_t;

typedef struct {
	uint16_t id;
	uint8_t state;//channel_record_item_state_t
	time_t start_time;
	time_t stop_time;

	uint8_t channel_id;
	uint8_t start_reason;//channel_record_item_start_reason_t
	uint8_t stop_reason;//channel_record_item_stop_reason_t
	uint8_t charge_mode;//channel_record_charge_mode_t

	uint8_t vin[17];
	uint8_t chm_version_1;//0x01
	uint16_t chm_version_0;//0x01
	uint8_t brm_battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t bcp_rate_total_power;//0.1kwh
	uint16_t bcp_total_voltage;//0.1v
	uint16_t bcp_max_charge_voltage_single_battery;//0.01v
	uint8_t bcp_max_temperature;// -50
	uint16_t bcp_max_charge_voltage;//0.1v 最高允许充电总电压
	uint8_t start_soc;
	uint8_t stop_soc;

	uint8_t serial_no[32];//订单号

	uint64_t card_id;//物理卡号
	uint8_t account[32];//帐号
	uint8_t password[32];//密码
	uint32_t account_balance;//帐号余额 0.01
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元

	time_t charge_duration;//设置充电时长
	uint32_t charge_amount;//设置充电金额 0.01
	uint32_t charge_energy;//设置充电电量

	uint32_t amount;//已使用金额 0.00000001
	uint32_t start_total_energy;//0.0001kwh
	uint32_t stop_total_energy;//0.0001kwh
	uint32_t energy_seg[PRICE_SEGMENT_SIZE];//0.0001kwh

	uint32_t energy;//已充电量 0.0001kwh

	uint8_t magic;//0x73
} channel_record_item_t;

typedef struct {
	uint16_t id;
	uint8_t state;//channel_record_item_state_t
	time_t start_time;
	uint8_t channel_id;
} channel_record_filter_info_t;

#pragma pack(pop)

typedef struct {
	uint8_t id;
	channel_record_info_t channel_record_info;
	callback_chain_t *channel_record_sync_chain;
	os_signal_t sync_signal;
	os_mutex_t mutex;
	storage_info_t *storage_info;
	uint16_t page_load_offset;
	time_t page_load_time;
	uint16_t finish_state_count;
	uint8_t repair_active;
} channel_record_task_info_t; 

typedef int (*channel_record_filter_t)(void *fn_ctx, channel_record_filter_info_t *info);

int alloc_channel_record_item_id(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int get_channel_record_item_by_id(channel_record_task_info_t *channel_record_task_info, uint16_t id, channel_record_item_t *channel_record_item);
int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int get_channel_record_item_by_filter(channel_record_task_info_t *channel_record_task_info, channel_record_filter_t filter, void *fn_ctx, uint16_t start, uint16_t end);
void request_channel_record_repair(channel_record_task_info_t *channel_record_task_info);
void channel_record_mark_upload_all(channel_record_task_info_t *channel_record_task_info);
int channel_record_sync(channel_record_task_info_t *channel_record_task_info);
int channel_record_item_page_load_current(channel_record_task_info_t *channel_record_task_info);
int channel_record_item_page_load_prev(channel_record_task_info_t *channel_record_task_info);
int channel_record_item_page_load_next(channel_record_task_info_t *channel_record_task_info);
int channel_record_item_page_load_location(channel_record_task_info_t *channel_record_task_info);
int get_channel_record_page_load_item_number(void);
void channel_record_item_page_item_refresh(channel_record_item_t *channel_record_item, uint16_t offset, uint16_t id);
channel_record_task_info_t *get_or_alloc_channel_record_task_info(uint8_t id);
char *get_channel_record_item_stop_reason_des(channel_record_item_stop_reason_t stop_reason);

#endif //_CHANNEL_RECORD_H
