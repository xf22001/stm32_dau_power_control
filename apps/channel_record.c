

/*================================================================
 *
 *
 *   文件名称：channel_record.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分21秒
 *   修改日期：2022年01月12日 星期三 10时57分12秒
 *   描    述：
 *
 *================================================================*/
#include "channel_record.h"

#include "object_class.h"
#include "config_layout.h"
#include "app.h"
#include "log.h"

static object_class_t *channel_record_task_class = NULL;

static int channel_record_info_load(channel_record_task_info_t *channel_record_task_info, channel_record_info_t *channel_record_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channel_record_seg.channel_record.storage_channel_record_info.channel_record_info;
	debug("offset:%d", offset);
	return load_config_item(channel_record_task_info->storage_info, "record_info", channel_record_info, sizeof(channel_record_info_t), offset);
}

static int channel_record_info_save(channel_record_task_info_t *channel_record_task_info, channel_record_info_t *channel_record_info)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channel_record_seg.channel_record.storage_channel_record_info.channel_record_info;
	debug("offset:%d", offset);
	return save_config_item(channel_record_task_info->storage_info, "record_info", channel_record_info, sizeof(channel_record_info_t), offset);
}

static int channel_record_item_load(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item, uint16_t id)
{
	config_layout_t *config_layout = get_config_layout();
	size_t offset = (size_t)&config_layout->channel_record_seg.channel_record.storage_channel_record_item[id].channel_record_item;
	debug("offset:%d", offset);
	return load_config_item(channel_record_task_info->storage_info, "record_item", channel_record_item, sizeof(channel_record_item_t), offset);
}

static int channel_record_item_save(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	config_layout_t *config_layout = get_config_layout();
	uint16_t id = channel_record_item->id;
	size_t offset = (size_t)&config_layout->channel_record_seg.channel_record.storage_channel_record_item[id].channel_record_item;
	debug("offset:%d", offset);
	return save_config_item(channel_record_task_info->storage_info, "record_item", channel_record_item, sizeof(channel_record_item_t), offset);
}

int alloc_channel_record_item_id(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	uint16_t id;

	channel_record_info_t *channel_record_info = &channel_record_task_info->channel_record_info;

	OS_ASSERT(channel_record_item != NULL);

	mutex_lock(channel_record_task_info->mutex);

	channel_record_item->state = CHANNEL_RECORD_ITEM_STATE_INIT;

	debug("start:%d, end:%d", channel_record_info->start, channel_record_info->end);

	id = channel_record_info->end;

	channel_record_info->end += 1;

	if(channel_record_info->end >= CHANNEL_RECORD_NUMBER) {
		channel_record_info->end = 0;
	}

	if(channel_record_info->start == channel_record_info->end) {
		channel_record_info->start += 1;
	}

	if(channel_record_info->start >= CHANNEL_RECORD_NUMBER) {
		channel_record_info->start = 0;
	}

	channel_record_item->id = id;
	channel_record_item->magic = 0x73;

	OS_ASSERT(channel_record_info_save(channel_record_task_info, channel_record_info) == 0);
	OS_ASSERT(channel_record_item_save(channel_record_task_info, channel_record_item) == 0);
	debug("start:%d, end:%d, id:%d", channel_record_info->start, channel_record_info->end, id);

	mutex_unlock(channel_record_task_info->mutex);

	return 0;
}

int get_channel_record_item_by_id(channel_record_task_info_t *channel_record_task_info, uint16_t id, channel_record_item_t *channel_record_item)
{
	int ret = -1;
	int i;

	OS_ASSERT(channel_record_item != NULL);

	if(id >= CHANNEL_RECORD_NUMBER) {
		return ret;
	}

	for(i = 0; i < 3; i++) {
		mutex_lock(channel_record_task_info->mutex);

		if(channel_record_item_load(channel_record_task_info, channel_record_item, id) == 0) {
			ret = 0;
		} else {
			debug("load channel_record %d failed!", id);
		}

		mutex_unlock(channel_record_task_info->mutex);
	}

	if(ret != 0) {
		channel_record_item->magic = 0;
		mutex_lock(channel_record_task_info->mutex);

		if(channel_record_item_save(channel_record_task_info, channel_record_item) == 0) {
			debug("mark channel_record %d invalid successful!", id);
		} else {
			debug("mark channel_record %d invalid failed!", id);
		}

		mutex_unlock(channel_record_task_info->mutex);
	}

	return ret;
}

