

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 14时28分08秒
 *   修改日期：2021年04月13日 星期二 17时39分26秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARDS_COMMUNICATION_H
#define _RELAY_BOARDS_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channels_config.h"
#include "can_command.h"
#include "callback_chain.h"
#include "connect_state.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	os_mutex_t handle_mutex;
	can_info_t *can_info;
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;
	void *channels_info;

	uint8_t relay_board_number;

	command_status_t *cmd_ctx;

	void *relay_boards_com_data_ctx;

	connect_state_t *connect_state;

	uint32_t periodic_stamp;

	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;
} relay_boards_com_info_t;

int relay_boards_com_update_config(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id, uint8_t config);
uint8_t relay_boards_com_get_connect_state(relay_boards_com_info_t *relay_boards_com_info, uint8_t relay_board_id);
relay_boards_com_info_t *get_or_alloc_relay_boards_com_info(void *ctx);

#endif //_RELAY_BOARDS_COMMUNICATION_H
