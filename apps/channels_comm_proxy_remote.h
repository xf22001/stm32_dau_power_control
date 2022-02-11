

/*================================================================
 *   
 *   
 *   文件名称：channels_comm_proxy_remote.h
 *   创 建 者：肖飞
 *   创建日期：2021年09月16日 星期四 10时34分50秒
 *   修改日期：2021年09月16日 星期四 14时54分07秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_COMM_PROXY_REMOTE_H
#define _CHANNELS_COMM_PROXY_REMOTE_H
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

uint8_t channels_comm_proxy_get_connect_state(channels_info_t *channels_info, uint8_t proxy_channel_index);
uint32_t channels_comm_proxy_get_connect_stamp(channels_info_t *channels_info, uint8_t proxy_channel_index);
int start_channels_comm_proxy_remote(channels_info_t *channels_info);

#endif //_CHANNELS_COMM_PROXY_REMOTE_H
