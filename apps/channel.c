

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分12秒
 *   修改日期：2022年02月13日 星期日 12时51分36秒
 *   描    述：
 *
 *================================================================*/
#include "channel.h"

#if defined(CHARGER_CHANNEL_NATIVE)
#include "channel_handler_native.h"
#endif
#if defined(CHARGER_CHANNEL_PROXY_REMOTE)
#include "channel_handler_proxy_remote.h"
#endif
#if defined(CHARGER_CHANNEL_PROXY_LOCAL)
#include "channel_handler_proxy_local.h"
#endif
#include "charger.h"
#include "config_layout.h"
#include "display.h"

#include "log.h"

static channel_handler_t *channel_handler_sz[] = {
#if defined(CHARGER_CHANNEL_NATIVE)
	&channel_handler_native,
#endif
#if defined(CHARGER_CHANNEL_PROXY_REMOTE)
	&channel_handler_proxy_remote,
#endif
#if defined(CHARGER_CHANNEL_PROXY_LOCAL)
	&channel_handler_proxy_local,
#endif
};

static channel_handler_t *get_channel_handler(channel_type_t channel_type)
{
	int i;
	channel_handler_t *channel_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(channel_handler_sz); i++) {
		channel_handler_t *channel_handler_item = channel_handler_sz[i];

		if(channel_handler_item->channel_type == channel_type) {
			channel_handler = channel_handler_item;
			break;
		}
	}

	return channel_handler;
}

char *get_channel_state_des(channel_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_STATE_NONE);
			add_des_case(CHANNEL_STATE_IDLE);
			add_des_case(CHANNEL_STATE_START);
			add_des_case(CHANNEL_STATE_WAITING);
			add_des_case(CHANNEL_STATE_STARTING);
			add_des_case(CHANNEL_STATE_CHARGING);
			add_des_case(CHANNEL_STATE_STOP);
			add_des_case(CHANNEL_STATE_STOPPING);
			add_des_case(CHANNEL_STATE_END);
			add_des_case(CHANNEL_STATE_CLEANUP);

		default: {
		}
		break;
	}

	return des;
}

char *get_channel_fault_des(channel_fault_t fault)
{
	char *des = "unknow";

	switch(fault) {
			add_des_case(CHANNEL_FAULT_CONNECT_TIMEOUT);
			add_des_case(CHANNEL_FAULT_RELAY_BOARD_OVER_TEMPERATURE);

		default: {
		}
		break;
	}

	return des;
}

char *get_channels_fault_des(channels_fault_t fault)
{
	char *des = "unknow";

	switch(fault) {
			add_des_case(CHANNELS_FAULT_RELAY_CHECK);
			add_des_case(CHANNELS_FAULT_FORCE_STOP);
			add_des_case(CHANNELS_FAULT_DOOR);
			add_des_case(CHANNELS_FAULT_DISPLAY);

		default: {
		}
		break;
	}

	return des;
}

#define add_fault_to_channel_record_item_stop_reason_case(e) \
		case e: { \
			return channel_record_item_stop_reason_fault(e); \
		} \
		break

channel_record_item_stop_reason_t channels_fault_to_channel_record_item_stop_reason(channels_fault_t fault)
{
	switch(fault) {
		add_fault_to_channel_record_item_stop_reason_case(CHANNELS_FAULT_RELAY_CHECK);
		add_fault_to_channel_record_item_stop_reason_case(CHANNELS_FAULT_FORCE_STOP);
		add_fault_to_channel_record_item_stop_reason_case(CHANNELS_FAULT_DOOR);
		add_fault_to_channel_record_item_stop_reason_case(CHANNELS_FAULT_DISPLAY);

		default: {
			return CHANNEL_RECORD_ITEM_STOP_REASON_UNKNOW;
		}
		break;
	}
}

