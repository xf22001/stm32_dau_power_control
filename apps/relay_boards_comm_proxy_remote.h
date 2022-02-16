

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_comm_proxy_remote.h
 *   创 建 者：肖飞
 *   创建日期：2022年02月15日 星期二 08时45分42秒
 *   修改日期：2022年02月16日 星期三 15时41分21秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARDS_COMM_PROXY_REMOTE_H
#define _RELAY_BOARDS_COMM_PROXY_REMOTE_H
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

int relay_boards_comm_update_config(channels_info_t *channels_info, uint8_t relay_board_id, uint8_t config);
uint8_t relay_boards_comm_proxy_get_connect_state(channels_info_t *channels_info, uint8_t relay_board_id);
uint32_t relay_boards_comm_proxy_get_connect_stamp(channels_info_t *channels_info, uint8_t relay_board_id);
int start_relay_boards_comm_proxy_remote(channels_info_t *channels_info);
#endif //_RELAY_BOARDS_COMM_PROXY_REMOTE_H
