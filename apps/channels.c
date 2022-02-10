

/*================================================================
 *
 *
 *   文件名称：channels.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 09时26分31秒
 *   修改日期：2022年02月10日 星期四 19时47分01秒
 *   描    述：
 *
 *================================================================*/
#include "channels.h"

#include <string.h>
#include <stdlib.h>

#include "object_class.h"
#include "os_utils.h"

#include "channel.h"
#include "power_manager.h"
#include "config_layout.h"
#include "display.h"
#if defined(CHARGER_CHANNEL_PROXY_REMOTE)
#include "channels_comm_proxy_remote.h"
#endif
#if defined(CHARGER_CHANNEL_PROXY_LOCAL)
#include "channels_comm_proxy_local.h"
#endif

#include "log.h"

static object_class_t *channels_class = NULL;

char *get_channel_event_type_des(channel_event_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNEL_EVENT_TYPE_UNKNOW);
			add_des_case(CHANNEL_EVENT_TYPE_START_CHANNEL);
			add_des_case(CHANNEL_EVENT_TYPE_STOP_CHANNEL);

		default: {
		}
		break;
	}

	return des;
}

char *get_channels_event_type_des(channels_event_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNELS_EVENT_UNKNOW);
			add_des_case(CHANNELS_EVENT_INSULATION);
			add_des_case(CHANNELS_EVENT_TELEMETER);
			add_des_case(CHANNELS_EVENT_CARD_READER);
			add_des_case(CHANNELS_EVENT_DISPLAY);
			add_des_case(CHANNELS_EVENT_CHANNEL);

		default: {
		}
		break;
	}

	return des;
}

int set_fault(bitmap_t *faults, int fault, uint8_t v)
{
	return set_bitmap_value(faults, fault, v);
}

int get_fault(bitmap_t *faults, int fault)
{
	return get_bitmap_value(faults, fault);
}

int get_first_fault(bitmap_t *faults)
{
	int ret = get_first_value_index(faults, 1);

	if(ret >= faults->size) {
		ret = -1;
	}

	return ret;
}

static void free_channels_info(channels_info_t *channels_info)
{
	app_panic();
}

static void channels_info_process_event(channels_info_t *channels_info)
{
	channels_event_t *channels_event = NULL;
	int ret = -1;

	if(channels_info == NULL) {
		return;
	}

	ret = event_pool_wait_event(channels_info->event_pool, CHANNEL_TASK_PERIODIC);

	if(ret != 0) {
		return;
	}

	channels_event = event_pool_get_event(channels_info->event_pool);

	if(channels_event != NULL) {
		do_callback_chain(channels_info->common_event_chain, channels_event);

		if(channels_event->event != NULL) {
			os_free(channels_event->event);
		}

		os_free(channels_event);
	}
}

static void channels_info_handle_common_periodic(channels_info_t *channels_info)
{
	do_callback_chain(channels_info->common_periodic_chain, channels_info);
}

static void channels_periodic(channels_info_t *channels_info)
{
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channels_info->periodic_stamp) >= CHANNEL_TASK_PERIODIC) {
		channels_info->periodic_stamp = ticks;
		channels_info_handle_common_periodic(channels_info);
	}
}

static void task_channels(void const *argument)
{
	channels_info_t *channels_info = (channels_info_t *)argument;

	OS_ASSERT(channels_info != NULL);

	while(1) {
		if(channels_info->configed == 0) {
			osDelay(10);
			continue;
		}

		channels_info_process_event(channels_info);

		//处理周期性事件
		channels_periodic(channels_info);
	}
}

int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout)
{
	return event_pool_put_event(channels_info->event_pool, channels_event, timeout);
}

