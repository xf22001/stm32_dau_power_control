

/*================================================================
 *
 *
 *   文件名称：channels_comm_proxy.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时42分29秒
 *   修改日期：2022年02月13日 星期日 20时25分01秒
 *   描    述：
 *
 *================================================================*/
#include "channels_comm_proxy.h"

//1,$s:\s\+\(.*\),:\tadd_des_case(\1);:gc
char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(channels_comm_proxy_command_enum(CHANNEL_HEARTBEAT));
			add_des_case(channels_comm_proxy_command_enum(CHANNEL_REQUIRE));
			add_des_case(channels_comm_proxy_command_enum(CHANNEL_START));
			add_des_case(channels_comm_proxy_command_enum(CHANNEL_STOP));
			add_des_case(channels_comm_proxy_command_enum(CHANNEL_OUTPUT));
			add_des_case(channels_comm_proxy_command_enum(DAU_STATUS));
			add_des_case(channels_comm_proxy_command_enum(MODULES_READY));
			add_des_case(channels_comm_proxy_command_enum(MODULES_STATUS));
			add_des_case(channels_comm_proxy_command_enum(INPUT_VOLTAGE));

		default: {
		}
		break;
	}

	return des;
}
