

/*================================================================
 *
 *
 *   文件名称：channels_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分10秒
 *   修改日期：2021年04月13日 星期二 17时38分59秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_COMMUNICATION_H
#define _CHANNELS_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "can_txrx.h"
#include "can_command.h"
#include "channel_command.h"
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

	uint8_t channel_number;
	uint8_t power_module_number;

	command_status_t *cmd_ctx;

	void *channels_com_data_ctx;

	void *module_status;

	connect_state_t *connect_state;

	uint32_t periodic_stamp;

	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;
} channels_com_info_t;

int channels_com_channel_module_assign_info(channels_com_info_t *channels_com_info, uint8_t channel_id, module_assign_info_t module_assign_info);
uint8_t channels_com_get_connect_state(channels_com_info_t *channels_com_info, uint8_t channel_id);
channels_com_info_t *get_or_alloc_channels_com_info(void *ctx);

#endif //_CHANNELS_COMMUNICATION_H
