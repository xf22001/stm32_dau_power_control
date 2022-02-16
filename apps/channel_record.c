

/*================================================================
 *
 *
 *   文件名称：channel_record.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分21秒
 *   修改日期：2022年02月16日 星期三 10时48分31秒
 *   描    述：
 *
 *================================================================*/
#include "channel_record.h"

#include "log.h"

#define add_des_case_fault(x) add_des_case(x)

//1,$s:\s\+\(.*\),:\tadd_des_case(\1);:gc
char *get_channel_record_item_stop_reason_des(channel_record_item_stop_reason_t stop_reason)
{
	char *des = "unknow";

	switch(stop_reason) {
			//正常结束
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_NONE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_POWER_MANAGER);

			//channels fault
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNELS_FAULT_RELAY_CHECK));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNELS_FAULT_FORCE_STOP));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNELS_FAULT_DOOR));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNELS_FAULT_DISPLAY));

			//channel fault
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNEL_FAULT_FAULT));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNEL_FAULT_CONNECT_TIMEOUT));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_CONNECT_TIMEOUT));
			add_des_case_fault(channel_record_item_stop_reason_fault(CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_OVER_TEMPERATURE));

			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_UNKNOW);

		default: {
		}
		break;
	}

	return des;
}