channel_record_item_stop_reason_t channel_fault_to_channel_record_item_stop_reason(channel_fault_t fault)
{
	switch(fault) {
		add_fault_to_channel_record_item_stop_reason_case(CHANNEL_FAULT_FAULT);
		add_fault_to_channel_record_item_stop_reason_case(CHANNEL_FAULT_CONNECT_TIMEOUT);
		add_fault_to_channel_record_item_stop_reason_case(CHANNEL_FAULT_RELAY_BOARD_OVER_TEMPERATURE);

		default: {
			return CHANNEL_RECORD_ITEM_STOP_REASON_UNKNOW;
		}
		break;
	}
}


void channel_request_start(channel_info_t *channel_info)
{
	if(time_valid() != 0) {
		debug("time invalid!");
	} else {
		channel_info->request_start_state = 1;
	}
}

void channel_request_charging(channel_info_t *channel_info)
{
	channel_info->request_charging_state = 1;
}

void channel_request_end(channel_info_t *channel_info)
{
	channel_info->request_end_state = 1;
}

void channel_request_idle(channel_info_t *channel_info)
{
	channel_info->request_idle_state = 1;
}

void channel_request_stop(channel_info_t *channel_info, channel_record_item_stop_reason_t stop_reason)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_NONE:
		case CHANNEL_STATE_IDLE: {
			return;
		}
		break;

		default: {
		}
		break;
	}

	if(channel_info->channel_record_item.stop_reason == CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
		debug("channel %d stop_reason:%s", channel_info->channel_id, get_channel_record_item_stop_reason_des(channel_info->channel_record_item.stop_reason));
		channel_info->channel_record_item.stop_reason = stop_reason;
		do_callback_chain(channel_info->request_stop_chain, channel_info);
	}
}

time_t get_ts_by_seg_index(uint8_t seg_index)
{
	time_t ts = 0;

	if(seg_index > PRICE_SEGMENT_SIZE) {
		seg_index %= PRICE_SEGMENT_SIZE;
	}

	ts = seg_index * 86400 / PRICE_SEGMENT_SIZE;

	return ts;
}

uint8_t get_seg_index_by_ts(time_t ts)
{
	uint8_t seg_index;

	if(ts > 86400) {
		ts %= 86400;
	}

	seg_index = ts / (86400 / PRICE_SEGMENT_SIZE);

	return seg_index;
}

uint8_t parse_price_info(price_info_t *price_info, price_item_cb_t price_item_cb, void *ctx)
{
	int i;
	uint8_t j = 0;
	uint32_t price = price_info->price[0];
	uint8_t start_seg = 0;

	for(i = 0; i < PRICE_SEGMENT_SIZE; i++) {
		if(price_info->price[i] == price) {
			continue;
		}

		if(price_item_cb != NULL) {
			if(price_item_cb(j, start_seg, i, price, ctx) == 0) {
				j++;
			}
		}

		if(i < PRICE_SEGMENT_SIZE) {
			price = price_info->price[i];
			start_seg = i;
		}
	}

	if(price_item_cb(j, start_seg, i, price, ctx) == 0) {
		j++;
	}

	return j;
}

uint32_t get_current_price(channels_info_t *channels_info, time_t ts)
{
	price_info_t *price_info_energy = &channels_info->channels_settings.price_info_energy;
	price_info_t *price_info_service = &channels_info->channels_settings.price_info_service;
	uint8_t index = get_seg_index_by_ts(ts);

	return price_info_energy->price[index] + price_info_service->price[index];
}

int set_channel_type(channel_info_t *channel_info, channel_type_t channel_type)
{
	int ret = -1;
	channel_handler_t *channel_handler = channel_info->channel_handler;

	if(channel_handler != NULL) {
		debug("channel %d set channel handler %s -> %s",
		      channel_info->channel_id,
		      get_channel_config_channel_type_des(channel_handler->channel_type),
		      get_channel_config_channel_type_des(channel_type));

		if(channel_handler->deinit != NULL) {
			if(channel_handler->deinit(channel_info) != 0) {
				debug("");
			}
		}
	} else {
		debug("channel %d set channel handler %s",
		      channel_info->channel_id,
		      get_channel_config_channel_type_des(channel_type));
	}

	channel_info->channel_handler = get_channel_handler(channel_type);

	OS_ASSERT(channel_info->channel_handler != NULL);
	OS_ASSERT(channel_info->channel_handler->init != NULL);
	OS_ASSERT(channel_info->channel_handler->init(channel_info) == 0);
	ret = 0;

	return ret;
}

