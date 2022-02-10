

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分21秒
 *   修改日期：2022年02月10日 星期四 19时54分33秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"

#include <string.h>

#include "charger_bms.h"
#include "channels.h"
#include "os_utils.h"
#include "net_client.h"

#include "log.h"

void alloc_charger_info(channel_info_t *channel_info)
{
	charger_info_t *charger_info = NULL;
	channel_settings_t *channel_settings = &channel_info->channel_settings;

	debug("channel %d alloc charger!", channel_info->channel_id);

	charger_info = (charger_info_t *)os_calloc(1, sizeof(charger_info_t));

	OS_ASSERT(charger_info != NULL);

	charger_info->channel_info = channel_info;
	channel_info->charger_info = charger_info;

	charger_info->handle_mutex = mutex_create();
	OS_ASSERT(charger_info->handle_mutex);

	charger_info->charger_state_event_chain = alloc_callback_chain();
	OS_ASSERT(charger_info->charger_state_event_chain != NULL);

	set_charger_bms_type(charger_info, channel_settings->charger_settings.charger_type);
}
