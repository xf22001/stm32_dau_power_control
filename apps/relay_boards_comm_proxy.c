

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_comm_proxy.c
 *   创 建 者：肖飞
 *   创建日期：2022年02月15日 星期二 09时11分45秒
 *   修改日期：2022年02月15日 星期二 17时09分57秒
 *   描    述：
 *
 *================================================================*/
#include "relay_boards_comm_proxy.h"

#define add_des_case_enum(x) add_des_case(x)

//1,$s:\s\+\(.*\),:\tadd_des_case(\1);:gc
char *get_relay_boards_comm_proxy_command_des(relay_board_cmd_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case_enum(relay_boards_comm_proxy_command_enum(RELAY_BOARD_HEARTBEAT));
			add_des_case_enum(relay_boards_comm_proxy_command_enum(RELAY_BOARDS_HEARTBEAT));
			add_des_case_enum(relay_boards_comm_proxy_command_enum(RELAY_BOARDS_CONFIG));
			add_des_case_enum(relay_boards_comm_proxy_command_enum(RELAY_BOARD_STATUS));

		default: {
		}
		break;
	}

	return des;
}