static int channel_init(channel_info_t *channel_info)
{
	int ret = 0;
	channel_settings_t *channel_settings = &channel_info->channel_settings;

	channel_info->faults = alloc_bitmap(CHANNEL_FAULT_SIZE);
	OS_ASSERT(channel_info->faults != NULL);

	channel_info->request_state = CHANNEL_STATE_NONE;
	channel_info->state = CHANNEL_STATE_IDLE;

	channel_info->channel_periodic_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->channel_periodic_chain != NULL);

	channel_info->start_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->start_chain != NULL);
	channel_info->end_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->end_chain != NULL);
	channel_info->state_changed_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->state_changed_chain != NULL);
	channel_info->request_stop_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->request_stop_chain != NULL);
	channel_info->power_manager_channel_request_state_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->power_manager_channel_request_state_chain != NULL);
	channel_info->bms_auto_start_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->bms_auto_start_chain != NULL);

	alloc_charger_info(channel_info);

	set_channel_type(channel_info, channel_settings->channel_type);

	return ret;
}

static int channel_info_load_config(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.settings.storage_channel_settings[channel_info->channel_id].channel_settings;
	debug("offset:%d", offset);
	return load_config_item(channels_info->storage_info, "channel_info->channel_settings", &channel_info->channel_settings, sizeof(channel_settings_t), offset);
}

static int channel_info_save_config(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.settings.storage_channel_settings[channel_info->channel_id].channel_settings;
	debug("offset:%d", offset);
	return save_config_item(channels_info->storage_info, "channel_info->channel_settings", &channel_info->channel_settings, sizeof(channel_settings_t), offset);
}

static void channel_info_reset_default_config(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;
	channel_settings_t *channel_settings = &channel_info->channel_settings;

	memset(channel_settings, 0, sizeof(channel_settings_t));

	channel_settings->channel_type = channel_config->channel_type;
	channel_settings->charger_settings.charger_type = channel_config->charger_config.charger_type;

	channel_settings->max_output_power = 2000000;

	channel_settings->max_output_voltage = 7500;
	channel_settings->min_output_voltage = 2000;
	channel_settings->max_output_current = 200;
	channel_settings->min_output_voltage = 3;
}

static int channel_info_init_config(channel_info_t *channel_info)
{
	int ret = channel_info_load_config(channel_info);

	if(ret == 0) {
		debug("channel %d load config successfully!", channel_info->channel_id);

		if(app_get_reset_config() != 0) {
			debug("try to reset config!");
			ret = -1;
		}
	} else {
		debug("channel %d load config failed!", channel_info->channel_id);
	}

	if(ret != 0) {
		channel_info_reset_default_config(channel_info);
		ret = channel_info_save_config(channel_info);
	}

	load_channel_display_cache(channel_info);

	return ret;
}

static void handle_channels_force_stop(channels_info_t *channels_info)
{
	uint8_t force_stop = 0;

	if(channels_info->channels_config->force_stop_port == NULL) {
		return;
	}

	if (HAL_GPIO_ReadPin(channels_info->channels_config->force_stop_port, channels_info->channels_config->force_stop_pin) != channels_info->channels_config->force_stop_normal_state) {
		force_stop = 1;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP) != force_stop) {
		set_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP, force_stop);
	}
}