int channels_info_load_config(channels_info_t *channels_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.settings.storage_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return load_config_item(channels_info->storage_info, "channels_info->channels_settings", &channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

int channels_info_save_config(channels_info_t *channels_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channels_settings_seg.settings.storage_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return save_config_item(channels_info->storage_info, "channels_info->channels_settings", &channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

__weak void channels_modbus_data_action(void *fn_ctx, void *chain_ctx)
{
	//channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	modbus_data_ctx_t *modbus_data_ctx = (modbus_data_ctx_t *)chain_ctx;

	debug("op:%s, addr:%d, value:%d",
	      (modbus_data_ctx->action == MODBUS_DATA_ACTION_GET) ? "get" :
	      (modbus_data_ctx->action == MODBUS_DATA_ACTION_SET) ? "set" :
	      "unknow",
	      modbus_data_ctx->addr,
	      modbus_data_ctx->value);
}

__weak void load_channels_display_cache(channels_info_t *channels_info)
{
}

__weak void sync_channels_display_cache(channels_info_t *channels_info)
{
}

static void channels_info_channels_settings_invalid(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;
	modbus_data_ctx_t *modbus_data_ctx = (modbus_data_ctx_t *)chain_ctx;

	if(modbus_data_ctx->influence < (void *)&channels_info->channels_settings) {
		return;
	}

	if(modbus_data_ctx->influence >= (void *)(&channels_info->channels_settings + 1)) {
		return;
	}

	debug("[%p, %p, %p]", &channels_info->channels_settings, modbus_data_ctx->influence, &channels_info->channels_settings + 1);

	channels_info->channels_settings_invalid = 1;
}

static void channels_info_channels_settings_changed(void *fn_ctx, void *chain_ctx)
{
	channels_info_t *channels_info = (channels_info_t *)fn_ctx;

	sync_channels_display_cache(channels_info);

	if(channels_info->channels_settings_invalid != 0) {
		channels_info->channels_settings_invalid = 0;
		channels_info_save_config(channels_info);
	}
}

static int channels_info_set_channels_config(channels_info_t *channels_info, channels_config_t *channels_config)
{
	int ret = 0;
	display_info_t *display_info;
	debug("use channels channels_config %d!", channels_config->id);

	channels_info->event_pool = alloc_event_pool();

	OS_ASSERT(channels_info->event_pool != NULL);

	alloc_display_info(channels_info);

	display_info = (display_info_t *)channels_info->display_info;
	OS_ASSERT(display_info != NULL);

	alloc_channels_channel_info(channels_info);

	alloc_power_manager(channels_info);

	alloc_voice_info(channels_info);

	osThreadDef(channels, task_channels, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(channels), channels_info);

	if(display_info->modbus_slave_info != NULL) {
		channels_info->display_data_action_callback_item.fn = channels_modbus_data_action;
		channels_info->display_data_action_callback_item.fn_ctx = channels_info;
		OS_ASSERT(register_callback(display_info->modbus_slave_info->data_action_chain, &channels_info->display_data_action_callback_item) == 0);

		channels_info->display_data_invalid_callback_item.fn = channels_info_channels_settings_invalid;
		channels_info->display_data_invalid_callback_item.fn_ctx = channels_info;
		OS_ASSERT(register_callback(display_info->modbus_slave_info->data_invalid_chain, &channels_info->display_data_invalid_callback_item) == 0);

		channels_info->display_data_changed_callback_item.fn = channels_info_channels_settings_changed;
		channels_info->display_data_changed_callback_item.fn_ctx = channels_info;
		OS_ASSERT(register_callback(display_info->modbus_slave_info->data_changed_chain, &channels_info->display_data_changed_callback_item) == 0);
	}

#if defined(CHARGER_CHANNEL_PROXY_REMOTE)
	start_channels_comm_proxy_remote(channels_info);
#endif

#if defined(CHARGER_CHANNEL_PROXY_LOCAL)
	start_channels_comm_proxy_local(channels_info);
#endif

	channels_info->configed = 1;

	return ret;
}

__weak void power_manager_restore_config(channels_info_t *channels_info)
{
	int i;
	int j;

	channels_config_t *channels_config = channels_info->channels_config;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	power_manager_settings_t *power_manager_settings = &channels_settings->power_manager_settings;

	power_manager_settings->power_manager_group_number = POWER_MANAGER_GROUP_MAX_SIZE;

	for(i = 0; i < power_manager_settings->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];
		power_manager_group_settings->channel_number = channels_config->channel_number;
		power_manager_group_settings->power_module_group_number = POWER_MODULE_GROUP_MAX_SIZE;

		for(j = 0; j < power_manager_group_settings->power_module_group_number; j++) {
			power_module_group_settings_t *power_module_group_settings = &power_manager_group_settings->power_module_group_settings[j];
			power_module_group_settings->power_module_number = 1;
		}
	}
}

static void update_power_module_config(channels_info_t *channels_info)
{
	int i;
	int j;
	uint8_t power_module_number = 0;

	channels_config_t *channels_config = channels_info->channels_config;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	power_manager_settings_t *power_manager_settings = &channels_settings->power_manager_settings;

	for(i = 0; i < power_manager_settings->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];

		for(j = 0; j < power_manager_group_settings->power_module_group_number; j++) {
			power_module_group_settings_t *power_module_group_settings = &power_manager_group_settings->power_module_group_settings[j];
			power_module_number += power_module_group_settings->power_module_number;
		}
	}

	channels_config->power_module_config.power_module_number = power_module_number;
	debug("channels_config->power_module_config.power_module_number:%d", channels_config->power_module_config.power_module_number);
}

static void channels_info_reset_default_config(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	channels_settings_t *channels_settings = &channels_info->channels_settings;

	memset(channels_settings, 0, sizeof(channels_settings_t));

	channels_settings->channel_number = channels_config->channel_number;

	channels_settings->power_module_settings.power_module_type = channels_config->power_module_config.power_module_default_type;
	channels_settings->power_module_settings.rate_current = 21;
	channels_settings->power_manager_settings.type = channels_config->power_manager_config.power_manager_default_type;
	power_manager_restore_config(channels_info);
	channels_settings->card_reader_settings.type = channels_config->card_reader_config.default_type;
	channels_settings->module_max_output_voltage = 10000;
	channels_settings->module_min_output_voltage = 2000;
	channels_settings->module_max_output_current = 1000;
	channels_settings->module_min_output_current = 30;
	channels_settings->module_max_input_voltage = 2530;
	channels_settings->module_min_input_voltage = 1870;
	channels_settings->module_max_output_power = 30000;
	channels_settings->channels_max_output_power = 300000;
}

static int channels_info_init_config(channels_info_t *channels_info)
{
	int ret = channels_info_load_config(channels_info);

	if(ret == 0) {
		debug("channels load config successfully!");

		if(app_get_reset_config() != 0) {
			debug("try to reset config!");
			ret = -1;
		}
	} else {
		debug("channels load config failed!");
	}

	if(ret != 0) {
		channels_info_reset_default_config(channels_info);
		ret = channels_info_save_config(channels_info);
	}

	update_power_module_config(channels_info);

	load_channels_display_cache(channels_info);

	return ret;
}

static channels_info_t *alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(channels_config != NULL);

	channels_info = (channels_info_t *)os_calloc(1, sizeof(channels_info_t));
	OS_ASSERT(channels_info != NULL);

	channels_info->channels_config = channels_config;
	channels_info->storage_info = app_info->storage_info;

	channels_info->common_periodic_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_periodic_chain != NULL);
	channels_info->common_event_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_event_chain != NULL);
	channels_info->channels_notify_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->channels_notify_chain != NULL);

	channels_info->faults = alloc_bitmap(CHANNELS_FAULT_SIZE);
	OS_ASSERT(channels_info->faults != NULL);

	if(channels_info_init_config(channels_info) != 0) {
		debug("");
	}

	OS_ASSERT(channels_info_set_channels_config(channels_info, channels_config) == 0);

	return channels_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channels_info_t *channels_info = (channels_info_t *)o;
	channels_config_t *channels_config = (channels_config_t *)ctx;

	if(channels_info->channels_config == channels_config) {
		ret = 0;
	}

	return ret;
}

static channels_info_t *get_or_alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;

	os_enter_critical();

	if(channels_class == NULL) {
		channels_class = object_class_alloc();
	}

	os_leave_critical();

	channels_info = (channels_info_t *)object_class_get_or_alloc_object(channels_class, object_filter, channels_config, (object_alloc_t)alloc_channels_info, (object_free_t)free_channels_info);

	return channels_info;
}

static channels_info_t *_channels_info = NULL;

channels_info_t *start_channels(void)
{
	channels_config_t *channels_config = get_channels_config(0);

	OS_ASSERT(channels_config != NULL);

	_channels_info = get_or_alloc_channels_info(channels_config);
	OS_ASSERT(_channels_info != NULL);

	return _channels_info;
}

channels_info_t *get_channels(void)
{
	return _channels_info;
}