int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	int ret = -1;

	if(channel_record_item == NULL) {
		return ret;
	}

	if(channel_record_item->id >= CHANNEL_RECORD_NUMBER) {
		return ret;
	}

	if(channel_record_item->state == CHANNEL_RECORD_ITEM_STATE_INVALID) {
		return ret;
	}

	mutex_lock(channel_record_task_info->mutex);
	ret = channel_record_item_save(channel_record_task_info, channel_record_item);
	mutex_unlock(channel_record_task_info->mutex);
	return ret;
}

int get_channel_record_item_by_filter(channel_record_task_info_t *channel_record_task_info, channel_record_filter_t filter, void *fn_ctx, uint16_t start, uint16_t end)
{
	int ret = -1;
	int i;

	debug("start:%d, end:%d", start, end);

	if(start == end) {//没有记录
		return ret;
	} else if(end < start) {
		end += CHANNEL_RECORD_NUMBER;
	}

	for(i = end - 1; i >= start; i--) {
		config_layout_t *config_layout = get_config_layout();
		uint16_t offset = i % CHANNEL_RECORD_NUMBER;
		size_t offset_base = (size_t)&config_layout->channel_record_seg.channel_record.storage_channel_record_item[offset].channel_record_item;
		channel_record_item_t *channel_record_item = (channel_record_item_t *)0;
		size_t offset_id = (uint8_t *)&channel_record_item->id - (uint8_t *)channel_record_item;
		size_t offset_state = (uint8_t *)&channel_record_item->state - (uint8_t *)channel_record_item;
		size_t offset_start_time = (uint8_t *)&channel_record_item->start_time - (uint8_t *)channel_record_item;
		size_t offset_channel_id = (uint8_t *)&channel_record_item->channel_id - (uint8_t *)channel_record_item;
		size_t offset_magic = (uint8_t *)&channel_record_item->magic - (uint8_t *)channel_record_item;
		uint8_t magic = 0;//0x73
		channel_record_filter_info_t info;

		storage_read(channel_record_task_info->storage_info, offset_base + offset_magic, (uint8_t *)&magic, sizeof(magic));

		if(magic != 0x73) {
			debug("");
			continue;
		}

		storage_read(channel_record_task_info->storage_info, offset_base + offset_id, (uint8_t *)&info.id, sizeof(info.id));

		if(info.id != offset) {
			debug("");
			continue;
		}

		storage_read(channel_record_task_info->storage_info, offset_base + offset_state, (uint8_t *)&info.state, sizeof(info.state));
		storage_read(channel_record_task_info->storage_info, offset_base + offset_start_time, (uint8_t *)&info.start_time, sizeof(info.start_time));
		storage_read(channel_record_task_info->storage_info, offset_base + offset_channel_id, (uint8_t *)&info.channel_id, sizeof(info.channel_id));

		if(filter(fn_ctx, &info) != 0) {
			continue;
		}

		ret = 0;
		break;
	}

	return ret;
}

static int channel_record_item_repare_filter(void *fn_ctx, channel_record_filter_info_t *info)
{
	int ret = -1;
	uint16_t *record_id = (uint16_t *)fn_ctx;
	channels_info_t *channels_info = get_channels();
	channel_info_t *channel_info;
	uint8_t found = 0;

	if(channels_info == NULL) {
		return ret;
	}

	if(info->state != CHANNEL_RECORD_ITEM_STATE_UPDATE) {
		return ret;
	}

	if(info->channel_id >= channels_info->channel_number) {
		debug("info->channel_id:%d, channels_info->channel_number:%d", info->channel_id, channels_info->channel_number);
		return ret;
	}

	channel_info = channels_info->channel_info + info->channel_id;

	if(channel_info->state == CHANNEL_STATE_IDLE) {//found
		found = 1;
	}

	if(channel_info->state == CHANNEL_STATE_CHARGING) {
		if(channel_info->channel_record_item.id != info->id) {
			found = 1;
		}
	}

	if(found == 1) {
		*record_id = info->id;
		ret = 0;
	}

	return ret;
}

static void channel_record_repair(channel_record_task_info_t *channel_record_task_info)
{
	uint16_t record_id = 0;

	if(channel_record_task_info->repair_active == 0) {
		return;
	}

	if(get_channel_record_item_by_filter(channel_record_task_info, channel_record_item_repare_filter, &record_id, channel_record_task_info->channel_record_info.start, channel_record_task_info->channel_record_info.end) == 0) {
		channel_record_item_t *channel_record_item = NULL;

		debug("found channel_record to fix: %d", record_id);
		channel_record_item = os_calloc(1, sizeof(channel_record_item_t));
		OS_ASSERT(channel_record_item != NULL);

		if(get_channel_record_item_by_id(channel_record_task_info, record_id, channel_record_item) == 0) {
			channel_record_item->stop_reason = CHANNEL_RECORD_ITEM_STOP_REASON_BREAKDOWN;
			channel_record_item->state = CHANNEL_RECORD_ITEM_STATE_FINISH;

			if(channel_record_update(channel_record_task_info, channel_record_item) != 0) {
				debug("");
			}
		}

		os_free(channel_record_item);
	} else {
		debug("no channel_record to fix!");
		channel_record_task_info->repair_active = 0;
	}
}