static void handle_channels_door_state(channels_info_t *channels_info)
{
	uint8_t fault_door = 0;

	if(channels_info->channels_config->door_port == NULL) {
		return;
	}

	if (HAL_GPIO_ReadPin(channels_info->channels_config->door_port, channels_info->channels_config->door_pin) != channels_info->channels_config->door_normal_state) {
		fault_door = 1;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_DOOR) != fault_door) {
		set_fault(channels_info->faults, CHANNELS_FAULT_DOOR, fault_door);
	}
}

static void handle_channels_common_periodic(void *_channels_info, void *__channels_info)
{
	channels_info_t *channels_info = (channels_info_t *)_channels_info;

	//debug("channels periodic!");
	handle_channels_force_stop(channels_info);
	handle_channels_door_state(channels_info);
}

static void handle_channels_common_event(void *_channels_info, void *_channels_event)
{
	//channels_info_t *channels_info = (channels_info_t *)_channels_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;

	debug("channels_info common process event %s!", get_channels_event_type_des(channels_event->type));

	switch(channels_event->type) {
		default: {
		}
		break;
	}
}

__weak void load_channel_display_cache(channel_info_t *channel_info)
{
}

__weak void sync_channel_display_cache(channel_info_t *channel_info)
{
}

static void channel_info_channel_settings_invalid(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;
	modbus_data_ctx_t *modbus_data_ctx = (modbus_data_ctx_t *)chain_ctx;

	if(modbus_data_ctx->influence < (void *)&channel_info->channel_settings) {
		return;
	}

	if(modbus_data_ctx->influence >= (void *)(&channel_info->channel_settings + 1)) {
		return;
	}

	debug("[%p, %p, %p]", &channel_info->channel_settings, modbus_data_ctx->influence, &channel_info->channel_settings + 1);

	channel_info->channel_settings_invalid = 1;
}

static void channel_info_channel_settings_changed(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;

	sync_channel_display_cache(channel_info);

	if(channel_info->channel_settings_invalid != 0) {
		channel_info->channel_settings_invalid = 0;
		channel_info_save_config(channel_info);
	}
}

void alloc_channels_channel_info(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	channel_info_t *channel_info = NULL;
	display_info_t *display_info = (display_info_t *)channels_info->display_info;
	int i;

	OS_ASSERT(display_info != NULL);

	channels_info->channel_number = channels_config->channel_number;
	channel_info = (channel_info_t *)os_calloc(channels_info->channel_number, sizeof(channel_info_t));
	OS_ASSERT(channel_info != NULL);
	channels_info->channel_info = channel_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info_item = channel_info + i;

		channel_info_item->channels_info = channels_info;
		channel_info_item->channel_config = channels_config->channel_config[i];
		channel_info_item->channel_id = i;

		if(channel_info_init_config(channel_info_item) != 0) {
			debug("");
		}

		channel_init(channel_info_item);

		if(display_info->modbus_slave_info != NULL) {
			channel_info_item->display_data_invalid_callback_item.fn = channel_info_channel_settings_invalid;
			channel_info_item->display_data_invalid_callback_item.fn_ctx = channel_info_item;
			OS_ASSERT(register_callback(display_info->modbus_slave_info->data_invalid_chain, &channel_info_item->display_data_invalid_callback_item) == 0);

			channel_info_item->display_data_changed_callback_item.fn = channel_info_channel_settings_changed;
			channel_info_item->display_data_changed_callback_item.fn_ctx = channel_info_item;
			OS_ASSERT(register_callback(display_info->modbus_slave_info->data_changed_chain, &channel_info_item->display_data_changed_callback_item) == 0);
		}
	}

	if(channels_info->channels_settings.channel_number > channels_info->channel_number) {
		channels_info->channels_settings.channel_number = channels_info->channel_number;
	}

	channels_info->channel_number = channels_info->channels_settings.channel_number;

	channels_info->periodic_callback_item.fn = handle_channels_common_periodic;
	channels_info->periodic_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_info->periodic_callback_item) == 0);

	channels_info->event_callback_item.fn = handle_channels_common_event;
	channels_info->event_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channels_info->event_callback_item) == 0);
}
