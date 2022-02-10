

/*================================================================
 *   
 *   
 *   文件名称：channels_comm_proxy.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时42分29秒
 *   修改日期：2021年12月21日 星期二 15时21分49秒
 *   描    述：
 *
 *================================================================*/
#include "channels_comm_proxy.h"

char *get_channels_comm_proxy_command_des(channels_comm_proxy_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(CHANNELS_COMM_PROXY_COMMAND_PROXY_REMOTE_HEARTBEAT);
			add_des_case(CHANNELS_COMM_PROXY_COMMAND_PROXY_LOCAL_HEARTBEAT);
			add_des_case(CHANNELS_COMM_PROXY_COMMAND_PROXY_REMOTE_STATELESS);
			add_des_case(CHANNELS_COMM_PROXY_COMMAND_PROXY_LOCAL_STATELESS);

		default: {
		}
		break;
	}

	return des;
}
