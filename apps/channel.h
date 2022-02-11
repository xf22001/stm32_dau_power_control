

/*================================================================
 *   
 *   
 *   文件名称：channel.h
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分16秒
 *   修改日期：2021年12月07日 星期二 12时53分53秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_H
#define _CHANNEL_H
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

typedef int (*price_item_cb_t)(uint8_t i, uint8_t start_seg, uint8_t stop_seg, uint32_t price, void *ctx);

char *get_channel_state_des(channel_state_t state);
char *get_channel_fault_des(channel_fault_t fault);
char *get_channels_fault_des(channels_fault_t fault);
channel_record_item_stop_reason_t channels_fault_to_channel_record_item_stop_reason(channels_fault_t fault);
channel_record_item_stop_reason_t channel_fault_to_channel_record_item_stop_reason(channel_fault_t fault);
void channel_request_start(channel_info_t *channel_info);
void channel_request_charging(channel_info_t *channel_info);
void channel_request_end(channel_info_t *channel_info);
void channel_request_idle(channel_info_t *channel_info);
void channel_request_stop(channel_info_t *channel_info, channel_record_item_stop_reason_t stop_reason);
time_t get_ts_by_seg_index(uint8_t seg_index);
uint8_t get_seg_index_by_ts(time_t ts);
uint8_t parse_price_info(price_info_t *price_info, price_item_cb_t price_item_cb, void *ctx);
uint32_t get_current_price(channels_info_t *channels_info, time_t ts);
int set_channel_type(channel_info_t *channel_info, channel_type_t channel_type);
void load_channel_display_cache(channel_info_t *channel_info);
void sync_channel_display_cache(channel_info_t *channel_info);
void alloc_channels_channel_info(channels_info_t *channels_info);

#endif //_CHANNEL_H
