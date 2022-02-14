

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时42分29秒
 *   修改日期：2022年02月14日 星期一 16时51分47秒
 *   描    述：
 *
 *================================================================*/
#include "channels_comm_proxy.h"

#define add_des_case_enum(x) add_des_case(x)

//1,$s:\s\+\(.*\),:\tadd_des_case(\1);:gc
char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case_enum(channels_comm_proxy_command_enum(CHANNEL_REQUIRE));
			add_des_case_enum(channels_comm_proxy_command_enum(CHANNEL_START));
			add_des_case_enum(channels_comm_proxy_command_enum(CHANNEL_STOP));
			add_des_case_enum(channels_comm_proxy_command_enum(CHANNEL_OUTPUT));
			add_des_case_enum(channels_comm_proxy_command_enum(DAU_STATUS));
			add_des_case_enum(channels_comm_proxy_command_enum(MODULES_READY));
			add_des_case_enum(channels_comm_proxy_command_enum(MODULES_STATUS));
			add_des_case_enum(channels_comm_proxy_command_enum(INPUT_VOLTAGE));

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_require_work_state_des(channel_require_work_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_WORK_STATE_IDLE);
			add_des_case(CHANNEL_WORK_STATE_PREPARE_START);
			add_des_case(CHANNEL_WORK_STATE_START);
			add_des_case(CHANNEL_WORK_STATE_CHARGE);
			add_des_case(CHANNEL_WORK_STATE_STOP);

		default: {
		}
		break;
	}

	return des;
}