static int filter_channel_record_finish(void *fn_ctx, channel_record_filter_info_t *info)
{
	int ret = -1;
	uint16_t *record_id = (uint16_t *)fn_ctx;

	if(info->state == CHANNEL_RECORD_ITEM_STATE_FINISH) {
		*record_id = info->id;
		ret = 0;
	}

	return ret;
}

void request_channel_record_repair(channel_record_task_info_t *channel_record_task_info)
{
	channel_record_task_info->repair_active = 1;
}

static int channel_record_mark_upload(channel_record_task_info_t *channel_record_task_info)
{
	int ret = -1;
	uint16_t record_id = 0;

	if(get_channel_record_item_by_filter(channel_record_task_info, filter_channel_record_finish, &record_id, channel_record_task_info->channel_record_info.start, channel_record_task_info->channel_record_info.end) == 0) {
		channel_record_item_t *channel_record_item = NULL;

		debug("found channel_record to mark upload: %d", record_id);
		channel_record_item = os_calloc(1, sizeof(channel_record_item_t));
		OS_ASSERT(channel_record_item != NULL);

		if(get_channel_record_item_by_id(channel_record_task_info, record_id, channel_record_item) == 0) {
			channel_record_item->state = CHANNEL_RECORD_ITEM_STATE_UPLOAD;

			if(channel_record_update(channel_record_task_info, channel_record_item) != 0) {
				debug("");
			} else {
				ret = 0;
			}
		}

		os_free(channel_record_item);
	}

	return ret;
}

void channel_record_mark_upload_all(channel_record_task_info_t *channel_record_task_info)
{
	int ret = 0;

	while(ret == 0) {
		ret = channel_record_mark_upload(channel_record_task_info);
	}
}

typedef enum {
	CHANNEL_RECORD_ACTION_NONE = 0,
	CHANNEL_RECORD_ACTION_SYNC,
	CHANNEL_RECORD_ACTION_LOAD_CURRENT,
	CHANNEL_RECORD_ACTION_LOAD_PREV,
	CHANNEL_RECORD_ACTION_LOAD_NEXT,
	CHANNEL_RECORD_ACTION_LOAD_LOCATION,
} channel_record_action_t;

int channel_record_sync(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, CHANNEL_RECORD_ACTION_SYNC, 10);
}

int channel_record_item_page_load_current(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, CHANNEL_RECORD_ACTION_LOAD_CURRENT, 10);
}

int channel_record_item_page_load_prev(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, CHANNEL_RECORD_ACTION_LOAD_PREV, 10);
}

int channel_record_item_page_load_next(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, CHANNEL_RECORD_ACTION_LOAD_NEXT, 10);
}

int channel_record_item_page_load_location(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, CHANNEL_RECORD_ACTION_LOAD_LOCATION, 10);
}

__weak int get_channel_record_page_load_item_number(void)
{
	return 10;
}

__weak void channel_record_item_page_item_refresh(channel_record_item_t *channel_record_item, uint16_t offset, uint16_t id)
{
	debug("offset:%d, id:%d", offset, id);
}

typedef struct {
	time_t ts;
	uint16_t id;
} channel_record_item_date_filter_ctx_t;

static int channel_record_item_date_filter(void *fn_ctx, channel_record_filter_info_t *info)
{
	int ret = -1;
	channel_record_item_date_filter_ctx_t *ctx = (channel_record_item_date_filter_ctx_t *)fn_ctx;

	if((info->start_time / 86400) == (ctx->ts / 86400)) {
		ctx->id = info->id;
		ret = 0;
	}

	return ret;
}

