

/*================================================================
 *
 *
 *   文件名称：charger_bms_nobms.c
 *   创 建 者：肖飞
 *   创建日期：2021年12月09日 星期四 08时43分21秒
 *   修改日期：2022年01月14日 星期五 10时48分38秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms_nobms.h"

#include "channel.h"
#include "charger_bms.h"
#include "power_manager.h"

#include "log.h"

typedef struct {
	uint32_t stamps;
} charger_bms_ctx_t;

typedef enum {
	CHARGER_BMS_STATE_IDLE = 0,
	CHARGER_BMS_STATE_STARTING,
	CHARGER_BMS_STATE_CHARGING,
	CHARGER_BMS_STATE_STOPPING,
} charger_bms_state_t;

static int prepare_bms_state_idle(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_request_bms_state_idle(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_START) {
		charger_info->charger_bms_request_action = CHARGER_BMS_REQUEST_ACTION_NONE;

		set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_STARTING);
	}

	return ret;
}

static int prepare_bms_state_starting(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_request_bms_state_starting(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_STOP) {
		charger_info->charger_bms_request_action = CHARGER_BMS_REQUEST_ACTION_NONE;

		set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_STOPPING);
		return ret;
	}

	set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_CHARGING);

	return ret;
}

static int prepare_bms_state_charging(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	//channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	power_manager_channel_request_state_t state = POWER_MANAGER_CHANNEL_REQUEST_STATE_START;

	channel_request_charging(channel_info);
	do_callback_chain(channel_info->power_manager_channel_state_chain, &state);

	return ret;
}

static int handle_request_bms_state_charging(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	//channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_STOP) {
		charger_info->charger_bms_request_action = CHARGER_BMS_REQUEST_ACTION_NONE;
		power_manager_channel_request_state_t state = POWER_MANAGER_CHANNEL_REQUEST_STATE_STOP;

		do_callback_chain(channel_info->power_manager_channel_state_chain, &state);
		channel_request_end(channel_info);
		set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_STOPPING);
	}

	return ret;
}

static int prepare_bms_state_stopping(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_request_bms_state_stopping(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;

	channel_request_idle(channel_info);
	set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_IDLE);

	return ret;
}

static charger_bms_state_handler_t state_handler_sz[] = {
	{
		.bms_state = CHARGER_BMS_STATE_IDLE,
		.prepare = prepare_bms_state_idle,
		.handle_request = handle_request_bms_state_idle,
	},
	{
		.bms_state = CHARGER_BMS_STATE_STARTING,
		.prepare = prepare_bms_state_starting,
		.handle_request = handle_request_bms_state_starting,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CHARGING,
		.prepare = prepare_bms_state_charging,
		.handle_request = handle_request_bms_state_charging,
	},
	{
		.bms_state = CHARGER_BMS_STATE_STOPPING,
		.prepare = prepare_bms_state_stopping,
		.handle_request = handle_request_bms_state_stopping,
	},
};

static charger_bms_state_handler_t *get_charger_bms_state_handler(uint8_t bms_state)
{
	int i;
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(state_handler_sz); i++) {
		charger_bms_state_handler_t *charger_bms_state_handler_item = &state_handler_sz[i];

		if(charger_bms_state_handler_item->bms_state == bms_state) {
			charger_bms_state_handler = charger_bms_state_handler_item;
		}
	}

	return charger_bms_state_handler;
}

static char *get_charger_bms_state_des(uint8_t state)
{
	char *des = NULL;

	switch(state) {
			add_des_case(CHARGER_BMS_STATE_IDLE);
			add_des_case(CHARGER_BMS_STATE_STARTING);
			add_des_case(CHARGER_BMS_STATE_CHARGING);
			add_des_case(CHARGER_BMS_STATE_STOPPING);

		default: {
			des = "unknow state";
		}
		break;
	}

	return des;
}

static void update_charger_bms_state(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;
	uint8_t request_state = charger_info->request_state;

	if((charger_info->state == request_state) && (charger_info->charger_bms_state_handler != NULL)) {
		return;
	}

	charger_bms_state_handler = get_charger_bms_state_handler(request_state);
	OS_ASSERT(charger_bms_state_handler != NULL);

	debug("charger %d change state: %s -> %s!", channel_info->channel_id, get_charger_bms_state_des(charger_info->state), get_charger_bms_state_des(request_state));

	charger_info->state = request_state;

	if(charger_bms_state_handler->prepare != NULL) {
		charger_bms_state_handler->prepare(charger_info);
	}

	charger_info->charger_bms_state_handler = charger_bms_state_handler;
}

static int handle_request(charger_info_t *charger_info)
{
	int ret = 0;

	if(charger_info->charger_bms_state_handler == NULL) {
		debug("");
		return ret;
	}

	ret = charger_info->charger_bms_state_handler->handle_request(charger_info);

	return ret;
}

static void charger_handle_request(charger_info_t *charger_info)
{
	int ret;

	mutex_lock(charger_info->handle_mutex);

	ret = handle_request(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

static void charger_bms_periodic(void *_charger_info, void *_channels_info)
{
	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	update_charger_bms_state(charger_info);
	charger_handle_request(charger_info);
}

static void modify_valid_time(void)
{
	struct tm tm = {0};
	time_t ts;

	tm.tm_year = 2021 - 1900;
	tm.tm_mon = 1 - 1;
	tm.tm_mon = 1 - 1;
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	ts = mktime(&tm);
	set_time(ts);
}

static int init_nobms(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	charger_bms_ctx_t *charger_bms_ctx = os_calloc(1, sizeof(charger_bms_ctx_t));

	OS_ASSERT(charger_bms_ctx != NULL);
	OS_ASSERT(charger_info->charger_bms_ctx == NULL);
	charger_info->charger_bms_ctx = charger_bms_ctx;

	charger_info->charger_bms_state_handler = NULL;
	set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_IDLE);
	update_charger_bms_state(charger_info);

	charger_info->periodic_request_cb.fn = charger_bms_periodic;
	charger_info->periodic_request_cb.fn_ctx = charger_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &charger_info->periodic_request_cb) == 0);
	modify_valid_time();

	return ret;
}

static int deinit_nobms(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;

	OS_ASSERT(remove_callback(channels_info->common_periodic_chain, &charger_info->periodic_request_cb) == 0);

	charger_info->charger_bms_ctx = NULL;

	OS_ASSERT(charger_bms_ctx != NULL);
	os_free(charger_bms_ctx);

	return ret;
}

charger_bms_handler_t charger_bms_handler_nobms = {
	.charger_type = CHANNEL_CHARGER_BMS_TYPE_NOBMS,
	.init = init_nobms,
	.deinit = deinit_nobms,
};
