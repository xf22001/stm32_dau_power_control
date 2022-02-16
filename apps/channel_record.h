

/*================================================================
 *   
 *   
 *   文件名称：channel_record.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分28秒
 *   修改日期：2022年02月16日 星期三 10时47分05秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_RECORD_H
#define _CHANNEL_RECORD_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "callback_chain.h"
#include "storage.h"
#include "channels_config.h"

#define channel_record_item_stop_reason_fault(e) CHANNEL_RECORD_ITEM_STOP_REASON_##e

typedef enum {
	//正常结束
	CHANNEL_RECORD_ITEM_STOP_REASON_NONE = 0,
	CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL,
	CHANNEL_RECORD_ITEM_STOP_REASON_POWER_MANAGER,

	//channels fault
	channel_record_item_stop_reason_fault(CHANNELS_FAULT_RELAY_CHECK),
	channel_record_item_stop_reason_fault(CHANNELS_FAULT_FORCE_STOP),
	channel_record_item_stop_reason_fault(CHANNELS_FAULT_DOOR),
	channel_record_item_stop_reason_fault(CHANNELS_FAULT_DISPLAY),

	//channel fault
	channel_record_item_stop_reason_fault(CHANNEL_FAULT_FAULT),
	channel_record_item_stop_reason_fault(CHANNEL_FAULT_CONNECT_TIMEOUT),
	channel_record_item_stop_reason_fault(CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_CONNECT_TIMEOUT),
	channel_record_item_stop_reason_fault(CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_OVER_TEMPERATURE),

	CHANNEL_RECORD_ITEM_STOP_REASON_UNKNOW,
} channel_record_item_stop_reason_t;

typedef struct {
	uint8_t channel_id;
	uint8_t stop_reason;//channel_record_item_stop_reason_t
} channel_record_item_t;

char *get_channel_record_item_stop_reason_des(channel_record_item_stop_reason_t stop_reason);

#endif //_CHANNEL_RECORD_H