static void channel_record_item_page_load(channel_record_task_info_t *channel_record_task_info, channel_record_action_t action)
{
	uint16_t start = channel_record_task_info->channel_record_info.start;
	uint16_t end = channel_record_task_info->channel_record_info.end;
	uint16_t page_load_offset = channel_record_task_info->page_load_offset;
	uint16_t offset = 0;
	int i;
	channel_record_item_t *channel_record_item = NULL;

	if(end == start) {
		return;
	}

	if(end < start) {
		end += CHANNEL_RECORD_NUMBER;
	}

	if(page_load_offset < start) {
		page_load_offset += CHANNEL_RECORD_NUMBER;
	}

	if(page_load_offset > end) {
		page_load_offset = end;
	}

	switch(action) {
		case CHANNEL_RECORD_ACTION_LOAD_CURRENT: {//最新记录
			page_load_offset = end;
		}
		break;

		case CHANNEL_RECORD_ACTION_LOAD_PREV: {//上页
			if(page_load_offset + get_channel_record_page_load_item_number() <= end) {
				page_load_offset += get_channel_record_page_load_item_number();
			} else {
				page_load_offset = end;//首页
			}
		}
		break;

		case CHANNEL_RECORD_ACTION_LOAD_NEXT: {//下页
			if(page_load_offset >= start + get_channel_record_page_load_item_number()) {
				page_load_offset -= get_channel_record_page_load_item_number();
			} else {//最后一页
			}
		}
		break;

		case CHANNEL_RECORD_ACTION_LOAD_LOCATION: {//定位
			channel_record_item_date_filter_ctx_t ctx;
			ctx.id = end;
			debug("ctx.id:%d", ctx.id);
			ctx.ts = channel_record_task_info->page_load_time;

			get_channel_record_item_by_filter(channel_record_task_info, channel_record_item_date_filter, &ctx, channel_record_task_info->channel_record_info.start, channel_record_task_info->channel_record_info.end);
			page_load_offset = ctx.id;
			debug("page_load_offset:%d", page_load_offset);
		}
		break;

		default: {
		}
		break;
	}

	//refresh page items
	channel_record_item = os_calloc(1, sizeof(channel_record_item_t));
	OS_ASSERT(channel_record_item != NULL);

	for(i = page_load_offset - 1; i >= start; i--) {
		if(get_channel_record_item_by_id(channel_record_task_info, i, channel_record_item) != 0) {
			channel_record_item_page_item_refresh(NULL, offset++, i);
		} else {
			channel_record_item_page_item_refresh(channel_record_item, offset++, i);
		}

		if(offset >= get_channel_record_page_load_item_number()) {
			break;
		}
	}

	os_free(channel_record_item);

	channel_record_task_info->page_load_offset = page_load_offset % CHANNEL_RECORD_NUMBER;
}

static void free_channel_record_task_info(channel_record_task_info_t *channel_record_task_info)
{
	app_panic();
}

static int calc_channel_record_finish_state(void *fn_ctx, channel_record_filter_info_t *info)
{
	int ret = -1;
	uint16_t *finish_count = (uint16_t *)fn_ctx;

	if(info->state == CHANNEL_RECORD_ITEM_STATE_FINISH) {
		*finish_count += 1;
	}

	return ret;
}

static void channel_record_update_finish_state_count(channel_record_task_info_t *channel_record_task_info)
{
	uint16_t finish_count = 0;
	get_channel_record_item_by_filter(channel_record_task_info, calc_channel_record_finish_state, &finish_count, channel_record_task_info->channel_record_info.start, channel_record_task_info->channel_record_info.end);
	channel_record_task_info->finish_state_count = finish_count;
}

static void channel_record_task(void const *argument)
{
	channel_record_task_info_t *channel_record_task_info = (channel_record_task_info_t *)argument;

	if(channel_record_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		uint32_t event;
		int ret;

		channel_record_repair(channel_record_task_info);

		ret = signal_wait(channel_record_task_info->sync_signal, &event, 60 * 1000);

		do_callback_chain(channel_record_task_info->channel_record_sync_chain, channel_record_task_info);

		if(ret == 0) {
			switch(event) {
				case CHANNEL_RECORD_ACTION_LOAD_CURRENT:
				case CHANNEL_RECORD_ACTION_LOAD_PREV:
				case CHANNEL_RECORD_ACTION_LOAD_NEXT:
				case CHANNEL_RECORD_ACTION_LOAD_LOCATION: {
					channel_record_item_page_load(channel_record_task_info, event);
					channel_record_update_finish_state_count(channel_record_task_info);
				}
				break;

				default: {
				}
				break;
			}
		}
	}
}

