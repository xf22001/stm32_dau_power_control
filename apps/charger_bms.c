

/*================================================================
 *
 *
 *   文件名称：charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月04日 星期五 16时51分31秒
 *   修改日期：2021年12月09日 星期四 08时52分43秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms.h"
#if defined(CHARGER_BMS_HANDLER_GB)
#include "charger_bms_gb.h"
#endif
#if defined(CHARGER_BMS_HANDLER_AC)
#include "charger_bms_ac.h"
#endif
#if defined(CHARGER_BMS_HANDLER_NOBMS)
#include "charger_bms_nobms.h"
#endif
#if defined(CHARGER_BMS_HANDLER_CUSTOM)
#include "charger_bms_custom.h"
#endif

#include "log.h"

static charger_bms_handler_t *charger_bms_handler_sz[] = {
#if defined(CHARGER_BMS_HANDLER_GB)
	&charger_bms_handler_gb,
#endif
#if defined(CHARGER_BMS_HANDLER_AC)
	&charger_bms_handler_ac,
#endif
#if defined(CHARGER_BMS_HANDLER_NOBMS)
	&charger_bms_handler_nobms,
#endif
#if defined(CHARGER_BMS_HANDLER_CUSTOM)
	&charger_bms_handler_custom,
#endif
};

static charger_bms_handler_t *get_charger_bms_handler(channel_charger_bms_type_t type)
{
	int i;
	charger_bms_handler_t *charger_bms_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(charger_bms_handler_sz); i++) {
		charger_bms_handler_t *charger_bms_handler_item = charger_bms_handler_sz[i];

		if(charger_bms_handler_item->charger_type == type) {
			charger_bms_handler = charger_bms_handler_item;
		}
	}

	return charger_bms_handler;
}

static char *get_charger_bms_work_state_des(charger_bms_work_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHARGER_BMS_WORK_STATE_IDLE);
			add_des_case(CHARGER_BMS_WORK_STATE_STARTING);
			add_des_case(CHARGER_BMS_WORK_STATE_RUNNING);
			add_des_case(CHARGER_BMS_WORK_STATE_STOPPING);

		default: {
		}
		break;
	}

	return des;
}

int set_charger_bms_work_state(charger_info_t *charger_info, charger_bms_work_state_t state)
{
	int ret = 0;

	channel_info_t *channel_info = charger_info->channel_info;

	debug("charger %d bms work state %s -> %s", channel_info->channel_id, get_charger_bms_work_state_des(charger_info->charger_bms_work_state), get_charger_bms_work_state_des(state));
	charger_info->charger_bms_work_state = state;

	return ret;
}

static char *get_charger_bms_request_action_des(charger_bms_request_action_t action)
{
	char *des = "unknow";

	switch(action) {
			add_des_case(CHARGER_BMS_REQUEST_ACTION_NONE);
			add_des_case(CHARGER_BMS_REQUEST_ACTION_START);
			add_des_case(CHARGER_BMS_REQUEST_ACTION_STOP);

		default: {
		}
		break;
	}

	return des;
}

int set_charger_bms_request_action(charger_info_t *charger_info, charger_bms_request_action_t action)
{
	int ret = -1;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_NONE) {
		ret = 0;
	}

	debug("charger bms request action %s -> %s", get_charger_bms_request_action_des(charger_info->charger_bms_request_action), get_charger_bms_request_action_des(action));
	charger_info->charger_bms_request_action = action;

	return ret;
}

void set_charger_bms_request_state(charger_info_t *charger_info, uint8_t request_state)
{
	charger_info->request_state = request_state;
}

int set_charger_bms_type(charger_info_t *charger_info, channel_charger_bms_type_t type)
{
	int ret = -1;
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_handler_t *charger_bms_handler = charger_info->charger_bms_handler;

	if(charger_bms_handler != NULL) {
		debug("channel %d set charger bms handler %s -> %s!",
		      channel_info->channel_id,
		      get_channel_config_charger_bms_type_des(charger_bms_handler->charger_type),
		      get_channel_config_charger_bms_type_des(type));

		if(charger_bms_handler->deinit != NULL) {
			if(charger_bms_handler->deinit(charger_info) != 0) {
				debug("");
			}
		}
	} else {
		debug("channel %d set charger bms handler %s!",
		      channel_info->channel_id,
		      get_channel_config_charger_bms_type_des(type));
	}

	charger_info->charger_bms_handler = get_charger_bms_handler(type);

	if((charger_info->charger_bms_handler != NULL) && (charger_info->charger_bms_handler->init != NULL)) {
		OS_ASSERT(charger_info->charger_bms_handler->init(charger_info) == 0);
		ret = 0;
	} else {
		debug("skip channel %d init charger bms handler %s!",
		      channel_info->channel_id,
		      get_channel_config_charger_bms_type_des(type));
	}

	return ret;
}