static channel_record_task_info_t *alloc_channel_record_task_info(void *ctx)
{
	channel_record_task_info_t *channel_record_task_info = NULL;
	app_info_t *app_info = get_app_info();
	uint8_t *id = (uint8_t *)ctx;
	int ret;

	OS_ASSERT(app_info != NULL);
	OS_ASSERT(app_info->storage_info != NULL);

	channel_record_task_info = (channel_record_task_info_t *)os_calloc(1, sizeof(channel_record_task_info_t));
	OS_ASSERT(channel_record_task_info != NULL);

	channel_record_task_info->id = *id;

	channel_record_task_info->storage_info = app_info->storage_info;

	channel_record_task_info->channel_record_sync_chain = alloc_callback_chain();
	OS_ASSERT(channel_record_task_info->channel_record_sync_chain != NULL);

	channel_record_task_info->mutex = mutex_create();
	OS_ASSERT(channel_record_task_info->mutex != NULL);

	channel_record_task_info->sync_signal = signal_create(1);
	OS_ASSERT(channel_record_task_info->sync_signal != NULL);

	mutex_lock(channel_record_task_info->mutex);

	ret = channel_record_info_load(channel_record_task_info, &channel_record_task_info->channel_record_info);

	if(ret == 0) {
		debug("channel record load config successful!");

		if(app_get_reset_config() != 0) {
			debug("try to reset config!");
			ret = -1;
		}
	} else {
		debug("channel record load config failed!");
	}

	if(ret != 0) {
		channel_record_task_info->channel_record_info.start = 0;
		channel_record_task_info->channel_record_info.end = 0;
		OS_ASSERT(channel_record_info_save(channel_record_task_info, &channel_record_task_info->channel_record_info) == 0);
	} else {
		debug("start:%d, end:%d", channel_record_task_info->channel_record_info.start, channel_record_task_info->channel_record_info.end);
	}

	mutex_unlock(channel_record_task_info->mutex);

	osThreadDef(channel_record_task, channel_record_task, osPriorityNormal, 0, 128 * 2);
	osThreadCreate(osThread(channel_record_task), channel_record_task_info);

	request_channel_record_repair(channel_record_task_info);

	return channel_record_task_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channel_record_task_info_t *channel_record_task_info = (channel_record_task_info_t *)o;
	uint8_t *id = (uint8_t *)ctx;

	if(channel_record_task_info->id == *id) {
		ret = 0;
	}

	return ret;
}

channel_record_task_info_t *get_or_alloc_channel_record_task_info(uint8_t id)
{
	channel_record_task_info_t *channel_record_task_info = NULL;

	os_enter_critical();

	if(channel_record_task_class == NULL) {
		channel_record_task_class = object_class_alloc();
	}

	os_leave_critical();

	channel_record_task_info = (channel_record_task_info_t *)object_class_get_or_alloc_object(channel_record_task_class, object_filter, (void *)&id, (object_alloc_t)alloc_channel_record_task_info, (object_free_t)free_channel_record_task_info);

	return channel_record_task_info;
}

char *get_channel_record_item_stop_reason_des(channel_record_item_stop_reason_t stop_reason)
{
	char *des = "unknow";

	switch(stop_reason) {
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_NONE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AMOUNT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_ENERGY);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DURATION);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_REMOTE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INSULATION);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_CARD_READER);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_DISPLAY);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_POWER_MODULES);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_DOOR);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_RELAY_ADHESION);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_FORCE_STOP);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INPUT_OVER_VOLTAGE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_INPUT_LOW_VOLTAGE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ELECTRIC_LEAK_CALIBRATION);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ELECTRIC_LEAK_PROTECT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_PE_PROTECT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BREAKDOWN);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ENERGYMETER);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_CONNECTOR);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_FUNCTION_BOARD_CONNECT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_OVER_TEMPERATURE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ADHESION_P);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_ADHESION_N);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_TEMPERATURE_LIMIT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_FAULT_VOLTAGE_LIMIT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_ARCHIEVED);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_ARCHIEVED);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_ARCHIEVED);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_OVER_TEMPERATURE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_OVER_TEMPERATURE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_TEMPERATURE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OVER_CURRENT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CURRENT_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_OTHER_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRM_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCP_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRO_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRO_READY_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCS_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCL_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BST_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BSD_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_CRO_NOT_READY_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_PRECHARGE_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_EXTERN_VOLTAGE_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_CHECK_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_FAULT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_WITH_ELECTRIFIED);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ABNORMAL_VOLTAGE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_CHARGER_NOT_CONNECTED);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_L_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_L);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_N_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_N);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_0_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_1_TIMEOUT);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CHARGER_LOCK_ERROR);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_UNMATCHED_VOLTAGE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_VOLTAGE_NOT_CREDIBLE);
			add_des_case(CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_CHARGE_DISABLE);

		default: {
		}
		break;
	}

	return des;
}
